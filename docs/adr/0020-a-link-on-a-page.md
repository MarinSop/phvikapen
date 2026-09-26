# 0020. A link on a page

Date: 2026-09-26

## Status

Accepted

## Context

A notebook is a pile of pages, and there is no way to get from one to another except by looking for
it. Everything a reader writes that refers to somewhere else — see the last meeting, the workings
are on the next page, the paper this came from — is a note to themselves that the application
cannot act on.

Two kinds of going somewhere else are wanted: to another page of this notebook, and out of the
application altogether. They are the same gesture to a reader and the same thing on the page, so
they are one thing here.

There is a third question underneath them, which is what makes this worth writing down: **a reader
writing with a pen must not have to put the pen down to follow a link.** A link that only works
when the pick tool is in hand is a link a reader stops using.

## Decision

**A link is a patch of the page, not a property of what is under it.** It stands over whatever is
there — handwriting, type, a picture, a table, nothing at all — rather than belonging to any of
them. One mechanism therefore links everything, including handwriting, which has no place to keep a
property of its own. It is moved, taken away, undone and put on a layer like everything else the
page holds.

The alternative was a property on a box of type, the way a word processor does it. That links type
and nothing else, and handwriting is what this application is for.

**A link to a page names the page by what it is, never by what it is called.** Renaming the page,
moving it to another section or reordering the notebook all leave the link pointing at the same
page. A link whose page has been thrown away says so plainly rather than going somewhere wrong.

**A link out of the application may only go three ways: a page over a network, a page over a secure
network, and mail.** Anything else — a file on this machine, a program to run, a scheme the
window's toolkit would hand to the machine — is refused when the link is made and refused again
when it is followed. A notebook can come from somebody else, and a notebook that can run something
is a notebook that can be used against its reader. The check is in the core, beside the model, so
nothing can reach around it.

**The application never opens anything itself.** It hands the address to the machine and says so
when the machine could not open it.

**A link answers a tap whatever tool is in hand.** The links of a page are drawn over the sheet as
their own layer, one small item each, so a tap on a link is a tap on that item and a stroke
anywhere else is a stroke. A drag that begins on a link is still a stroke, because giving up on a
drag is exactly what separates a tap from writing. Nothing had to be taught to the pen, and nothing
about writing changed.

**A link on a layer that is hidden or held still is not there to be tapped.** It asks the same
question every other hit test on a page asks, so locking a layer locks its links with it.

## Consequences

- Any page can be reached from any other page, and any page can point out of the application.
- A link survives the page it points at being renamed or moved, which is what makes it worth having
  in a notebook that is rearranged.
- A reader writing with the pen follows a link with a tap and keeps writing, which was the point.
- A link cannot yet point at another notebook, at a section, or at a place within a page. Each of
  those is a wider name than a page identifier, and none is needed to move about one notebook.
- A link is a rectangle. It cannot follow the shape of a line of handwriting that wraps, so a link
  over two lines covers the space between them as well.
- Only three ways out are followed. A reader who wants to open a file on their machine from a
  notebook cannot, and that is deliberate.
