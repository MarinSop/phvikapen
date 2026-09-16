# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- One Euro filter in the core library, with a pen filter that smooths position and pressure.

## [0.1.0] - 2026-09-16

### Added

- Repository conventions: EditorConfig, line-ending normalization, clang-format, clang-tidy,
  qmlformat and qmllint settings, and pre-commit hooks.
- CMake build system with presets for macOS and Windows ARM64, a vcpkg manifest with a pinned
  baseline, warning, sanitizer and sccache configuration, and layering checks.
- Core library with the error type, `Result` alias and generated version header.
- Ink sample and stroke types with incremental bounding boxes, and a monotonic UUIDv7 generator.
- Google Benchmark suite for the core library, built by the release presets.
- Platform interfaces for ink backends, PDF documents and updates, a placeholder for the native
  Windows ink backend, and the Velopack startup hook.
- Qt Quick ink backend that captures pen and mouse input and renders wet ink incrementally
  through QRhi, backed by pressure-aware stroke tessellation in the core library.
- Application window with a tool bar, notebook tabs and the ink canvas, C++ view models exposed to
  QML, and logging to a rotating file.
- Qt Quick tests for the view models and the tool bar, and qmllint as part of every build.
- Ink recorder tool that writes raw pen and mouse events to CSV for filter tuning.
- Continuous integration on macOS and Windows on ARM, a release workflow that publishes the
  Windows installer and update packages with their checksums, and Dependabot updates.
- README with prerequisites, build and release instructions, and architecture decision records for
  the technology stack, the ink backend and the update framework.
