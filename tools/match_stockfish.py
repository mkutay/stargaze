#!/usr/bin/env python3
"""
Run a timed UCI match between Stargaze and Stockfish.

Install `python-chess`, build Stargaze, and run the match tool.
It alternates colours and reports Stargaze's estimated Elo difference
from the selected Stockfish skill level with an approximate 95%
interval. It also estimates Stargaze's absolute Elo from Stockfish's
approximate skill-level rating.

`--base` and `--increment` are seconds. Stockfish skill levels range
from 0 to 20. For levels 0-19, the tool uses Stockfish's published
non-linear CCRL Blitz calibration, ranging from approximately 1320 to
3190 Elo. Level 20 is unrestricted strength and has no calibrated Elo,
so only the relative Elo difference is reported for it. Actual strength
varies by Stockfish build, hardware, opponent pool, and time control.
"""

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
    print("Error: python-chess is required (python3 -m pip install python-chess).", file=sys.stderr)
    sys.exit(1)


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


def stockfish_elo(skill: int) -> float:
    """CCRL Blitz calibration published by Stockfish for skill levels 0-19."""
    calibrated_elos = (
        1320.1,
        1467.6,
        1608.4,
        1742.3,
        1922.9,
        2203.7,
        2363.2,
        2499.5,
        2596.2,
        2702.8,
        2788.3,
        2855.5,
        2923.1,
        2972.9,
        3024.8,
        3069.5,
        3111.2,
        3141.3,
        3170.3,
        3191.1,
    )
    if skill == 20:
        raise ValueError("Stockfish skill 20 is unrestricted and has no calibrated Elo")
    return calibrated_elos[skill]


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
    margin = z * math.sqrt(
        proportion * (1.0 - proportion) / games + z * z / (4.0 * games * games)
    ) / denominator
    return max(0.0, centre - margin), min(1.0, centre + margin)


def configure_stockfish(engine: chess.engine.SimpleEngine, skill: int) -> None:
    if "Skill Level" not in engine.options:
        raise RuntimeError("Stockfish does not expose the 'Skill Level' UCI option")
    engine.configure({"Skill Level": skill})


def play_game(
    stargaze_path: str,
    stockfish_path: str,
    stargaze_is_white: bool,
    skill: int,
    base_seconds: float,
    increment_seconds: float,
) -> tuple[chess.pgn.Game, str, str]:
    board = chess.Board()
    white_path = stargaze_path if stargaze_is_white else stockfish_path
    black_path = stockfish_path if stargaze_is_white else stargaze_path
    white_name = "Stargaze" if stargaze_is_white else f"Stockfish (skill {skill})"
    black_name = f"Stockfish (skill {skill})" if stargaze_is_white else "Stargaze"
    engines: list[chess.engine.SimpleEngine] = []
    game = chess.pgn.Game()
    game.headers.update(
        {
            "Event": "Stargaze vs Stockfish",
            "Site": "Local",
            "Date": date.today().strftime("%Y.%m.%d"),
            "White": white_name,
            "Black": black_name,
            "TimeControl": f"{base_seconds:g}+{increment_seconds:g}",
        }
    )
    node = game
    clocks = [base_seconds, base_seconds]
    result = "*"
    termination = "unterminated"

    try:
        engines = [
            chess.engine.SimpleEngine.popen_uci(white_path),
            chess.engine.SimpleEngine.popen_uci(black_path),
        ]
        configure_stockfish(engines[1 if stargaze_is_white else 0], skill)

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
            except (chess.engine.EngineError, chess.engine.EngineTerminatedError, TimeoutError) as error:
                result = "0-1" if turn == 0 else "1-0"
                termination = f"{white_name if turn == 0 else black_name} engine failure: {error}"
                break
            elapsed = time.perf_counter() - started
            clocks[turn] -= elapsed
            if clocks[turn] < -0.1:
                result = "0-1" if turn == 0 else "1-0"
                termination = f"{white_name if turn == 0 else black_name} time forfeit"
                break
            clocks[turn] += increment_seconds

            if move is None or move not in board.legal_moves:
                result = "0-1" if turn == 0 else "1-0"
                termination = f"{white_name if turn == 0 else black_name} returned an illegal move"
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
        stargaze_result = "draw"
    elif (result == "1-0") == stargaze_is_white:
        stargaze_result = "win"
    else:
        stargaze_result = "loss"
    return game, stargaze_result, termination


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Play Stargaze against Stockfish and estimate their Elo difference."
    )
    parser.add_argument("--stockfish", default="stockfish", help="Stockfish executable")
    parser.add_argument("--engine", default="./bin/stargaze", help="Stargaze executable")
    parser.add_argument("--skill", type=int, default=0, choices=range(21), metavar="0-20")
    parser.add_argument("--games", type=int, default=20, help="number of games")
    parser.add_argument("--base", type=float, default=10.0, help="base time per side in seconds")
    parser.add_argument("--increment", type=float, default=0.1, help="increment per move in seconds")
    parser.add_argument("--pgn", type=Path, help="write all games to this PGN file")
    args = parser.parse_args()
    if args.games < 1 or args.base <= 0 or args.increment < 0:
        parser.error("--games and --base must be positive; --increment cannot be negative")
    return args


