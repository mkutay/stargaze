# stargaze

Stargaze is a C++23 UCI chess engine built around bitboards, magic move generation, iterative-deepening alpha-beta search, and a transposition table.

This project is only possible because of the amazing docs from [Chess Programming Wiki](https://www.chessprogramming.org).

---

Requires `clang++` with C++23 support and GNU Make.

- `make` or `make release`: optimized native build.
- `make debug`: debug build with UndefinedBehaviorSanitizer.
- `make sanitize`: debug build with AddressSanitizer and UndefinedBehaviorSanitizer.
- `make verify`: debug build with internal consistency checks.
- `make run`, `make run-debug`, `make run-verify`: build and run the selected profile.
- `make run-perft`: Runs a perft performance test. By default, it builds the release version and tests the starting position at depth 5 by piping UCI commands into the engine. You can customise the test position and depth:
    ```bash
    make run-perft FEN="r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1" DEPTH=4
    ```
- `make clean`: Removes the `build/` and `bin/` directories.
- `make compdb`: Regenerates `compile_commands.json` for clangd.
- `make test`: Runs all tests.
- `make test-unit`: Runs unit tests only.
- `make benchmark`: Run the release-mode position benchmarks to measure time to depth and depth reached in one second:

## UCI Protocol & Testing

The engine supports the Universal Chess Interface (UCI) protocol, which allows playing against it via chess GUIs (like Arena or Cute Chess) or setting up matches against other engines.

To run with a GUI, simply load the compiled binary `./bin/stargaze` into your chess GUI of choice.

You can run the engine in self-play mode to test its performance against itself:

```bash
./bin/stargaze --selfplay
```

In addition to standard UCI commands, Stargaze supports:

- `perft <depth>`: Runs a perft performance test from the current position to the specified depth (with move division).
- `go perft <depth>`: Alternative UCI-compatible syntax to run a perft test.
- `d` or `print`: Prints the current board state as text.

### Playing the Engine Against Itself with `cutechess-cli`

You can use [cutechess-cli](https://github.com/cutechess/cutechess/) to test the engine's performance by playing it against itself or other engines.

To play a 10-game match between Stargaze and itself with 10 seconds of time control:

```bash
cutechess-cli -engine cmd=./bin/stargaze name=Stargaze_1 -engine cmd=./bin/stargaze name=Stargaze_2 -each proto=uci tc=10 -games 10 -repeat
```

### Estimating Strength Against Stockfish

Install `python-chess`, build Stargaze, and run the match tool. It cycles through the checked-in `tools/data/openings.epd` suite and plays every position twice with reversed engine colours. `--games` is rounded up to a complete pair; use at least 48 games to cover every bundled position once per colour. The tool reports Stargaze's estimated absolute Elo from Stockfish's approximate skill-level rating.

```bash
python3 -m pip install python-chess
make match-stockfish STOCKFISH=/path/to/stockfish SKILL=5 GAMES=96 \
    BASE=10 INCREMENT=0.1 PGN=games/stockfish-match.pgn
```

Use `MATCH_ARGS="--openings path/to/suite.epd"` to pass additional options. More games and several Stockfish skill levels produce a more useful estimate; the reported confidence interval quantifies sampling uncertainty but cannot guarantee an exact Elo.
