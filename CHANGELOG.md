# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- One Euro filter in the core library, with a pen filter that smooths position and pressure.
- Binary stroke format that stores samples as varint differences, ready for notebook files.
- Notebook files: a SQLite database per notebook, with a versioned schema and one transaction per
  stored stroke.
- What is drawn is kept: finished strokes go into a notebook file and come back when the
  application starts again.
- Undo and redo for drawing, clearing and erasing, from the tool bar and the standard shortcuts.
- Eraser that removes whole strokes, from the eraser tool or the eraser end of the pen. One sweep
  is one change for undo.
- Finished strokes are drawn along a spline as one continuous strip.
- Notebooks hold sections and pages. Pages carry their own paper size, orientation, background and
  line spacing, and can be added, deleted, moved and renamed, all of it undoable.
- The page is shown through a viewport that scrolls and zooms with the wheel, the trackpad, a pinch
  or the fingers on a touch screen, while the pen keeps drawing.
- Paper and its ruling are drawn on the GPU: a sheet on a desk, or an endless canvas, blank, lined,
  squared or dotted.
- A sidebar with the sections and pages of the notebook and the page setup, and page navigation in
  the tool bar and from the keyboard.
- Several notebooks open at once in tabs, created, opened, renamed and deleted from the tab bar.
  The notebooks that were open, and the page each was left on, come back at the next start.
- Three pens, a highlighter and an eraser whose size can be set, with a colour palette. The tools
  are remembered between sessions.
- Messages that used to go only to the log are shown in the window.
- PDFs and pictures can be imported: a PDF adds a page per page of it, a picture adds one page, and
  both are drawn behind the ink so they can be written on. The files are kept inside the notebook.
- A notebook can be written out as a PDF: one page of the document per page of the notebook, each
  keeping its own size, with the paper, its ruling, imported pages and the ink on them.
- The application looks for a newer version when it starts and offers to install it and restart.
- A settings dialog: whether to look for updates at start, where the notebooks are kept, and which
  version this is.
- Deleted pages and sections wait in a trash that can be looked at, put back from, or emptied for
  good, which also removes imported files no page shows any more and shrinks the notebook file.
- Keyboard shortcuts for the tools: P, H and E, the pens on 1 to 3, and [ and ] for the width.
- Going back to a page returns to the zoom and the place it was left at.
- A copy of a notebook can be saved somewhere safe from inside the application.

### Changed

- Erasing, undoing and leafing back to a page reuse the shapes of the strokes that did not change
  instead of working all of them out again.
- Pictures are read on their own thread, so a large photograph no longer holds up the window.

- The ink canvas draws and stores smoothed samples, while ink backends keep reporting the raw ones.
- Notebook files are read and written on a storage thread, so drawing never waits for the disk.
- Every stroke keeps a fixed place on its page; notebooks from earlier schema versions are upgraded.
- Notebooks live in the application data folder instead of the local application data folder,
  which the Windows installer deletes on uninstall.
- Ink stays on the sheet: a stroke cannot be started beside fixed paper and is clipped to it.
- Settings are stored under the application's own domain instead of the placeholder Qt falls back to.

### Fixed

- Removing an ink canvas no longer runs code on the half destroyed item.

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
