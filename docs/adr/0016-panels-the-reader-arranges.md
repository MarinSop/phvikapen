# 0016. Panels the reader arranges

Date: 2026-09-26

## Status

Accepted

## Context

The window put every panel where the sources said it went: a list of sections and pages on the
left, the page setup and the layers on the right, each one a switch in the View menu and a width in
the settings. Adding a panel meant a new switch, a new width, a new line in the menu and a new
child of the one split view that held them all. Nothing could be moved, nothing could share a place
with anything else, and a reader who wanted the layers on the left could not have them there.

The page setup was not a panel at all. It was a dialog, and a second copy of the same controls
stood in a panel of its own, so the same settings were written twice and shown two ways.

## Decision

**The layout of the window is a thing in its own right, kept apart from what a notebook holds.**
It is a tree. A branch is a split that runs across or down; a leaf is either a group of panels, one
of them in front, or the sheet itself, of which there is exactly one. The window has no fixed sides
and no fixed number of places: a panel goes wherever the tree can be cut. It is written beside the
other settings, not into the notebook, and it never enters the undo history: undoing a stroke must
not move a panel.

**A panel is named by an identifier and nothing else.** The model knows the identifiers; the window
knows what each one is called and what it shows. That keeps the names translatable, because a name
is read where it is shown rather than kept in a table that was filled once.

**Adding a panel is two lines.** One names it in the model, with whether it is open to begin with;
one says what it shows. The sections and the pages are two panels rather than one, because there
was no reason for them to be joined once anything could be put anywhere. Everything else — the
header, the tab, dragging, docking, stacking, resizing, closing, restoring and remembering — is
already there and does not know which panels exist.

**A panel is carried by its header, and the line where it would land is drawn before it lands.**
Whatever the pointer is over answers for itself, and there are three answers. Near an edge of it,
the panel is put on that side and the line is drawn along that edge. Over its tabs, the panel joins
them and the line is the caret between two tabs. Anywhere else in it, the panel is put underneath
and the line is drawn across the middle. The sheet answers the same way as a panel, so a panel can
be put against the sheet, and against the tool palette by way of the leaf beside it.

The one with the strongest claim wins, and the claim is settled afresh for every position of the
pointer, so the order the parts answer in does not change the answer. A target for putting the
panel away comes down from the top and outranks them all.

**A panel that is put away is not forgotten.** Where it stood is remembered, and opening it again
from the View menu puts it back there, in the group it was in if that group is still standing.

**Carrying a row is one piece of work, written once.** The sections, the pages and the layers are
all lists a row can be carried up and down in. They share the handler that decides which gap a
carried row would drop into, where it lands, and how the line marking the gap is drawn.

**Every motion is timed from one place.** The window has three durations and two easings, and one
switch that sets them all to nothing for a reader who asked for less movement.

## Consequences

- The reader arranges the window. Panels dock to any side, stack into tabs, resize, close and come
  back where they were, and the arrangement is there again the next time.
- The page setup is a panel like any other, opened from the page it belongs to or from the View
  menu, and shut by default. The dialog and its duplicate panel are gone.
- A future panel joins the system by existing. It is dockable, stackable and restorable on the day
  it is written, and nothing has to be taught about it.
- Panel layout is not undoable. That is deliberate: it is how the window is arranged, not what the
  document says.
- A group with several tabs in a narrow window shows the tabs elided and scrolls to the one in
  front. That is the cost of letting any panel stand anywhere.
- Taking a panel out of the tree can fold a split away, and every path below it then means
  something else. A drop therefore holds on to what stands at the place rather than to its path,
  and looks the place up again once the panel has been taken out.
- The sheet takes whatever room the panels leave. Every other leaf keeps the size it was given, so
  making the window wider makes the sheet wider and leaves the panels alone.
- The motion switch is the reader's own. The system's own preference for less movement is not read,
  because the toolkit does not report it on every machine the application runs on.
