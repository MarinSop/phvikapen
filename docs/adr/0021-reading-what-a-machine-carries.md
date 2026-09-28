# 0021. Reading what a machine carries: speech and the words in a picture

Date: 2026-09-26

## Status

Accepted

Extended by [0022](0022-reading-a-recording-back-as-words.md), which fills in the reader for
recordings on macOS, and by
[0024](0024-reading-a-picture-and-a-recording-on-windows.md), which fills in both readers on
Windows.

Amended by [0023](0023-keeping-the-words-read-out-of-a-picture.md), which writes what was read out
of a picture into the notebook after all, reversing the decision below to hold it only in memory.

## Context

Three kinds of reading are wanted, and they are the same kind of problem three times: reading
handwriting back as words, reading a recording back as words, and reading the words in a picture.

Handwriting was answered in [0007](0007-handwriting-recognition.md): a name for the job, an
implementation per machine behind it, and a plain refusal where the machine carries none. That
decision has held, and the two new readings are answered the same way rather than differently.

Buying a recognizer for each would mean three new dependencies, three sets of model files to ship
or download, and three things to keep current. Every machine the application runs on already
carries readers for all three.

## Decision

**Each reading is named once, and the machine answers it.** Speech and pictures each get an
interface beside the one handwriting already has, an implementation per machine behind it, and
nothing at all where the machine carries no reader. The window looks the same everywhere; where
there is no reader it says so plainly rather than coming back empty.

**Reading never blocks.** Both hand their answer back through a callback rather than returning it,
because a recording is minutes long and a picture can be large, and nothing may wait for either.

**What a reading produces is kept beside what it was read from, or not kept at all, and the
difference is deliberate.** What was said in a recording is written into the notebook: it is long,
it costs real time to produce, and a reader will want it again. What was read out of a picture is
held only while the notebook is open: it is quick to produce again, it is made from the picture
rather than written by the reader, and writing it would mean a shape for it in the file that would
have to be kept correct as pictures are replaced.

**The words in a picture are read on macOS today, by the reader the platform carries.** It is given
the bytes of the file rather than a decoded picture, so whatever the machine can open, it can read.
Where the reader is unsure of a run of shapes it is left out rather than guessed at.

**Nothing is written onto the picture.** What comes back is words to be copied or put on the page,
and the picture stands as it was.

## Consequences

- A picture is read on macOS. Windows carries a reader of its own, and filling that in is one file
  against this interface; until it is there, Windows says so plainly.
- A recording is read nowhere yet. The whole path is there — asking, the state while it runs, what
  comes back, keeping it, tapping a line to move the playing — and only the reader is missing.
- No new dependency, no model files to ship, and nothing to keep current.
- A reading that is only as good as the machine it runs on will differ between machines. That is
  the price of not carrying a recognizer, and it is the same price handwriting already pays.
- What was read out of a picture is worked out again after the notebook is closed. A reader who
  reads a large picture twice in two sittings waits twice.
