# stargaze

This project is a chess engine that I'm trying to create. The end goal for it is being able to defeat my friend in chess.

This project is only possible because of the amazing docs from [Chess Programming Wiki](https://www.chessprogramming.org).

## Building

This project uses a standard `Makefile`.

- **Release (Default)**: `make release` or just `make`
  Builds the engine with aggressive optimisations.
- **Debug**: `make debug`
  Builds the engine with debug symbols and sanitisers.

### Other Commands

- `make run`: Builds the release version and runs the engine.
- `make run-debug`: Builds the debug version and runs the engine.
- `make run-perft`: Runs a perft performance test. By default, it builds the release version and tests the starting position at depth 5 by piping UCI commands into the engine. You can customise the test position and depth:
    ```bash
    make run-perft FEN="r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1" DEPTH=4
    ```
- `make clean`: Removes the `build/` and `bin/` directories.

The build process uses dependency tracking (`.d` files) and object compilation (`.o` files) so only modified files are recompiled.

## Testing

Stargaze has a comprehensive test suite built on the **doctest** framework. It includes unit tests and integration tests.

- **Run all tests**:
    ```bash
    make test
    ```
- **Run unit tests only** (skip integration tests like perft and process spawning):
    ```bash
    make test-unit
    ```
- **Filter specific tests** (using doctest command-line arguments):

    ```bash
    # Run only perft validation tests
    ./bin/stargaze_tests --test-case="*perft*"

    # Run only unit tests
    ./bin/stargaze_tests --test-suite="unit"
    ```

## UCI Protocol & Testing

The engine supports the Universal Chess Interface (UCI) protocol, which allows playing against it via chess GUIs (like Arena or Cute Chess) or setting up matches against other engines.

### Running with a GUI

Simply load the compiled binary `./bin/stargaze` into your chess GUI of choice.

### Self-Play for Testing/Debugging

```bash
./bin/stargaze --selfplay
```

### Custom UCI Commands

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
