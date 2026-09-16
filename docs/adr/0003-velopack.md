# 0003. Velopack for packaging and updates

Date: 2026-09-16

## Status

Accepted

## Context

The application is distributed to a single non-technical user on a Windows 11 ARM64 laptop. It has
to install without administrator rights, update itself quietly, and never ask anyone to download a
new version by hand. Releases are built by continuous integration and published as GitHub releases.

The options considered:

- **An installer only** (for example Inno Setup): produces a setup program, but updating is left
  entirely to us, which is the part that actually matters here.
- **The platform store format** (MSIX): good system integration and updates, but it expects a
  store or signed distribution and adds account and signing requirements that a private project
  does not need.
- **An installer and update framework** (Velopack): produces the installer, the update packages
  and the client library that checks for and applies updates, with delta packages so that an
  update downloads only what changed. It reads releases directly from a GitHub repository.

Velopack's support for Windows on ARM64 is younger than its support for x64, which is the main
risk in choosing it.

## Decision

We will use Velopack for the installer, the update packages and the in-application update check,
publishing to GitHub releases.

The client library is downloaded during the build as a prebuilt archive pinned to a version and a
SHA-256 hash, rather than being committed to this repository. Its startup hook runs as the very
first statement of the application, because the installer and the updater start the application
with arguments that this hook has to handle. The library is wrapped behind an interface of ours,
so that the rest of the code does not depend on it directly.

## Consequences

- Building a release needs the .NET software development kit, because the packaging tool is
  distributed that way. This affects continuous integration, not the development machine.
- The Velopack library is a dynamic library that has to sit next to the executable, so both the
  build and the installation copy it.
- The Visual C++ runtime is installed by the Velopack bootstrapper rather than deployed next to
  the executable.
- Delta packages require the previous release to be available when packaging, so the release
  workflow downloads it first and tolerates its absence for the very first release.
- The ARM64 support has to be tested on the target device early, before the application depends on
  automatic updates. If it turns out to be unreliable, the fallback is a plain installer plus a
  manual update, which costs the automatic update path but nothing else.
