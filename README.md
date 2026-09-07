<h1 align="center">Horizon</h1>

<p align="center">
  <em>Online colouring of interval graphs under bounded lookahead.</em><br>
  <sub>C++20 · CMake · GoogleTest</sub>
</p>

---

Intervals arrive one at a time and must be coloured irrevocably on arrival.
Offline the optimum ω is the largest number of intervals covering a point;
online the best possible competitive ratio is 3 (Kierstead-Trotter, 1981).
**Does a buffer holding the next `k` intervals help?**

No, for any constant `k`. The adversary can always fill the buffer with
intervals that say nothing about the decision at hand: parked far away in the
weak model, nested inside the current interval in the strong one.

- `f_weak(k) = 3` for every constant `k`: proved.
- Strong lookahead buys nothing against stub-friendly adversaries: proved.
- `f_strong(k) = 3` in general: conjecture; needs the 1981 lower-bound
  construction to be stub-friendly, which is not checkable against a source
  that is not available online.

A simulator that could have contradicted all three does not:
[`docs/proof_k1.md`](docs/proof_k1.md) §6-7 has the measurements.

The proofs were worked out by hand; AI assistance was used afterwards for
proofreading and copy-editing ([provenance](docs/proof_k1.md)).

## Layout

| | |
| --- | --- |
| [`docs/model.md`](docs/model.md) | The lookahead model, stated precisely. Read first. |
| [`docs/proof_k1.md`](docs/proof_k1.md) | Proofs, the conjecture, and everything measured. |
| [`docs/references.md`](docs/references.md) | Sources, with a confidence marker on each. |
| [`src/`](src/) | Offline optimum, three colourers, generators, padding transforms. |
| [`bench/`](bench/) | `bench` (measurement harness), `solve` (exhaustive game solver). |
| [`results/`](results/) | Raw CSVs. `reproduce.ps1` regenerates all of them. |

## Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/horizon_tests
```

On Windows/MSYS2 put the toolchain first on `PATH` before configuring, or CMake
picks up conflicting DLLs from another MinGW install:
`$env:Path = "C:\msys64\mingw64\bin;" + $env:Path`

## Run

```bash
./build/bench --algo=lookahead --gen=adversary --omega=5 --n=120 --k=8 --pad=strong
./build/solve --cells=9 --omega=2 --colours=3 --k=1 --moves=14
pwsh -File results/reproduce.ps1     # regenerates every CSV, ~1 minute
```

`bench` writes per-trial CSV, `solve` prints a verdict; pass a bad option to
either for the full list. Every generator takes a seed and every run is
reproducible. Colourings are validated after every run and an invalid one
throws, in release builds too.
