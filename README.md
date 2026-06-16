# stargaze

This project is a chess engine that I'm trying to create. The end goal for it is being able to defeat my friend in chess.

This project is only possible because of the amazing docs from [Chess Programming Wiki](https://www.chessprogramming.org).

## Building

This project uses a standard `Makefile`.

- **Release (Default)**: `make release` or just `make`
  Builds the engine with aggressive optimisations.
- **Debug**: `make debug`
  Builds the engine with debug symbols and sanitisers.
- **Perft**: `make perft`
  Builds the standalone perft testing executable (`bin/perft`).

### Other Commands

- `make run`: Builds the release version and runs the engine.
- `make run-debug`: Builds the debug version and runs the engine.
- `make run-perft`: Builds and runs a perft performance test. By default, it tests the starting position at depth 5. You can customise the test position and depth:
  ```bash
  make run-perft FEN="r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1" DEPTH=4
  ```
- `make clean`: Removes the `build/` and `bin/` directories.

The build process uses dependency tracking (`.d` files) and object compilation (`.o` files) so only modified files are recompiled.
