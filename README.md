# G4LArBox

`G4LArBox` transports generated particles through one configurable box of liquid
argon. The detector model is intentionally small: an active `G4_lAr` box inside
an invisible vacuum world. The simulation records particle steps, tracks, the
liquid-argon medium response, and generator truth in a `ROOT` file.

## Run

The self-contained route is `Docker`:

```bash
docker build --platform linux/amd64 -t g4larbox .
mkdir -p data
docker run --rm --platform linux/amd64 \
  -v "$PWD/data:/opt/G4LArBox/data" \
  g4larbox
```

The pinned image is `linux/amd64`. On an ARM machine, prefer a remote amd64
builder: compiling the dependency stack under local emulation is resource
intensive.

This runs `simplebox.mac` with `singlegun.mac` and writes
`data/output.root`. Choose another generator macro by replacing the image's
default arguments:

```bash
docker run --rm --platform linux/amd64 \
  -v "$PWD/data:/opt/G4LArBox/data" \
  g4larbox -d simplebox.mac -g marley.mac
```

For a local build, provide `Geant4`, `ROOT`, GSL, CMake, Git, and Make, then run:

```bash
cmake -S . -B build
cmake --build build -j2
./G4LArBox -d simplebox.mac -g singlegun.mac
```

CMake fetches and builds MARLEY and BxDecay0 under the ignored `extern/`
directory when they are not already present.

## The box

`simplebox.mac` defines a one-metre cube. The three detector commands accept any
positive length:

```text
/box/width 1 m
/box/height 1 m
/box/length 1 m
```

Set `G4LARBOX_RANDOM_SEED` for a repeatable run and
`G4LARBOX_OUTPUT_FILE` to change the output path.

## Generators

All generators feed the same box and write to the same truth tree.

| Mode | Example macro | Input |
| --- | --- | --- |
| Geant4 GPS | `singlegun.mac` | `/gps/*` commands |
| GENIE GST | `genie_gst.mac` | `gst` tree in a `ROOT` file |
| GENIE nucleon decay | `genie_pdecay.mac` | final-state arrays in a `gst` tree |
| CORSIKA | `corsika_cosmics.mac` | converted `corsika` tree |
| CORSIKA + GENIE | `corsika_overlay_genie.mac` | one tree of each type |
| MARLEY | `marley.mac` | JSON configuration or event file |
| BxDecay0 | `bxdecay0_Ar39.mac`, `bxdecay0_Co60.mac`, `bxdecay0_Rn222.mac` | configured isotope |
| Radiological activity | `radiological.mac` | isotope activities in the macro |
| Rock neutrons | `rock_neutrons.mac` | spectrum and rate in the macro |

The GENIE and CORSIKA examples expect user-supplied `ROOT` files at the paths
shown in their macros.

GENIE input is read with momenta in GeV and optional vertices in metres and
seconds. CORSIKA trees use the branches `iev`, `nprimary`, `pdg`, `px`, `py`,
`pz`, `x`, `y`, `z`, and `t`, with momenta in GeV, positions in metres, and
times in seconds. Input exhaustion is an error unless the corresponding
`cycleEvents` command is enabled.

MARLEY can generate from `config/marley_ve40ar_mono.js` or replay an event file with
`/generator/marley/file`. BxDecay0 also exposes `dbd`, `dbdranged`, `mdl`, and
`mdlr` commands under `/generator/bxdecay0/`.

Radiological activity may be used as its own source or overlaid on another mode
with `/generator/radiological/enable true`. Rock neutrons can likewise be
overlaid with `/generator/rockNeutrons/enable true`.

## Output

The output file contains four trees:

- `stepTree`: deposited energy, step geometry, particle IDs, and medium response
- `trackTree`: particle vertices, endpoints, momenta, and ancestry
- `eventTree`: total excitons, ions, scintillation photons, and electrons
- `truthTree`: source-specific metadata and all injected primary particles

The original quick-look trajectory and lifetime plots remain available:

```bash
cd data
mkdir -p plots
./runplots.sh output.root
```

![Example three-dimensional particle trajectory](doc/plot_x_vs_y_vs_z.png)
