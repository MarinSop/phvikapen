# 0009. One command, whichever way it is asked for

Date: 2026-09-24

## Status

Accepted

## Context

A command of this application could be asked for in four ways: from a text menu across the top,
from a button on a bar, from the keys, and — from now on — from a small menu that opens under the
pointer. Each of those was free to decide for itself what the command was called, whether it could
be used at that moment, and which keys it answered to.

The keys in particular were kept in two places. A list in the application named every command and
the keys it came with, so that the settings could show them and let them be changed; and every
command in the window then repeated the keys it came with a second time, as plain text beside the
command. The two lists were free to disagree, and they did: two commands had been given the same
keys, which the toolkit answers by refusing both.

Worse, what was written beside a command was the text a **reader** should see, asked of the
platform: on a Mac that is `⌘Z`, with the symbol for the key rather than its name. That text was
then handed straight back to the toolkit as the keys the command should answer to. Nothing can read
it back, so it parsed to nothing, and undo, redo, copy, paste, cut, save, close, zoom and page
turning answered to no key at all on that platform. They worked from the menus, which is how it
went unnoticed.

Two more things were wrong with keys. A command whose key is a plain letter — the tools are `V`,
`P`, `E`, `T` and so on — took that letter even while words were being typed into a box on the
page, so typing `text` picked up three different tools. And the key that removes what is picked
answers only to the one name the platform gives it, while a Mac keyboard sends another.

## Decision

Every command is named once, in one list, with what it is called, which group it belongs to, and
the keys it comes with. Where a command follows a convention that differs from one platform to the
next — undo, redo, copy, save, find, the page keys — the list names the convention rather than the
keys, and the keys are asked of the toolkit at the moment they are needed.

The keys a command answers to are decided in exactly one place, which is asked by the menus, the
palette, the bars and the keyboard alike: what the reader chose in the settings, or else what the
command came with. Nothing in the window writes a key out by hand any more.

What is **shown** beside a command and what a command **answers to** are two different things and
are asked for separately. What is shown is written the way the platform writes it, and is never
read back.

A command whose keys belong to the page — the tools, undo, cut, copy, paste, delete, and the rest
of what acts on what is drawn — gives its keys up entirely while words are being typed, so that
the editor with the keyboard keeps every letter, and its own undo, its own copy and its own
selection behave as they do anywhere else. A command that belongs to the window — save, new,
close, zoom, turning the page — keeps its keys throughout.

The menu that opens under the pointer is built from the same command objects as everything else.
It shows what suits what is picked up: what can be done to a piece of drawing, what can be done to
a box of words, or what can be done to the page itself.

That menu is asked for in the canvas rather than by a handler laid over it: a right click, the
button on the barrel of a pen, or holding the pen still for a moment. Holding still is a setting,
because it costs a slow, deliberate dot; the other two are always there. Doing this inside the
canvas means the menu cannot come between the pen and the paper, which is the one thing in this
application that must never be made worse.

## Consequences

- Adding a command is one entry in one list plus one object in the window, and it appears with its
  keys everywhere at once, the settings included.
- Two commands cannot quietly be given the same keys: a reader is told of a clash when changing
  one, and the list that comes with the application is checked by a test.
- The keys are held in the portable spelling throughout, which the toolkit reads back exactly and
  which already means the right key on each platform — the same entry is `Ctrl` on Windows and the
  command key on a Mac.
- While words are being typed, the page's keys do nothing. That is the point, but it does mean
  that undo inside a box of words undoes the typing rather than the last change to the page, which
  is what every other application does and what a reader expects.
- The menu under the pointer costs a held press. A slow, deliberate dot with the pen opens the
  menu instead of leaving a mark; the setting turns that off for anyone who draws that way.
