# PhvikaPen

A pen-first note-taking application for Windows on ARM. Notebooks open in tabs, pages are written
on with a stylus, and imported PDFs and images can be annotated. Everything is stored locally: no
account, no cloud.

The target platform is Windows 11 on ARM64. Development happens on macOS, where the application
builds and runs with a Qt-based ink canvas that accepts pen and mouse input.

What works today: notebooks in tabs, each with sections and pages, written on with a pen, a mouse
or a finger on a touch screen. Pages are paper of a chosen size or an endless canvas, blank, lined,
squared or dotted, or a page of an imported PDF or a picture to write on top of. Three pens, a
highlighter, an eraser and a tool that picks strokes to move, copy, recolour or delete share a
colour palette and their widths, the pen can draw straight lines, boxes and ovals, every change can
be undone, and
everything is written to the notebook file as it happens. A notebook can be written out as a PDF or
copied somewhere safe, and the application updates itself from its published releases.

## The window

The window is laid out the way drawing and document applications are: text menus across the top
(File, Edit, View, Insert, Tools, Window, Help), a palette of tools down the left side, the pages of
the notebook beside it, the sheet in the middle, the page setup on the right, and the page and zoom
at the bottom. Menus carry the commands and their shortcuts; the palette carries only the tools that
are used on the sheet, each with a tooltip that names it. On Windows the menus sit in the window; on
macOS they sit in the menu bar at the top of the screen, as that platform expects.

## Updates

An installed application looks for a newer version when it starts and offers to install it and
restart. The setting can be turned off in the settings dialog, which also says where the notebooks
are kept. Where to look is the build setting `PHVIKAPEN_UPDATE_FEED`: a repository of releases by
default, or a folder of releases to try the whole path out locally.

## Where notebooks are kept

One notebook is one SQLite file with the `.phvika` suffix, in the application data folder:
`%AppData%\PhvikaPen\notebooks` on Windows and `~/Library/Application Support/PhvikaPen/notebooks`
on macOS. The file name is the name of the notebook. Deleting a notebook moves it to the trash.
The location is deliberate and is explained in `docs/adr/0004-notebook-location.md`.

## Prerequisites

The development machine is macOS on Apple Silicon.

| Requirement | Notes |
|---|---|
| Xcode Command Line Tools | `xcode-select --install` |
| CMake 4.4 or newer, Ninja | `brew install cmake ninja` |
| sccache | `brew install sccache`, used automatically when present |
| pkgconf | `brew install pkgconf`, required while vcpkg builds dependencies |
| LLVM 23 | `brew install llvm@23`, provides clang-format, clang-tidy and run-clang-tidy |
| pre-commit | `brew install pre-commit`, then `pre-commit install` |
| vcpkg | Clone it outside this repository and set `VCPKG_ROOT` |
| Qt 6.11 (Core, Gui, Quick, Quick Controls, Quick Test, Shader Tools) | Install with the Qt installer and set `QT_ROOT_DIR` to the `macos` directory |
| .NET SDK | Only needed to cut a release, which uses the `vpk` tool |
| actionlint, shellcheck | Optional, for checking the workflow files locally |

Both environment variables are read by the CMake presets:

```bash
export VCPKG_ROOT="$HOME/vcpkg"
export QT_ROOT_DIR="$HOME/Qt/6.11.2/macos"
```

## Building on macOS

```bash
export VCPKG_ROOT="$HOME/vcpkg" QT_ROOT_DIR="$HOME/Qt/6.11.2/macos"
cmake --workflow --preset mac-debug
open build/mac-debug/src/app/phvikapen.app
```

The first configure builds the dependencies from source, which takes a few minutes. Later builds
reuse the vcpkg binary cache.

## Presets

| Preset | What it is for |
|---|---|
| `mac-debug` | Everyday development: debug build, tests, qmllint |
| `mac-release` | Optimized build, also builds the benchmarks |
| `mac-asan` | Debug build with AddressSanitizer and UndefinedBehaviorSanitizer |
| `win-arm64-debug` | Windows on ARM, debug |
| `win-arm64-release` | Windows on ARM, the configuration that is shipped |

Each preset exists as a configure, build, test and workflow preset. `cmake --workflow --preset X`
runs all three stages; the build stage also runs qmllint, and any QML warning fails the build.

Run only the tests of an existing build with `ctest --preset mac-debug`, and the benchmarks with
`./build/mac-release/tests/bench/phvikapen_bench`.

## Code quality

`pre-commit install` wires up formatting, spell checking and the usual file hygiene. The same
checks run in continuous integration, together with clang-tidy:

```bash
git ls-files '*.cpp' '*.hpp' | xargs "$(brew --prefix llvm@23)/bin/clang-format" -i
git ls-files '*.qml' | xargs "$QT_ROOT_DIR/bin/qmlformat" -i
"$(brew --prefix llvm@23)/bin/run-clang-tidy" -p build/mac-debug -quiet "^${PWD}/(src|tests|tools)/"
```

## Recording pen input

The ink recorder captures raw pen and mouse events so that filters can be tuned on real data from
the target device:

```bash
open build/mac-debug/tools/ink-recorder/ink_recorder.app
```

Press **Start**, write across the window, then press **Stop**. Each recording is a CSV file below
the application data directory, for example
`~/Library/Application Support/PhvikaPen Ink Recorder/recordings/` on macOS.

## Layout

```
src/core/       pure C++23: ink samples, strokes, geometry, identifiers
src/platform/   ink backends, PDF and update interfaces
src/app/        QML user interface and the view models behind it
tests/          unit tests, QML tests, benchmarks, recordings
tools/          the ink recorder
```

Dependencies point downward only: the application uses the platform layer, the platform layer uses
the core, and the core depends on neither Qt nor the operating system. A test and a configure-time
check enforce that.

## Cutting a release

1. Update the version in `project(... VERSION x.y.z ...)` in the top-level `CMakeLists.txt`.
2. Move the entries of the release from `Unreleased` into a new section in `CHANGELOG.md`.
3. Commit, then tag and push:

```bash
git tag vX.Y.Z
git push origin vX.Y.Z
```

The release workflow builds on Windows on ARM, refuses to continue if the tag and the project
version disagree, deploys the Qt runtime, packages the installer and the update packages, and
publishes them to a GitHub release together with their SHA-256 checksums.

## License

MIT, see `LICENSE`.
