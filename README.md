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

Install `python-chess`, build Stargaze, and run the match tool. It alternates colours and reports Stargaze's estimated absolute Elo from Stockfish's approximate skill-level rating.

```bash
python3 -m pip install python-chess
make release
python3 tools/match_stockfish.py --stockfish /path/to/stockfish \
    --skill 5 --games 100 --base 10 --increment 0.1 \
    --pgn games/stockfish-match.pgn
```
