# 0023. Keeping the words read out of a picture

Date: 2026-09-27

## Status

Accepted

Amends [0021](0021-reading-what-a-machine-carries.md).

## Context

[0021](0021-reading-what-a-machine-carries.md) decided to hold what was read out of a picture only
while the notebook was open. Two reasons were given: reading again is quick, and writing it down
would mean a shape in the file that has to be kept correct as pictures are replaced.

The first reason turned out to be the weaker one. Reading is quick only where the machine carries a
reader, and a reader who opens a notebook on a machine that carries none loses everything already
read. The second reason turned out to be answerable rather than true: the notebook already names a
picture by what it contains rather than by where it stands, so the words can be named the same way.

What that decision cost was the thing a reader most wants from reading a picture at all: searching
the notebook never reached the words in its pictures, however many had been read.

## Decision

**What was read out of a picture is written into the notebook.** It survives closing the notebook,
it survives on a machine that carries no reader, and a picture is read once rather than once a
sitting.

**The words are kept against the picture, not against where it stands.** A picture is already named
by what it contains, so the words are too. Moving the picture, drawing it larger, turning it or
putting the same picture on a second page all leave the words right, and replacing a picture leaves
the old words behind with the old picture rather than lying about the new one. The same picture on
ten pages is read once and found on all ten.

**Each run of words keeps its corners as shares of the picture.** Where it falls on a page is worked
out when it is searched for, from the picture it belongs to. That is the same reason the corners are
kept that way by the reader, and it is what makes them survive the picture being moved or resized.

**Searching a notebook reaches them.** The words are folded the way typed words and handwriting
already are, so one search reaches all four and a hit in a picture points at the part of it the
words are on.

**Words with no picture behind them are swept out with the pictures.** Emptying the trash already
drops the assets nothing points at; it now drops the words nothing points at with them.

## Consequences

- Searching a notebook reaches the words in its pictures, and a picture read on one machine is
  searchable on a machine that could never have read it.
- A picture that was read before this change is read again the first time it is asked for, and
  written down then.
- Nothing reads the pictures already sitting on the pages. Until a picture is asked about, it holds
  no words, so a notebook is only as searchable as the reading that has been asked for.
- The notebook grows by the words read out of its pictures, which is small beside the pictures.
- The words cannot yet be picked out on the picture itself. The corners are kept, so that is one
  layer away rather than a change to how anything is stored.
