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
- `make clean`: Removes the `build/` and `bin/` directories.

The build process uses dependency tracking (`.d` files) and object compilation (`.o` files) so only modified files are recompiled.
