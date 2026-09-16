# 0001. C++23 with Qt Quick for the user interface

Date: 2026-09-16

## Status

Accepted

## Context

PhvikaPen is a pen-first note-taking application. It is written for Windows 11 on ARM64, while
development happens on macOS on Apple Silicon. Two properties decide whether the application is
pleasant to use: the delay between moving the pen and seeing ink, and the absence of wobble in
slowly drawn lines. Both depend on how directly the application can reach input and the GPU.

The application also has to draw thousands of strokes per page, render imported PDF pages, and
keep a usable frame rate on a laptop class ARM processor.

Candidate technology stacks:

- **C++ with Qt Quick.** One codebase for the development and the target platform. Rendering goes
  through Qt's hardware abstraction, which maps to Direct3D on Windows and Metal on macOS. Custom
  items can take over drawing entirely, and the platform specific input path can be replaced later
  without touching the user interface.
- **C++ with Qt Widgets.** Mature, but its painting model is tied to the CPU-oriented QPainter
  stack, which is the wrong starting point for a GPU tile cache.
- **A managed or web based stack (WinUI, Electron, Flutter).** Either ties the project to Windows
  from the start, or puts a runtime and a compositor between the pen and the pixels, which is
  exactly where the latency budget goes.

## Decision

We will write the application in C++23 and build the user interface with Qt 6.11 and Qt Quick,
using Qt Quick Controls for standard widgets.

C++ types are exposed to QML declaratively, through the registration macros rather than imperative
registration calls, and QML is compiled ahead of time. Custom drawing uses the scene graph and the
Qt rendering hardware interface. Qt Widgets is not used.

The code is split into three layers. The core layer is plain C++23 with no Qt and no operating
system dependency, so that the model, the filters and the geometry can be tested quickly and stay
portable. The platform layer hides operating system specifics behind interfaces. The application
layer holds the QML and the view models. Dependencies point downward only.

## Consequences

- The same code builds and runs on macOS and on Windows on ARM, so most work happens on the
  development machine and only pen behavior has to be verified on the target device.
- Rendering has a stable GPU path on both platforms, and the canvas can later keep finished
  strokes in GPU tiles without changing the user interface.
- The core layer stays fast to test and free of Qt, at the cost of a little ceremony: types from
  the platform layer are exposed to QML from the application layer instead of directly.
- The build depends on Qt's code generators, which the build system runs automatically, and on
  their linting tools, which are wired into every build.
- Errors cross layer boundaries as values rather than exceptions, which keeps the interfaces
  explicit but requires discipline at every call site.
