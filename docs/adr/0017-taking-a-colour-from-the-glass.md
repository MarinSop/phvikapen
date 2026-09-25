# 0017. Taking a colour from the glass

Date: 2026-09-26

## Status

Accepted

## Context

The colour picker walked the page: it asked the boxes of typed text, then every stroke of ink, then
the imported document underneath. Anything else gave nothing at all. A table gave nothing, a
picture put on the page gave nothing, the paper and its ruling gave nothing, and a stroke picked up
its colour as it was stored rather than as it was drawn, so a highlighter that is half transparent
handed over the colour it would have been on white.

Every kind of thing added to a page would have had to be added to that walk as well.

## Decision

**The colour is read from the window as it was drawn.** When the picker is used, the window is read
back and the pixel under the pointer is taken. What the reader sees and what they are handed are
the same thing, by construction.

Everything follows from that without being named: paper, ruling, ink, pictures, tables, typed text,
an imported document, one thing over another, transparency, the zoom, the scroll and the pixel
ratio of the screen. A kind of thing added to a page later is picked up on the day it is drawn.

**Where the window cannot be read back, the walk through the page answers instead.** Nothing is
lost that worked before.

## Consequences

- The picker works on everything the reader can see, and needs no line of code per kind of thing.
- A colour half transparent over another is handed over as it looks, not as it was stored. That is
  what a picker is for, and it is a change from what the picker did before.
- Reading the window back is a round trip to the graphics card. It happens once, when the pointer
  is put down, and never while the pointer moves.
- A window with no screen behind it hands back nothing of what the canvas drew. The tests say so
  and stand aside there rather than pretending; they run wherever there is a window system.