def main() -> int:
    args = parse_args()
    try:
        stargaze_path = resolve_executable(args.engine)
        stockfish_path = resolve_executable(args.stockfish)
    except FileNotFoundError as error:
        print(f"Error: executable not found: {error}", file=sys.stderr)
        return 2

    wins = draws = losses = 0
    pgn_file = None
    try:
        if args.pgn:
            pgn_file = args.pgn.open("w", encoding="utf-8")
        for game_number in range(1, args.games + 1):
            stargaze_is_white = game_number % 2 == 1
            game, result, termination = play_game(
                stargaze_path,
                stockfish_path,
                stargaze_is_white,
                args.skill,
                args.base,
                args.increment,
            )
            wins += result == "win"
            draws += result == "draw"
            losses += result == "loss"
            if pgn_file:
                print(game, file=pgn_file, end="\n\n")
                pgn_file.flush()
            colour = "White" if stargaze_is_white else "Black"
            print(
                f"Game {game_number}/{args.games}: {result} as {colour} "
                f"({termination}); W-D-L {wins}-{draws}-{losses}",
                flush=True,
            )
    except (OSError, RuntimeError, chess.engine.EngineError) as error:
        print(f"Error: {error}", file=sys.stderr)
        return 2
    finally:
        if pgn_file:
            pgn_file.close()

    points = wins + 0.5 * draws
    score = points / args.games
    low_score, high_score = score_interval(points, args.games)
    difference = elo_difference(score)
    low_elo, high_elo = elo_difference(low_score), elo_difference(high_score)
    print(f"\nStargaze score: {points:g}/{args.games} ({score:.1%})")
    print(f"Estimated Elo difference: {format_elo(difference)}")
    print(f"Approximate 95% interval: [{format_elo(low_elo)}, {format_elo(high_elo)}]")
    if args.skill == 20:
        print("Stockfish skill 20 is unrestricted and has no calibrated Elo.")
    else:
        opponent_elo = stockfish_elo(args.skill)
        estimated_elo = opponent_elo + difference
        estimated_low = opponent_elo + low_elo
        estimated_high = opponent_elo + high_elo
        print(f"Approximate Stockfish Elo at skill {args.skill}: {opponent_elo:.0f}")
        print(f"Estimated Stargaze Elo: {format_rating(estimated_elo)}")
        print(
            "Approximate Stargaze Elo 95% interval: "
            f"[{format_rating(estimated_low)}, {format_rating(estimated_high)}]"
        )
        print("Calibration is CCRL Blitz based and varies by build and time control.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
