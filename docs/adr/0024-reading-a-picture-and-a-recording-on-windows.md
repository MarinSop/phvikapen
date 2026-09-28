# 0024. Reading a picture and a recording on Windows

Date: 2026-09-28

## Status

Accepted

Extends [0021](0021-reading-what-a-machine-carries.md) and
[0022](0022-reading-a-recording-back-as-words.md).

## Context

Windows on ARM is the machine this application is for. macOS is where it is written. That order had
been reversed for two features: [0022](0022-reading-a-recording-back-as-words.md) gave macOS a
speech reader and left Windows refusing, and the picture reader behind
[0021](0021-reading-what-a-machine-carries.md) was in the same state. A reader who buys the
application for Windows cannot read a photograph or a recording, and a reader who happens to
develop it on a Mac can. That is the wrong way round, and nothing about it was a decision — it was
only where the work had reached.

Windows carries two readers of its own, and they are not the same shape as each other.

The picture reader is straightforward. `Windows.Media.Ocr` is part of the system, reaches it through
the projection already used for handwriting, reads whatever the machine can decode, and hands back
lines of words with a rectangle around each. It needs no permission from the person and no network.

The speech reader is not straightforward. The modern speech recognizer Windows carries reads from a
microphone and from nothing else: there is no way to hand it a recording made an hour ago. The older
one — the Speech API the system has carried since long before, reached through `sapi.lib` — does read
from a file, offline, and gives a moment for every word. It is the only reader on the machine that
can read a recording back, so the choice is that reader or no reading at all.

There is a second obstacle. A recording is kept as compressed sound, the same on every machine, and
the Windows reader takes plain samples and nothing else.

## Decision

**Windows reads a picture with `Windows.Media.Ocr`.** One file behind the interface 0021 named,
reached through the same projection the handwriting reader already uses, so no new dependency. A
picture larger than the reader accepts is made smaller before it is read; the corners come back as
shares of the whole picture, so nothing above the platform layer can tell which machine read it.

**Windows reads a recording with the Speech API the system carries, named by `sapi.lib`.** It is
part of Windows rather than something fetched, it reads a file rather than a microphone, it reads
without a network, and it gives a moment for every word. The modern recognizer cannot read a
recording at all, so the older one is not a compromise between two readers — it is the only one.

**The recording is turned back into plain samples before the reader sees it.** Qt decodes it to
sixteen-thousand samples a second, one channel, whole numbers, which is the one shape every
recognizer Windows carries is sure to read, and those samples are written out as a plain sound file
for the reader to open. The file is removed as soon as the reading ends, whichever way it ends.

**Words are gathered into lines by the same code macOS uses.** The gathering lives in `core`, so
both readers hand up words and both get the same lines out. A transcript made on Windows reads the
same shape as one made on macOS.

**Both readers read away from the thread that asked, and answer on it.** Reading a photograph or a
recording takes long enough that the window must not wait, and the callers above already expect an
answer that arrives later, because that is how the macOS speech reader already behaves.

**Where the machine carries no reader for the language asked for, it says so.** A machine without a
speech pack has no recognizer to give; the reading fails with that reason rather than coming back
empty, which is what the panels already show.

## Consequences

- Windows reads the words in a picture and reads a recording back as words. Nothing above the
  platform layer changed to make that true, which is what the interface in 0021 was for.
- `sapi.lib` is linked on Windows. It is a part of the system rather than a fetched dependency, so
  there is nothing to pin and nothing to build.
- The speech reader is an old one. It reads dictation with the packs the machine has, and it is
  poorer at that than the reader macOS carries. A machine with no speech pack reads nothing, and
  says so.
- What is read still depends on the machine. Two machines can make two different transcripts of the
  same recording, and two different readings of the same picture. That is the price 0021 accepted,
  and it is now paid on both machines rather than on one.
- A recording is decoded in full before it is read, so a long recording is held in memory twice
  over: once compressed, once as samples.
- Neither reader is exercised by the test suite against real sound or a real photograph. The build
  is proven on Windows on ARM by the pipeline; what the readers make of real material is tried by
  hand, as it already is on macOS.
