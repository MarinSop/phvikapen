# 0008. Typed text as boxes on a page

Date: 2026-09-24

## Status

Accepted

## Context

A page has held nothing but ink and an imported document. A reader who wants a title, a label on a
drawing or a paragraph that stays readable has to write it by hand. Every other notebook
application of this kind has a text tool: a box that is put down anywhere on the paper, typed in,
and given a face — a font, a size, a colour, and the weights and marks that go with type.

Two things had to be decided: what a box of text is, and who draws it.

**What a box is.** Either the text carries its own formatting piece by piece, so that one word
inside a box can be bold while the rest is not, or the whole box wears one face. Formatting piece
by piece means keeping a marked-up document rather than a string: it has to be stored, undone,
searched, printed and edited, and every one of those has to agree with the others. One face per box
is a string, a rectangle and a handful of numbers.

**Who draws it.** The ink is drawn by the graphics card through a scene of triangles built from the
strokes. Type could be drawn the same way, by turning glyphs into geometry, but then everything a
reader expects while typing — the caret, selecting with the mouse, the keyboard of another
language, cutting and pasting — would have to be built from nothing. The window toolkit already
has an editor that does all of it.

## Decision

A box of typed text is a page object beside the strokes: where it sits, how wide it may run, what
it says, and one face for the whole box (font, size, colour, alignment, bold, italic, underline,
struck out). It is kept in the notebook in its own table, changed through the same undo stack as
ink, and searched the same way handwriting is, so that a search finds what was typed as well as
what was written.

The window draws the boxes over the ink with the toolkit's own text items, laid out in the units of
the page and scaled to the zoom of the canvas, so that a box breaks its lines in the same places
whatever the zoom. One editor is alive at a time: the box being worked on is typed in an editor
that belongs to the layer, while every other box is drawn as plain text. What is printed or made
into a small picture of the page is drawn again with the same fonts and the same width to run in,
by the painter that already draws the paper and the ink.

The text tool takes the pen and the mouse for itself: while it is in hand the canvas below draws
nothing, so a tap puts a box down instead of a dot of ink. Other tools leave text alone.

A box that is put down and never typed in is not a change: it is held aside until something is
typed, so that tapping about with the text tool does not fill the undo history with empty boxes.

## Consequences

- One face per box. Bolding a single word inside a box is not possible; the reader makes a second
  box. The room for it is left open: a later version can add runs of formatting beside the text
  without moving anything that is already kept.
- Text is always above ink. Boxes are drawn after the strokes rather than mixed in with them by the
  order they were made, which is what the applications this was measured against do as well.
- What is seen is what is printed, as far as the toolkit's own text engine allows: the same font,
  the same size in the units of the page and the same width to run in are used for both, so lines
  break in the same places.
- Line spacing is kept with the rest of the face but nothing sets it yet: the editor the window
  uses cannot show it, and half of a setting is worse than none.
- Handwriting turned into text lands in one of these boxes, so it can be corrected, moved and found
  like anything else that was typed.
