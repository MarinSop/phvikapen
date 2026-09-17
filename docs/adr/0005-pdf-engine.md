# 0005. PDFium for reading and drawing PDF pages

Date: 2026-09-17

## Status

Accepted

## Context

The application has to show the pages of an imported PDF as the background of a notebook page and
let the user write on top of them. The files come from lectures and handouts, so they use the whole
range of what PDF allows: embedded fonts, scanned images, vector drawings and transparency. Pages
have to be rendered at the zoom the page is shown at, on a Windows 11 ARM64 laptop and on the macOS
machine used for development, and rendering must be able to run away from the thread that draws
ink.

Writing a PDF renderer is out of the question, so the choice is which one to use:

- **PDFium**, the renderer inside Chrome and inside Qt's own PDF module. It covers everything the
  files above contain, it is maintained continuously, and prebuilt libraries for Windows on ARM64
  and macOS on Apple silicon are published for every Chromium version. Its interface is a C API,
  which is stable and easy to wrap, but it is ours to wrap.
- **Qt PDF**, which is PDFium with a Qt interface. It would save writing the wrapper and gives
  asynchronous rendering out of the box, but it is an add-on module: it has to be present in the Qt
  installation on the development machine and added to the Qt installed by continuous integration,
  and it ties the PDF support to the Qt version.
- **MuPDF**, small and fast, but distributed under the AGPL unless a commercial licence is bought,
  which does not suit a freely distributed application.
- **Poppler**, common on Linux, with no maintained prebuilt binaries for Windows on ARM64.

## Decision

We will render PDF pages with PDFium, taking the prebuilt package for the target platform from the
pdfium-binaries releases, pinned to a version and verified against a SHA-256 hash while the project
is configured, the same way the update library is taken. The rest of the application only sees our
own `IPdfDocument` interface, so the engine can be replaced without touching anything above the
platform layer.

## Consequences

- No source of PDFium is built here, so the build stays fast and the toolchain stays simple, at the
  price of trusting a published binary; the hash pins exactly which one.
- The library is a dynamic library. On Windows it travels next to the executable and into the
  installer; on macOS its install name is rewritten when it is downloaded, because the published
  one is relative.
- The C API is ours to wrap and to keep safe: PDFium is not thread-safe, so a document is used from
  one thread only, and the wrapper owns that rule.
- Updating means bumping the version and the hashes together, and the published tags follow
  Chromium's version numbers rather than anything of ours.
- Should the wrapper ever become a burden, Qt PDF remains the fallback, because it is the same
  engine underneath.
