# 0011. Pictures standing on a page

Date: 2026-09-25

## Status

Accepted

## Context

A page has held ink, typed text and, behind all of it, one imported document — a page of a PDF or
a picture the page was made from. What it could not hold is a picture put on the paper afterwards:
a photograph beside a note, a diagram to write over, a corner of a screen captured and dropped in.
Every application of this kind can do it, and the two things still to come, tables and equations,
need the same parts.

Three things had to be settled: what a picture on a page is, what it stands above and below, and
who draws it.

**What it is.** The picture itself could be kept with the page that shows it, or kept once for the
whole notebook and pointed at. A notebook already keeps whole documents that way, by what they
contain rather than by what they are called, so the same document imported twice costs the room of
one. A photograph dropped on ten pages should cost no more.

**Where it stands.** Typed text was settled the other way: it is always drawn over the ink,
because a box of words is a thing in its own right and writing over it would be an accident. A
picture is the opposite. A reader puts a picture down in order to write on it — to circle a part
of a diagram, to mark a photograph. A picture over the ink would make that impossible.

**Who draws it.** Typed text is drawn by the window toolkit in a layer over the canvas, because
the editor, the caret and the keyboard all come with it. A picture needs none of that, and it must
be under the ink, which rules the layer out: the canvas draws the ink, so the canvas must draw the
picture.

## Decision

A picture on a page names the picture it shows, where it stands, how large it is drawn and how far
it has been turned. What it is made of is kept once among the notebook's assets, by what it
contains, and several pictures on several pages may name the same one. Emptying the trash keeps an
asset that any page still names.

Pictures are drawn by the canvas, over the document a page was made from and under everything
written on it. They keep an order of their own among themselves, so that one dropped later stands
over one dropped earlier, but no picture ever stands over ink.

The canvas already drew one picture per page, the imported document, through a small pipeline of
its own. That pipeline now draws a list: the document first, then the pictures of that page. It
also learned to turn what it draws, which it did not need before, because an imported document is
never turned and a picture usually is.

Turning is part of the picture rather than of the thing it is made of, so the same photograph can
lie flat on one page and on its side on another without being kept twice.

What is printed or made into a small picture of the page is drawn again by the painter that
already draws the paper, the ink and the words, in the same order and turned the same way.

## Consequences

- A picture cannot be put over the ink. Someone who wants a picture to cover what is written has
  to rub the writing out, which is the trade the other way round from typed text, and is what the
  applications this was measured against do.
- Sizing a picture does not change what it is made of: the same picture is drawn larger or
  smaller. Made very large it will show its own grain, which is honest and is what a reader
  expects.
- A picture keeps the whole of what it was given. Cropping is not part of this and would be a
  rectangle kept beside the rest, added later without moving anything already kept.
- Several pictures on a page each cost a texture on the graphics card while that page is on the
  screen. A page with dozens of large photographs has not been measured.
- The turning is done where the picture is drawn rather than by turning what it is made of, so it
  costs nothing to change and loses nothing.
