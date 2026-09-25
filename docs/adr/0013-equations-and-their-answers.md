# 0013. Equations, and answering them

Date: 2026-09-25

## Status

Accepted

## Context

A page can hold ink, typed text, an imported document, pictures and tables. The last thing asked
for is arithmetic: write a sum by hand, ask for the answer, and have it appear on the page. It is
the one thing a reader cannot do with paper, and the reason many people keep a calculator beside
their notes.

Four things stand between a stroke of ink and an answer, and they are four different problems:

1. Reading the shapes back as characters.
2. Understanding what those characters mean as arithmetic.
3. Working the arithmetic out.
4. Putting the answer back on the page.

The temptation is to buy one library that claims to do all four. That would tie the whole feature
to whatever that library understands, on whatever machines it runs on, and would make each of the
four impossible to change without changing the others. It is also the most expensive thing to
undo.

**What already exists.** Handwriting is already read on Windows, by the reader the platform
carries, and comes back as words with the place they sit in. `12 + 7` written by hand comes back
as the characters `12 + 7`. That reader knows nothing about arithmetic, and it reads one line at a
time, but for a sum written on one line it is enough, and it costs nothing new.

**What it cannot do.** It reads a line of characters, not a two-dimensional arrangement. A
fraction written with one number above another, a power written small and raised, a root drawn
over what it covers, a matrix — none of these are a line, and no reader that reads lines will ever
make sense of them.

**What a reader of handwriting hands back is not quite arithmetic.** A one is read as a letter l,
a nought as the letter O, a times sign as the letter x. In much of the world, and in the place
this application is written, a decimal point is written as a comma and division as a colon. These
are not faults in the reader; they are what those shapes are.

## Decision

The four problems are kept apart, and each link of the chain is named so that any one of them can
be replaced without touching the others:

```
strokes  →  characters  →  a structure  →  an answer  →  type on the page
          (a reader)      (our reading)   (our working)
```

**The structure is ours, and it is the contract.** An equation is kept as numbers and what is done
to them, flat, without pointers, so that it can be copied, compared and carried across the layers
like every other value in the core. Nothing else in the chain is allowed to become the contract:
not the characters, which differ from reader to reader, and not the answer, which says nothing
about how it was reached.

**Reading characters into that structure is ours, and takes no dependency.** Arithmetic written on
one line is a small, well understood grammar, and writing it is a day's work against a lifetime of
being tied to someone else's idea of what an equation is. It understands the four operations,
powers, roots, brackets, a sign in front of a number, and a number standing against a bracket
meaning multiplication, because that is how people write.

**What a reader of handwriting hands back is put right first, as a step of its own.** A letter l
becomes a one, a letter O a nought, a letter x a times sign, a colon a division, a comma a decimal
point. This step is named and tested on its own, so that what is being guessed at is written down
rather than hidden inside the reading.

**Working the arithmetic out is ours, and takes no dependency.** The numbers are the ones the
machine already counts with. An answer that cannot be reached — divided by nothing, a root of
something less than nothing, a number too large to hold — is refused plainly rather than handed
back as nonsense.

**Recognition stays behind the reader that is already there.** No new dependency is taken for it.
The reader on Windows hands back characters, and everywhere else there is no reader, and the
application says so, exactly as it already does for finding handwriting.

## Consequences

- A sum written on one line is read, worked out and answered with nothing new brought in, on the
  machines that can read handwriting at all. Everywhere else, typed arithmetic still works, because
  the chain after the reader does not care where the characters came from.
- A fraction written as one number over another, a raised power, a root drawn over what it covers
  and anything else arranged in two dimensions will not be read. That needs a recognizer built for
  mathematics, which is a dependency, and taking one will need a decision of its own written down
  here. Nothing in the chain has to move when it comes: it replaces the first link only.
- Putting right what a reader hands back is guessing, and it can guess wrong. A letter x is always
  read as a times sign, so an equation with an unknown in it cannot be written that way. This is
  the price of arithmetic being the common case, and it is why the step is named and tested rather
  than buried.
- A comma is always a decimal point, so a thousands separator written as a comma will be read as a
  decimal point. Writing large numbers with a comma in them is the rarer thing in notes, and a
  space between the groups is read correctly.
- Only arithmetic is answered. An equation with an unknown to be solved for, which is what a
  student most wants, is refused plainly rather than half answered. Solving for an unknown works on
  the same structure and can be added without moving anything already kept.
- The answer goes back on the page as type, like everything else read out of handwriting, so it can
  be corrected, moved and taken away, and one step of undo puts the handwriting back.

## What was added later: drawing a sum the way it is written

A sum typed on one line reads badly as one line. `1/2` is not how anyone writes a half, and `2^10`
is not how anyone writes a power. The structure was already there, so what was missing was only the
drawing of it.

A fourth link was added to the chain, after the structure and beside the working out:

```
strokes  →  characters  →  a structure  →  an answer
                                 ↓
                            a drawing
```

**Laying a sum out is ours, and takes no dependency.** Division becomes a fraction, one part over
the other with a bar between; a power is set smaller and raised to the shoulder of what it stands
on; a root is drawn under a roof that runs the length of what it covers. Brackets are put back
wherever the shape needs them, whatever the reader typed, and they grow with what they hold.

**How wide type runs is asked for rather than worked out.** Only a window knows how wide a letter
is, and the core must not know what a window is. The laying out therefore takes a way of measuring
as an argument. The window hands it one that asks the font; the tests hand it one that reckons every
letter half as wide as the type is tall, so that what is laid out can be checked without a window
at all.

**A box of type says whether it holds a sum.** A sum is not a document object of its own: it is a
box of type that is drawn differently. It is moved, sized, taken away, printed and shown in the
small picture of the page like any other box, and one flag beside the words is the whole of what
had to be written down. While it is being typed in, what was typed is shown, so that it can be
corrected; the moment it is let go of, it is drawn.

Where what is typed is not arithmetic — half a sum, or a shopping list — nothing is laid out and
the words are shown as they were typed. Nothing is ever left blank.
