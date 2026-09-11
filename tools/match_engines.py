#!/usr/bin/env python3
"""Run a timed, opening-diverse match between two UCI chess engines."""

import argparse
import math
import shutil
import sys
import time
from datetime import date
from pathlib import Path

try:
    import chess
    import chess.engine
    import chess.pgn
except ImportError:
    print(
        "Error: python-chess is required (python3 -m pip install python-chess).",
        file=sys.stderr,
    )
    sys.exit(1)


DEFAULT_OPENINGS = Path(__file__).with_name("data") / "openings.epd"


def resolve_executable(value: str) -> str:
    path = Path(value).expanduser()
    if path.exists():
        return str(path.resolve())
    resolved = shutil.which(value)
    if resolved:
        return resolved
    raise FileNotFoundError(value)


def elo_difference(score: float) -> float:
    if score <= 0.0:
        return -math.inf
    if score >= 1.0:
        return math.inf
    return 400.0 * math.log10(score / (1.0 - score))


def format_elo(value: float) -> str:
    if math.isinf(value):
        return "+infinity" if value > 0 else "-infinity"
    return f"{value:+.0f}"


def format_rating(value: float) -> str:
    if math.isinf(value):
        return "+infinity" if value > 0 else "-infinity"
    return f"{value:.0f}"


def score_interval(points: float, games: int) -> tuple[float, float]:
    """95% Wilson interval, treating each game's score as a proportion."""
    proportion = points / games
    z = 1.959963984540054
    denominator = 1.0 + z * z / games
    centre = (proportion + z * z / (2.0 * games)) / denominator
    variance = proportion * (1.0 - proportion) / games
    variance += z * z / (4.0 * games * games)
    margin = z * math.sqrt(variance) / denominator
    return max(0.0, centre - margin), min(1.0, centre + margin)


def configure_strength(
    engine: chess.engine.SimpleEngine,
    name: str,
    elo: int | None,
    skill: int | None,
) -> None:
    if elo is not None:
        required = ("UCI_LimitStrength", "UCI_Elo")
        if any(option not in engine.options for option in required):
            raise RuntimeError(f"{name} does not expose UCI_LimitStrength and UCI_Elo")
        option = engine.options["UCI_Elo"]
        if (option.min is not None and elo < option.min) or (
            option.max is not None and elo > option.max
        ):
            raise RuntimeError(
                f"{name} supports UCI_Elo from {option.min} to {option.max}, not {elo}"
            )
        engine.configure({"UCI_LimitStrength": True, "UCI_Elo": elo})
    elif skill is not None:
        if "Skill Level" not in engine.options:
            raise RuntimeError(f"{name} does not expose the 'Skill Level' UCI option")
        option = engine.options["Skill Level"]
        if (option.min is not None and skill < option.min) or (
            option.max is not None and skill > option.max
        ):
            raise RuntimeError(
                f"{name} supports Skill Level from {option.min} to {option.max}, not {skill}"
            )
        engine.configure({"Skill Level": skill})


def strength_label(name: str, elo: int | None, skill: int | None) -> str:
    if elo is not None:
        return f"{name} (Elo {elo})"
    if skill is not None:
        return f"{name} (skill {skill})"
    return name


def load_openings(path: Path) -> list[tuple[str, chess.Board]]:
    openings: list[tuple[str, chess.Board]] = []
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except OSError as error:
        raise OSError(f"cannot read opening suite {path}: {error}") from error

    for line_number, raw_line in enumerate(lines, 1):
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        try:
            board, operations = chess.Board.from_epd(line)
        except ValueError as error:
            raise ValueError(
                f"{path}:{line_number}: invalid EPD position: {error}"
            ) from error
        name = str(operations.get("id", f"Position {len(openings) + 1}"))
        if not board.is_valid():
            raise ValueError(f"{path}:{line_number}: opening position is not legal")
        if board.is_game_over(claim_draw=True):
            raise ValueError(
                f"{path}:{line_number}: opening position is already game over"
            )
        openings.append((name, board))

    if not openings:
        raise ValueError(f"opening suite {path} contains no positions")
    return openings


