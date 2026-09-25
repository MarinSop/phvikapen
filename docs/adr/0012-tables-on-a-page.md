# 0012. Tables standing on a page

Date: 2026-09-25

## Status

Accepted

## Context

A page can hold ink, typed text, an imported document and, since the last step, pictures. What it
cannot hold is a table: a timetable, a list of measurements, a comparison of two things side by
side. It is the thing readers of this kind of application ask for most after pictures, and it is
the last of the three larger pieces before equations.

A table is unlike everything the page holds so far, because it is not one thing but a grid of
things. That raises four questions that had to be settled before any of it could be written down.

**What a box holds.** A box could hold what a text box holds, with its own face, its own width and
its own place; or it could hold words alone and take its face from the table. The first makes every
box a text box and the table a mere arrangement of them, which is how a word processor does it. The
second makes the table one thing that happens to be divided, which is how a spreadsheet does it.
A reader putting a timetable on a page wants the timetable to look like one timetable, and wants to
change how all of it looks at once.

**Where the grid is kept.** The measures of a table — how wide each column runs, how tall each row
stands — are never asked a question about. Nothing searches them, nothing joins on them, nothing
looks at one of them alone. What is typed into the boxes is the opposite: a reader searches for it,
the way they search for everything else written in a notebook.

**Where it stands.** Typed text is drawn over the ink, because a box of words is a thing in its own
right. A picture is drawn under the ink, because a reader puts one down in order to write on it. A
table is both: it is a thing in its own right, and a reader draws a table on a page in order to
write in it with a pen.

**Whether boxes may be joined.** Joining two boxes into one is the next thing anyone asks for after
a table exists. It changes how a table is measured, how a tap finds the box under it and how the
keyboard walks from box to box.

## Decision

A table names where its top left corner is, how wide each of its columns runs, how tall each of its
rows stands, and what is typed in each of its boxes, kept row by row. One face is worn by the whole
table, and each box says only how its own words line up within it. A box that says nothing and asks
for nothing is not written down at all, so a wide table with a few words in it costs the room of
those words.

The measures are kept as they are measured, in one piece, because nothing ever asks a question
about them. What is typed into the boxes is kept box by box, because a reader searches for it, and
a search now finds words typed into a table exactly as it finds words typed anywhere else, showing
each one where its own box stands.

A table is drawn over the ink, as typed text is, but only its ruling is drawn: the boxes themselves
are empty, so handwriting inside a box stays where it was written and is seen through the table. A
reader may rule a table and then fill it in by hand, which is what a pen-first application is for.

Boxes may not be joined. A table is a plain grid, and the shape that is written down leaves room
for joining to be added later — a box saying how far it reaches across and down — without moving
anything already kept.

Rows and columns are added and taken away by making a new table from the old one rather than by
changing one in place, so that one step of undo puts back exactly what was there, the way every
other change to a page does. The last row and the last column cannot go: a table with nothing in it
is not a table.

## Consequences

- Every box of a table wears the same face. A reader who wants one word in bold inside a table
  cannot have it yet. The face is kept with the table, so a face per box would be a column added
  beside the words, not a change of shape.
- Nothing may be written down about a box that is empty and plain, so a table of a hundred empty
  boxes costs one row in the notebook. The measures say how many boxes there are, and a box that
  was never written down comes back empty.
- Because the measures are kept in one piece, no question can be asked of them in the notebook
  itself — how wide the third column of some table runs can only be answered by reading the table.
  Nothing asks such a question.
- A table cannot be turned, although a picture can. Turning a grid of type means turning the type
  in it, which is a different piece of work from turning a picture, and no application of this kind
  offers it.
- Joining boxes is left out. A reader who needs a heading across a table has to put a box of text
  above it instead.
- Sizing a table gives every column and every row the same share of the change, so a table dragged
  by its corner keeps its proportions. No column and no row is ever squeezed out of sight.
