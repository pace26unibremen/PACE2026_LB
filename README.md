# PACE 2026 — Maximum Agreement Forest

Solvers for the [PACE Challenge 2026](https://pacechallenge.org/2026/) (Maximum
Agreement Forest). Given two forests over the same terminal labels, find the
minimum number of edges to cut so the resulting forests become isomorphic.

By Jonas Schramm, Philip Kail, Jurin Hoffmann, Alexander Wachowski, Leon Flaack,
André Kaufmann and Florian Feegel.

## Build

Requirements: [cmake][cmake] ≥ 3.27, [ninja][ninja], [clang][clang] (C++23), and
[mimalloc][mimalloc] (`libmimalloc-dev`) for the allocator speedup. The
lower-bound track additionally uses the bundled [UWrMaxSat][uwrmaxsat] MaxSAT
backend, which needs [GMP][gmp] (`libgmp-dev`) and zlib (`zlib1g-dev`).

On Debian 13.5 (the competition environment) just run the setup script. It
installs the toolchain and builds the solver with the flags below:

```sh
./docker_setup.sh
```

The result is `build/src/startSolver`.

To build manually on any system that already has the dependencies:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_UWRMAXSAT=ON -DCMAKE_CXX_FLAGS="-flto=thin" -DCMAKE_EXE_LINKER_FLAGS="-flto=thin -lmimalloc"
cmake --build build --target startSolver
```

What the flags do: `-DCMAKE_BUILD_TYPE=Release` enables `-O3` and `-lmimalloc`
links the [mimalloc][mimalloc] allocator (~20% faster on this allocation-heavy
solver) — those are the two that matter for speed; `-flto=thin` just shrinks the
binary. `-DBUILD_UWRMAXSAT=ON` builds the incremental MaxSAT lower-bound solver
used by the lower-bound track; `startSolver` does not link without it on this
branch. The solver also builds and runs without mimalloc; drop the `-lmimalloc`
flag and the `libmimalloc-dev` dependency for a plain build, it will just be
slower.

## Usage

```sh
./build/src/startSolver [--track PRESET] [INFILE [OUTFILE]]
```
> Notice: You can use the `--track` option, but for this specific solver, only the lower-bound track is activated and usable and selected by default, so you do not need it.

- **With a file:** reads `INFILE` (`.nw` / `.tree`) and writes the solution to
  `OUTFILE`. If `OUTFILE` is omitted it defaults to `INFILE_solution.tree`.
- **Without arguments:** reads the instance from **stdin** and writes the
  solution to **stdout**. This is the mode the PACE evaluator uses.

```sh
# file in, file out
./build/src/startSolver instance.nw solution.tree

# stdin/stdout (as PACE runs it), choosing a track
./build/src/startSolver --track exact < instance.nw > solution.tree
```

## Tracks

The solver ships one preset per PACE track, selected with `--track`:

| Preset        | Use for                     | Notes                                                                 |
|---------------|-----------------------------|-----------------------------------------------------------------------|
| `lower-bound` | Lower-bound track           | Branching + incremental MaxSAT coordinator; emits **only** a certified solution within the `a·OPT + b` bound (no output if it cannot certify). |

```sh
./build/src/startSolver --track lower-bound  < instance.nw > solution.tree
```

If no track is given, the compiled-in default (`lower-bound`) is used.

The track can also be set without the flag, which is convenient for evaluation
harnesses that only pass stdin/stdout. Precedence, highest first:

1. `--track PRESET` on the command line
2. the `PACE_TRACK` environment variable
3. `PACE_TRACK=<preset>` in a `.env` file in the working directory

```sh
PACE_TRACK=lower-bound ./build/src/startSolver < instance.nw > solution.tree
```

Run `./build/src/startSolver --help` for the full option list.

[cmake]: https://cmake.org/
[ninja]: https://ninja-build.org/
[clang]: https://clang.llvm.org/
[mimalloc]: https://github.com/microsoft/mimalloc
[uwrmaxsat]: https://github.com/marekpiotrow/UWrMaxSat
[gmp]: https://gmplib.org/
