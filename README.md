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
- `make clean`: Removes the `build/` and `bin/` directories.
- `make compdb`: Regenerates `compile_commands.json` for clangd.
- `make test`: Runs all tests.
- `make test-unit`: Runs unit tests only.
- `make benchmark`: Run the release-mode position benchmarks to measure time to depth and depth reached in one second:

## UCI Protocol & Testing

The engine supports the Universal Chess Interface (UCI) protocol, which allows playing against it via chess GUIs (like Arena or Cute Chess) or setting up matches against other engines.

To run with a GUI, simply load the compiled binary `./bin/stargaze` into your chess GUI of choice.

In addition to standard UCI commands, Stargaze supports:

- `d` or `print`: Prints the current board state as text.

### Playing the Engine Against Itself with `cutechess-cli`

You can use [cutechess-cli](https://github.com/cutechess/cutechess/) to test the engine's performance by playing it against itself or other engines.

To play a 10-game match between Stargaze and itself with 10 seconds of time control:

```bash
cutechess-cli -engine cmd=./bin/stargaze name=Stargaze_1 -engine cmd=./bin/stargaze name=Stargaze_2 -each proto=uci tc=10 -games 10 -repeat
```

### Engine Matches

Install `python-chess` and use the match target to compare any two UCI engine binaries. It cycles through `tools/data/openings.epd`, playing every position twice with reversed colours. Use at least 48 games to cover every bundled position once per colour.

```bash
python3 -m pip install python-chess

# Current Stargaze against a saved baseline binary.
make match ENGINE_B=./bin/stargaze-baseline NAME_B=Baseline \
    GAMES=96 BASE=2 INCREMENT=0.05

# Fast approximate strength check against weakened Stockfish.
make match ENGINE_B=/path/to/stockfish GAMES=96 BASE=2 INCREMENT=0.05 \
    MATCH_ARGS="--elo-b 2200"

# Stockfish's older skill-level mode deliberately chooses weaker moves.
make match ENGINE_B=/path/to/stockfish MATCH_ARGS="--skill-b 7"
```

Set `ENGINE_A`, `ENGINE_B`, `NAME_A`, and `NAME_B` to select the competitors. `MATCH_ARGS` accepts strength controls (`--elo-a/b` or `--skill-a/b`) and `--openings path/to/suite.epd`. The tool always reports Engine A's relative Elo difference; when `--elo-b` is supplied, it also estimates Engine A's absolute Elo.