def play_game(
    engine_paths: tuple[str, str],
    engine_names: tuple[str, str],
    elos: tuple[int | None, int | None],
    skills: tuple[int | None, int | None],
    engine_a_is_white: bool,
    base_seconds: float,
    increment_seconds: float,
    opening_name: str,
    opening_board: chess.Board,
    pair_number: int,
) -> tuple[chess.pgn.Game, str, str]:
    board = opening_board.copy()
    white = 0 if engine_a_is_white else 1
    black = 1 - white
    order = (white, black)
    white_name = strength_label(engine_names[white], elos[white], skills[white])
    black_name = strength_label(engine_names[black], elos[black], skills[black])
    engines: list[chess.engine.SimpleEngine] = []
    game = chess.pgn.Game()
    game.headers.update(
        {
            "Event": f"{engine_names[0]} vs {engine_names[1]}",
            "Site": "Local",
            "Date": date.today().strftime("%Y.%m.%d"),
            "Round": str(pair_number),
            "White": white_name,
            "Black": black_name,
            "TimeControl": f"{base_seconds:g}+{increment_seconds:g}",
            "Opening": opening_name,
            "EngineAPath": engine_paths[0],
            "EngineBPath": engine_paths[1],
        }
    )
    game.setup(board)
    node = game
    clocks = [base_seconds, base_seconds]
    result = "*"
    termination = "unterminated"

    try:
        engines = [
            chess.engine.SimpleEngine.popen_uci(engine_paths[index]) for index in order
        ]
        for turn, index in enumerate(order):
            configure_strength(
                engines[turn], engine_names[index], elos[index], skills[index]
            )

        while not board.is_game_over(claim_draw=True):
            turn = 0 if board.turn == chess.WHITE else 1
            limit = chess.engine.Limit(
                white_clock=max(clocks[0], 0.001),
                black_clock=max(clocks[1], 0.001),
                white_inc=increment_seconds,
                black_inc=increment_seconds,
            )
            started = time.perf_counter()
            try:
                move = engines[turn].play(board, limit).move
            except (
                chess.engine.EngineError,
                chess.engine.EngineTerminatedError,
                TimeoutError,
            ) as error:
                result = "0-1" if turn == 0 else "1-0"
                failed_name = white_name if turn == 0 else black_name
                termination = f"{failed_name} engine failure: {error}"
                break
            clocks[turn] -= time.perf_counter() - started
            if clocks[turn] < -0.1:
                result = "0-1" if turn == 0 else "1-0"
                failed_name = white_name if turn == 0 else black_name
                termination = f"{failed_name} time forfeit"
                break
            clocks[turn] += increment_seconds

            if move is None or move not in board.legal_moves:
                result = "0-1" if turn == 0 else "1-0"
                failed_name = white_name if turn == 0 else black_name
                termination = f"{failed_name} returned an illegal move"
                break
            board.push(move)
            node = node.add_variation(move)
    finally:
        for engine in engines:
            try:
                engine.quit()
            except (chess.engine.EngineError, chess.engine.EngineTerminatedError):
                pass

    if result == "*":
        outcome = board.outcome(claim_draw=True)
        if outcome is not None:
            result = outcome.result()
            termination = outcome.termination.name.replace("_", " ").lower()
    game.headers["Result"] = result
    game.headers["Termination"] = termination

    if result == "1/2-1/2":
        engine_a_result = "draw"
    elif (result == "1-0") == engine_a_is_white:
        engine_a_result = "win"
    else:
        engine_a_result = "loss"
    return game, engine_a_result, termination


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Play two UCI engines and estimate Engine A's Elo difference."
    )
    parser.add_argument("--engine-a", default="./bin/stargaze")
    parser.add_argument("--engine-b", default="stockfish")
    parser.add_argument("--name-a", default="Stargaze")
    parser.add_argument("--name-b", default="Stockfish")
    for suffix in ("a", "b"):
        group = parser.add_mutually_exclusive_group()
        group.add_argument(f"--elo-{suffix}", type=int, metavar="ELO")
        group.add_argument(f"--skill-{suffix}", type=int, metavar="LEVEL")
    parser.add_argument(
        "--games",
        type=int,
        default=48,
        help="minimum number of games (rounded up to a complete opening pair)",
    )
    parser.add_argument("--base", type=float, default=10.0, help="seconds per side")
    parser.add_argument("--increment", type=float, default=0.1, help="seconds per move")
    parser.add_argument("--pgn", type=Path, help="write all games to this PGN file")
    parser.add_argument("--openings", type=Path, default=DEFAULT_OPENINGS)
    args = parser.parse_args()
    if args.games < 1 or args.base <= 0 or args.increment < 0:
        parser.error(
            "--games and --base must be positive; --increment cannot be negative"
        )
    if any(value is not None and value < 0 for value in (args.elo_a, args.elo_b)):
        parser.error("--elo-a and --elo-b cannot be negative")
    return args


