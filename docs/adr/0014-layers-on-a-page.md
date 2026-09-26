# 0014. Layers on a page

Date: 2026-09-25

## Status

Accepted

## Context

A page carries ink, typed text, pictures and tables. Until now the order they were drawn in was
fixed and built into the application: the document a page was made from at the bottom, then
pictures, then the ink written over them, then tables and boxes of type on top. Nothing on a page
could be put in front of or behind anything else, and nothing could be set aside for a moment.

That order is a good guess and a bad rule. A picture dropped onto a page always went under the
writing, which is right for tracing and wrong for a diagram meant to cover a draft. A table always
stood over everything, which is right for a form and wrong for a table that a reader wants to write
over. Nothing could be hidden while working on what lay beneath it, and nothing could be pinned
down so that a hand resting on it could not drag it away.

Every drawing application answers this the same way, and readers already know the answer: layers.

## Decision

**A page holds layers, and everything on the page stands on exactly one of them.** A layer has a
name, a place in the order, and two switches: whether it is shown and whether it is locked. The
first layer is the bottom of the pile and the last is the top.

**A page always has at least one layer.** The page itself keeps that promise, whichever way it is
made: read from a file, copied from another page, or made empty for a page that is being added. A
page written down before there were layers has none written against it, and it makes itself one
when it is read; everything on it names no layer at all, which is read as the bottom one. Nothing
has to be rewritten, and a notebook made by an older version opens with all of it on a single
layer, looking exactly as it did. Because the promise is the page's own, no caller can hand out a
page with nowhere to write.

**A layer that is not shown is drawn nowhere**: not on the paper, not in what is printed, not in
the small picture of the page. **A layer that is locked is drawn but cannot be touched**: nothing
on it answers a tap, so it cannot be picked up, moved or taken away by accident.

**Changing the layers is one kind of change.** Adding a layer, taking one away, renaming one,
hiding one, locking one and reordering them are all the same step: the list of layers as it was
becomes the list as it is to be. There is therefore one thing to get right and one thing to take
back, and every one of them undoes exactly. Taking a layer away takes everything standing on it in
the same step, so one undo brings the layer and its whole contents back together.

**The order is the layers, not the objects.** Within one layer the kinds keep the order that suits
writing by hand: a picture stands under the ink written over it, and a table or a box of type over
both. Putting one thing in front of another is done by putting it on a layer above, which is how
every layered editor works and what keeps the model small: one number per thing rather than a
page-wide ordering that every insertion has to renumber.

## Consequences

- A picture can be put over ink or under it, a table over a picture or under it, and a page can be
  traced over a locked drawing. What is printed and exported follows the layers exactly, layer by
  layer from the bottom up.
- A notebook made before this opens unchanged, and one made after this cannot be opened by an older
  version, which is what the version stamped on a notebook is for.
- Two things on the same layer cannot be reordered against each other. A reader who wants that adds
  a layer, which is the answer every layered editor gives.
- **What is typed still stands over what is drawn, on the glass.** The ink is drawn by the canvas,
  which is one piece of the window, and tables and boxes of type are drawn by the window over it.
  The order of the layers is therefore honoured among pictures and ink, among tables, and among
  boxes of type, and honoured in full in what is printed — but on the glass a table cannot be shown
  underneath ink. Removing that difference means drawing all four kinds through one renderer, which
  is a larger change than the layers themselves and is not taken on here.
- Whether a thing may be taken hold of is one question with one answer, asked of the page. Marking
  out ink with the loop, rubbing ink out, and tapping a picture, a table or a box of type all ask
  it, so no way of reaching for a thing goes round a layer that is hidden or locked. Hiding or
  locking a layer also lets go of whatever on it was already in hand.
- Nothing new is put on a layer that is hidden or locked. The reader is told why rather than having
  the work put somewhere they did not choose.
