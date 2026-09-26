# 0022. Reading a recording back as words on macOS

Date: 2026-09-26

## Status

Accepted

Extends [0021](0021-reading-what-a-machine-carries.md).

## Context

[0021](0021-reading-what-a-machine-carries.md) named the job of reading a recording back as words
and left the reader itself empty: every machine refused. Everything around it was already there —
asking for a reading, the state while it runs, what comes back, keeping it in the notebook, tapping
a line to move the playing — so what was missing was one file.

macOS carries a reader in its Speech framework. It reads a whole recording from a file, hands back
one run per word, and gives each word the moment it was said and how long it lasted. That is what a
transcript that can be tapped through needs, and it is exactly what nothing else in the application
could produce.

Two things stand in the way of using it. The reader needs the person's permission, and macOS will
stop the whole application rather than the reading where the bundle does not say why the permission
is wanted. The runs it hands back are one word each, which is right for moving through a recording
and far too fine to read.

## Decision

**macOS reads a recording with its own reader.** One file behind the interface 0021 named. Nothing
above it changes, and no machine that has no reader behaves differently than before.

**The reading is done on the machine where the machine can do it.** Where the reader says it reads
without sending anything away, it is told to read that way. A notebook is private, and a recording
made beside it is no less private; sending one to a company's machine to be read is not something
to do quietly on the reader's behalf. Where a language can only be read away from the machine, the
reader falls back to that, because refusing outright would leave whole languages unreadable.

**The bundle says why the microphone and the reader are wanted.** Both lines are written into the
application's own property list rather than left to the default one. Without them the microphone
was never reachable either, so this fixes recording on macOS as much as it enables reading.

**Where a build cannot carry those lines, the reader refuses before asking.** A test binary is not
a bundle. Asking for permission from one would stop it dead, so the reader looks for the line first
and refuses plainly where it is missing. That keeps every machine's tests honest and identical.

**Words are gathered into lines in `core`, not in the reader.** A silence, a full stop, or a line
that has grown too long ends a line; a line begins the moment its first word was said. This is
plain logic over plain data, so it lives where it can be tested without a machine that reads
speech, and any other reader added later gathers its words the same way.

## Consequences

- A recording is read back as words on macOS, with a moment against every line, and tapping a line
  moves the playing to it.
- Windows reads nothing yet. It carries a reader of its own, and filling that in is one file
  against the same interface.
- The first reading asks the person for permission. Refusing it is answered with a plain reason and
  where to change it, not with an empty transcript.
- What is read depends on the machine and on its language support, so two machines can make two
  different transcripts of the same recording. That is the price 0021 already accepted.
- A recording is written to a temporary file before it is read, because the reader takes a file.
  The file is removed as soon as the reading ends, whichever way it ends.
- The lines are as fine as a pause. A word cannot be tapped on its own, only the line it sits in.
