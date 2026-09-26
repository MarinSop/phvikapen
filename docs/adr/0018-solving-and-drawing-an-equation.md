# 0018. Solving an equation, and drawing it

Date: 2026-09-26

## Status

Accepted

## Context

A sum written on one line is read, worked out and answered. That was written down in
[0013](0013-equations-and-their-answers.md), and it stops where a student starts: at a letter. The
decision taken then said so plainly — "an equation with an unknown to be solved for, which is what
a student most wants, is refused plainly rather than half answered" — and left the door open:
"solving for an unknown works on the same structure and can be added without moving anything
already kept."

Three things were wanted on top of it: an equation solved for a letter, the steps that reached the
answer, and a curve drawn from a function. A fourth, smaller one comes with them: a sum typed with
an equals sign at the end, worked out where it stands, the way a calculator would.

## Decision

**A letter is a third kind of step, and nothing else about the structure moves.** An equation was
numbers and what is done to them; it is now numbers, letters and what is done to them. Everything
already written against that structure — working a sum out, drawing it the way it is written, the
box of type that holds it — reads the new kind or refuses it, and none of it had to be rewritten.

**A line with an equals sign in it is two equations, not one.** Reading it hands back both sides
separately. A line with no equals sign is all left side, so every sum that was read before is read
the same way now.

**An equation is solved by seeing it as a run of powers of one letter.** Everything on both sides
is gathered into coefficients, lowest power first, and the two sides are brought together. What is
left says what to do: nothing at all means the statement holds for every number; a number alone
means it holds for none; one power means dividing; two means the formula for a square. Anything
higher is refused plainly, as is a letter under a root, a letter in a divisor and a letter raised to
a letter, because the gathering cannot undo those.

This is chosen over rearranging the equation symbolically. Gathering into powers is a page of code
with an exact answer for the shapes a reader actually writes; rearranging is a small algebra system,
which is the thing a dependency would be bought for. Where the gathering refuses, it says which part
it could not take, rather than handing back something it half understood.

**The working is named, not written out.** Each step says why it was taken — everything gathered to
one side, both sides divided, the formula used, the answer reached — and carries the statement as it
stood at that point. The window says the reason in the language the reader asked for. A step written
out in English in the core would be a step no translation could reach.

**A curve is drawn by solving, once for every place across it.** For each value across the graph the
other letter is substituted and the statement solved for the one that runs up. A straight line, a
square, a circle and a curve that breaks are all drawn by the same few lines, because they are all
the same question asked many times. A curve with two answers at a place — a circle has a top and a
bottom — is kept as two runs, so that the halves are never joined by a line that is not there, and a
run breaks wherever there is no answer or the answer climbs out of sight.

**A statement with two letters in it is drawn rather than answered.** There is no one number it
comes to, and saying so as a refusal would be wrong: what the reader wrote is a curve, and the curve
is what they get.

**A sum typed with an equals sign at the end is worked out where it stands.** It is only taken as a
sum when it holds a sign of arithmetic, holds no letters and can be read whole; ordinary writing
that happens to end in an equals sign is left alone. The answer is put after the sign, so typing can
go on, and the sign is no longer at the end, so it is never worked out twice. The reader can switch
it off.

## Consequences

- An equation with one letter in it, up to a square, is solved with the steps that reached the
  answer, and nothing new was brought in.
- A curve is drawn for anything that can be solved for one letter at each place across it, which
  covers straight lines, squares, circles and curves that break. It is worked out no more finely
  than the graph is wide, so a narrow graph costs less.
- A power higher than a square, a letter under a root and a letter in a divisor are refused, each
  saying which part it could not take. A system of two equations is not solved.
- The graph is drawn in the window rather than on the page. Putting a curve on a page the way a sum
  goes on a page is a drawing of its own and is not done yet.
- Reading a letter as a letter is the opposite of what handwriting needs, where a letter x is a
  times sign. The two stay apart: handwriting is put right before it is read, typed writing is not,
  so a sum written by hand still cannot hold an unknown.
- The maths panel is a panel like any other, so it docks, stacks, floats and is remembered with the
  rest of them.
