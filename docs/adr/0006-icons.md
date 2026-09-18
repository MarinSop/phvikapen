# 0006. Icon set

Date: 2026-09-18

## Status

Accepted

## Context

The window is laid out the way desktop drawing and document applications are: text menus across
the top, a tool palette down the left side, panels beside the workspace. A tool palette is icons,
not words, so the application needs a set of them.

Drawing them by hand would mean drawing twenty shapes and then drawing every later one in the same
weight and grid, which is a job in itself and one that is easy to do badly. Buying or copying a set
into the repository is out: nothing third party is kept in the tree.

Three sets were looked at:

- **Boxicons.** 1500 outline and filled icons on a 24 unit grid, MIT, published as a plain npm
  package of SVG files with no build step and no font.
- **Material Symbols.** Larger and better known, but published as a variable font or through an
  icon component, and the SVG download is a per icon web service rather than one archive.
- **Feather.** Small and consistent, but thin at the sizes a tool palette uses and missing the
  document shapes, and it has been unmaintained for years.

## Decision

Use **Boxicons 2.1.4**, fetched at configure time from the npm registry with an expected SHA-256,
exactly the way PDFium and Velopack are fetched. The archive is not kept in the repository; only
the names of the icons the application uses are.

Only the outline set is used, and only the files that are named in the build, which are copied into
the application's QML module and shown through `icon.source` on the ordinary controls. Qt's SVG
module draws them, and `icon.color` tints them, so one file serves the light and the dark theme.

An icon is chosen for what it means, not for how it looks: the pen for drawing, the eraser for
erasing, the pointer for picking. Where Boxicons has no icon for something, the nearest honest one
is used rather than a pretty one: the four way arrows stand for moving the page under the window,
because there is no hand in the set.

## Consequences

- One more download at configure time, hash pinned, and one more thing to update by hand when a
  newer Boxicons comes out.
- Qt's SVG module is linked and its image plugin is deployed with the application.
- Icons are for tools and for compact, frequently used actions. Menus, settings and anything whose
  meaning is not obvious at a glance stay in words.
- The set is MIT, which asks only that the licence notice travels with it; the notice is kept with
  the third party notices the installer ships.
