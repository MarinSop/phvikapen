# 0002. Ink backend for Windows on ARM

Date: 2026-09-16

## Status

Proposed

## Context

The application must show ink as close to the pen tip as possible. The goal is that ink appears
within one frame of the input event on the target device, a Windows 11 ARM64 laptop with an active
stylus, and that slowly drawn diagonal lines look straight rather than wobbly.

A portable backend already exists: a Qt Quick item that receives pointer and tablet events and
renders the stroke being written on the GPU. It is what development on macOS uses. Whether it is
fast enough on the target device is unknown, because the input path there goes through the
compositor like any other application window.

Windows offers two lower level routes, and they differ in how much of the pipeline we own:

- **Option A, the system ink presenter.** Windows renders the wet stroke itself, on its own
  high priority thread, and hands the finished stroke back to the application. The lowest latency
  for the least amount of code, but the surface has to be composed into the application window,
  and smoothing and prediction are the system's, not ours.
- **Option B, our own renderer on a low latency swap chain.** The application receives pointer
  input directly, filters and predicts it, and draws into a swap chain configured for minimal
  frame latency. Full control over filtering and appearance, at the price of considerably more
  code and of owning prediction.

Both routes need the Windows Runtime projection for C++, which the build system is already
prepared for.

The decision is hard to reverse once tiles, undo and persistence are built on top of it, so it is
worth measuring rather than guessing.

## Decision

Undecided. We will build a minimal prototype of both options and measure them on the target device
before choosing. The criteria, in order:

1. Pen-to-pixel latency, measured from a 240 frames per second recording of the screen while
   writing.
2. How straight a slowly drawn diagonal line looks, from the same recordings.
3. How cleanly the surface composes with the rest of the application window.
4. The amount of code we have to own and maintain.

The portable Qt backend stays in place as the fallback and as the development path on macOS, no
matter which option wins. This record is updated with the outcome and moved to Accepted.

## Consequences

- The milestone that follows is a spike with a measurable result, not a feature.
- Until the spike concludes, the ink backend stays behind an interface with a single
  implementation, which costs one indirection and keeps the choice open.
- The measurements need the target device and a camera, so this decision cannot be made from the
  development machine alone.
- If neither option meets the latency goal, the remaining lever is reducing work per frame rather
  than switching frameworks, which is a much larger change.
