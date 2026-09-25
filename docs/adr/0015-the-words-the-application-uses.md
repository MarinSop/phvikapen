# 0015. The words the application uses

Date: 2026-09-25

## Status

Accepted

## Context

Every word a reader sees was written in English in the source, and shown exactly as written. The
application is used where English is not the first language, and the first language wanted is
Croatian. Writing the Croatian words into the sources beside the English ones would answer that
once and make the third language impossible: two strings in two places, and a third would be three.

## Decision

**Every word a reader sees is marked where it is written and translated where it is shown.** The
sources keep the English, because the people who write the code read English and a source that
reads as prose is worth keeping. Everything else is a catalogue beside the sources, named by the
language it holds.

**The toolkit's own machinery does the work.** The strings are already marked, the tool that
gathers them ships with the toolkit, and the catalogues are built into the application, so nothing
is brought in that is not already there. A catalogue is a file; adding a language is adding one.

**Nothing keeps a copy of a translated word.** A word is read where it is shown, so asking for
another language and being handed it are the same moment. The table of commands, which is built
once and kept, holds the words as they were written rather than as they read, and translates them
each time one is shown; a name translated once would have stayed that way whatever the reader
asked for afterwards.

**The language chosen is kept beside the theme.** Both say how the application looks and reads
rather than what it does, and both take hold at once. Asking for nothing at all follows the
machine, which is what a reader who never opens the settings gets.

## Consequences

- A third language is one catalogue and one line naming it. Nothing a reader sees is written down
  twice, and nothing has to be found and changed in the sources.
- Changing the language shows at once, everywhere, without restarting. What a reader has already
  written is their own and is left exactly as it was: only the words of the application change.
- A word added to the application and not added to a catalogue is shown in English. That is a gap
  a reader can see, and the tool that gathers the strings lists every one of them, so it is a gap
  that can be counted rather than guessed at.
- The toolkit's own words, in the dialogs it opens for choosing a file or a colour, are taken from
  where the toolkit keeps them. Where they were not shipped alongside the application, those
  dialogs stay in English while everything else does not.
- English needs no catalogue, because it is what the sources say. Choosing it puts nothing in
  place, which is also the fastest thing the application can do.
