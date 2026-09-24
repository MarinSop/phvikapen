# 0007. Windows Ink for reading handwriting

Date: 2026-09-20

## Status

Accepted

## Context

A notebook is only as useful as what can be found in it. The application has to find a word in
pages of handwriting, and hand a chosen piece of handwriting over as text.

What is written is already kept as strokes: the path of the pen with its pressure and its order in
time. Reading that is a different and easier problem than reading a picture of writing, and the
engines that do it are not the same ones that do optical character recognition.

Three ways to read it were considered.

- **Windows Ink.** `Windows.UI.Input.Inking.Analysis.InkAnalyzer` reads strokes into words, lines
  and paragraphs, and says which strokes each word was written with. It is part of Windows, costs
  nothing, runs on the machine without a network, and is already reachable: the build links the
  Windows Runtime projection for C++ for the native ink backend. It reads the languages whose
  handwriting is installed on the machine, and Croatian is among the languages Microsoft lists for
  ink to text. It exists only on Windows.
- **MyScript iink.** The best reader of joined-up handwriting, and it carries its own languages, so
  nothing has to be installed. It is paid for per application and per year, and it is a third-party
  library in the middle of the most private part of the application.
- **A model of our own.** An open model run through ONNX Runtime. Free and the same everywhere, but
  the Croatian letters with diacritics are exactly where the freely available models are weakest,
  and training one is a project of its own.

Whatever reads the handwriting, the words have to be kept somewhere: reading every page again to
answer every search would be far too slow for a notebook of any size.

## Decision

Read handwriting with Windows Ink, behind an interface in the platform layer, the way the ink
backend and the PDF engine are. Where the machine has no reader, which is every machine that is not
Windows, the application says so instead of pretending to search.

The words a page was read into are kept in the notebook itself, beside the strokes they came from,
together with the revision of the ink they were read from. A page is read again when its ink has
moved on from what was read, so the words follow what is on the page and a notebook can be searched
the moment it is opened, without reading anything again.

Searching is over the kept words. A word is stored twice: as it was read, and folded down to plain
letters without case or diacritics, so that searching for `ceskoslovacka` finds `Čehoslovačka`.

## Consequences

- Nothing about reading handwriting can be tried on the development machine. The interface, the
  words, the folding and the searching are pure and tested there; only the reader itself is
  Windows, and only the target device can show what it makes of real handwriting.
- The quality of what is read is Microsoft's, not ours, and it depends on the handwriting package
  the reader installed. The application has to say which languages the machine can read, and say
  plainly when the language of the writing is not one of them.
- A notebook carries its own index, so it grows a little, and a notebook copied to another machine
  arrives searchable.
- MyScript stays open as a later choice: it would be another implementation of the same interface,
  and the words it reads would be kept the same way.