def main() -> int:
    args = parse_args()
    try:
        engine_paths = (
            resolve_executable(args.engine_a),
            resolve_executable(args.engine_b),
        )
        openings = load_openings(args.openings)
    except FileNotFoundError as error:
        print(f"Error: executable not found: {error}", file=sys.stderr)
        return 2
    except (OSError, ValueError) as error:
        print(f"Error: {error}", file=sys.stderr)
        return 2

    engine_names = (args.name_a, args.name_b)
    elos = (args.elo_a, args.elo_b)
    skills = (args.skill_a, args.skill_b)
    total_games = args.games + args.games % 2
    if total_games != args.games:
        print(
            f"Rounding --games up to {total_games} to complete the final opening pair."
        )
    print(f"Loaded {len(openings)} opening positions from {args.openings}.")

    wins = draws = losses = 0
    pgn_file = None
    try:
        if args.pgn:
            args.pgn.parent.mkdir(parents=True, exist_ok=True)
            pgn_file = args.pgn.open("w", encoding="utf-8")
        for game_number in range(1, total_games + 1):
            pair_number = (game_number - 1) // 2
            opening_name, opening_board = openings[pair_number % len(openings)]
            engine_a_is_white = game_number % 2 == 1
            game, result, termination = play_game(
                engine_paths,
                engine_names,
                elos,
                skills,
                engine_a_is_white,
                args.base,
                args.increment,
                opening_name,
                opening_board,
                pair_number + 1,
            )
            wins += result == "win"
            draws += result == "draw"
            losses += result == "loss"
            if pgn_file:
                print(game, file=pgn_file, end="\n\n")
                pgn_file.flush()
            colour = "White" if engine_a_is_white else "Black"
            print(
                f"Game {game_number}/{total_games}: {args.name_a} {result} as {colour}, "
                f"{opening_name} ({termination}); W-D-L {wins}-{draws}-{losses}",
                flush=True,
            )
    except (OSError, RuntimeError, chess.engine.EngineError) as error:
        print(f"Error: {error}", file=sys.stderr)
        return 2
    finally:
        if pgn_file:
            pgn_file.close()

    points = wins + 0.5 * draws
    score = points / total_games
    low_score, high_score = score_interval(points, total_games)
    difference = elo_difference(score)
    low_elo, high_elo = elo_difference(low_score), elo_difference(high_score)
    print(f"\n{args.name_a} score: {points:g}/{total_games} ({score:.1%})")
    print(f"Estimated Elo difference versus {args.name_b}: {format_elo(difference)}")
    print(f"Approximate 95% interval: [{format_elo(low_elo)}, {format_elo(high_elo)}]")
    if args.elo_b is not None:
        print(f"Estimated {args.name_a} Elo: {format_rating(args.elo_b + difference)}")
        print(
            f"Approximate {args.name_a} Elo 95% interval: "
            f"[{format_rating(args.elo_b + low_elo)}, "
            f"{format_rating(args.elo_b + high_elo)}]"
        )
        print("Target Elo calibration varies by engine, hardware, and time control.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
