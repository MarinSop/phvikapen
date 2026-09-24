# 0010. Moving, sizing and turning what is picked up

Date: 2026-09-24

## Status

Accepted

## Context

What is picked up with the loop could be carried about the page, recoloured, copied and taken
away, but not made larger, smaller or turned. Every application of this kind can do all three, and
the pictures, tables and equations that are still to come will each need the same thing. Writing
the arithmetic again for each of them would be three chances to get it wrong.

Two questions had to be settled: where the arithmetic lives, and what the reader sees while
dragging.

**Where the arithmetic lives.** Moving, sizing and turning are one thing — a point that stays
still, a distance moved, how much wider and taller, and an angle — not three. Kept as one shape of
change, in the layer that knows nothing of the window, it can be tested on its own and used by
anything that sits on a page.

**What the reader sees.** A frame with grips could be drawn by the graphics card along with the
ink, or laid over the canvas by the window toolkit. The canvas already knows how to leave some
strokes out and draw others in their place — that is how rubbing out shows its work — so the
drawing being changed can be shown through the same door, while the frame and its grips are
ordinary items of the window, which already know about pointers, pens and being held.

## Decision

A change of place, size and angle is one value: a point that stays still, how far it moved, how
much wider and taller it became, and how far it turned, all in the units the page is measured in.
It knows how to place a point, a rectangle and a stroke, and it makes a line thicker as the
drawing grows — by what both directions come to together, since a line cannot be thick one way and
thin the other.

Undoing it does not work the arithmetic backwards. What the strokes were is kept, and undoing puts
exactly that back, so that turning something a dozen times and undoing a dozen times leaves it
where it started rather than slightly askew.

The frame and its grips are laid over the canvas: eight grips that size — the corners keep the
drawing in proportion, the edges pull one way each — and a knob above the frame that turns, in
steps of fifteen degrees while Shift is held. Nothing inside the frame takes the pointer, so
carrying what is picked about still belongs to the canvas, as it always did.

While a grip is being dragged, the strokes being changed are left out of the page and drawn in
their new shape instead, which costs nothing that rubbing out does not already cost. When the drag
ends, and only then, it becomes one change that can be undone.

Turning by a quarter in either direction is also a command with keys of its own, so that a drawing
can be squared up without dragging anything.

## Consequences

- Sizing changes how thick the lines are, which is what a reader expects of a drawing and not what
  they would expect of a photograph. When pictures arrive they will use the same value but will
  not take that part of it.
- What is picked is sized about the opposite corner of its own upright box. A drawing that has
  already been turned is sized about the box around it as it now stands, not about the shape it had
  before, which is the usual behaviour and the only one that can be shown honestly with an upright
  frame.
- The grips stand still while the frame turns under them. They follow the upright box, so during a
  turn the frame and the grips part company until the drag ends.
- Sizing a drawing changes the strokes themselves rather than remembering that it was sized. A page
  keeps no history of the shape things used to be, which keeps the file simple and means that
  making something smaller and larger again is not exactly what it was.
