# 0019. Recording what is said, and tying it to the page

Date: 2026-09-26

## Status

Accepted

## Context

A page holds ink, type, pictures, tables and an imported document. What it cannot hold is the other
half of a lecture or a meeting: what was said while the notes were being written. A reader who
writes three words during a fast explanation wants those three words to take them back to the
minute they were written in.

Two things are needed, and they are different problems: making and playing a recording, which is a
machine question, and tying what is on the page to a moment in it, which is a question about the
document.

**Making a recording needs a dependency.** Reading a microphone, encoding what comes off it and
playing it back again are not things to write by hand. Qt carries a module for it, `QtMultimedia`,
which is part of the toolkit the application is already built on, under the same licence, released
on the same day, and present on every machine the application ships to. It is not a new party to
trust; it is a part of one already trusted.

The alternative considered was a small audio library with a codec beside it. That is two new
parties, two new build steps and two new sets of platform code, to reach the place `QtMultimedia`
already stands. It was refused.

## Decision

**A recording is a thing the page holds, and the sound is an asset.** The bytes go where every
other run of bytes goes: into the assets of the notebook, named by what they contain, so the same
recording on two pages costs the room of one and nothing has to learn a new way of keeping bytes.
What the page keeps is the name, how long it runs, when it was made, and what was said in it.

**The recording is kept as one file in one format, and that format is chosen once.** AAC in an MPEG
4 container, because it is the one format every machine the application runs on can both write and
read without anything being installed. A recording made on one machine plays on another.

**A tie between the page and a recording names the thing, not the place and not the words.** A mark
holds which recording, which thing on the page, and how far into the recording that thing was put
there. The thing is named by its own identifier, which is the one part of it that never changes:
moving a box of type, correcting what it says, restyling it or putting it on another layer all
leave the mark pointing at the same box. This is the whole reason the tie is a mark rather than a
number written into the text.

The cost is that the tie is as fine as a thing on the page, not as fine as a word. A box of type
typed over four minutes carries one moment, the moment it was begun. Marking every word would mean
holding a place inside the words, which is exactly the thing that does not survive editing.

**Marks are made while the recording runs and never afterwards.** Anything put on the page while a
recording is going — a stroke, a box of type, a picture, a table — is tied to the moment it was put
there. Nothing is guessed at later.

**What was said is read by whatever the machine carries, exactly as handwriting is.** Reading speech
is the same shape of problem as reading handwriting, and it is answered the same way: a name for the
job, an implementation per machine behind it, and a plain refusal where the machine has none. No
new dependency is taken for it, and the recording is kept whether or not it was ever read.

**Reading is kept beside the recording, in runs with the moments they cover.** A run is a piece of
what was said, the moment it starts and the moment it ends, so that tapping a line of what was said
moves the playing to it and playing marks the line being heard. That is the same relationship a
mark has, so the panel says one thing twice rather than two things once.

## Consequences

- A recording is made, paused, carried on, stopped, named, played, moved through and taken away,
  and it is there again the next time the notebook is opened.
- Anything written while a recording runs is tied to the moment it was written, and tapping it
  moves the playing there. Editing it afterwards does not break the tie.
- A machine with no microphone, or one where the reader has not allowed listening, says so plainly
  and the rest of the application goes on working.
- The sound goes into the notebook file. A long recording makes a large notebook, and nothing
  prunes it; taking the recording away is the only way to get the room back.
- A recording cannot yet be trimmed, joined to another, or exported on its own.
- What was said is read where the machine can read it and refused plainly where it cannot, so the
  panel looks the same everywhere and says what it could not do.
