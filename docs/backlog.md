# Backlog

Work that is understood but deliberately postponed, so that it is not forgotten once the features
of the current milestone are in place. Each entry says what is missing, why it was left out, and
what it would take. Items are picked up after the feature milestones, unless one of them turns
into a real problem earlier.

## Correctness and data safety

- **The trash is not undoable and keeps no order.** A page that is put back goes to the end of its
  section, not where it was, and emptying the trash cannot be undone, which is the point but is
  only guarded by asking twice.
- **Crash safety test.** The promise is that a crash costs at most the stroke being written. There
  is no test that kills the application in the middle of a stroke and checks that the notebook
  still opens with everything else in it.
- **Backing notebooks up.** Notebooks live where uninstalling cannot reach them, but there is no
  way to copy one somewhere safe from inside the application, and no reminder to do so.

## Performance

- **Redrawing only what changed.** Undoing, erasing or leafing to a page tessellates every stroke
  of the page again, at about 45 nanoseconds per sample. A page with two hundred thousand samples
  therefore costs about nine milliseconds. Keeping the mesh per stroke and only rebuilding what a
  change touched would remove that.
- **Tiles for dry ink.** The design asks for finished strokes to be baked into GPU tiles with a
  memory budget, so that the cost of a new stroke does not grow with how full the page is. The
  canvas currently keeps one vertex buffer for the whole page.
- **Loading a large notebook.** Opening a page reads and decodes all of its strokes at once. A page
  with tens of thousands of strokes has not been measured, and there is no benchmark for it.

## Imported documents

- **Pictures are decoded on the thread that draws.** PDF pages are read and drawn on their own
  thread, but a picture is decoded where the view model runs, which a very large photograph would
  be felt on.
- **A page of a document is one texture.** The whole page is drawn again whenever the zoom changes
  enough, instead of only the part in view, which will matter on large pages at high zoom.
- **Nothing removes a file that no page shows any more.** Undoing an import leaves the file in the
  notebook, and emptying the trash should take it with it.

## Written documents

- **The whole notebook, or nothing.** Export writes every section and every page. There is no way
  to pick a section, a page or a range, and no way to leave the ruling off the paper.
- **Nothing says how far it got.** The writer reports after every page, but the window only shows
  that it is busy; a long notebook gives no sign of progress and cannot be stopped halfway.
- **An imported page is drawn again, not copied.** A page of an imported PDF is drawn into a
  picture at twice the size of the sheet, so its text is a picture in the exported document and
  cannot be searched or selected. Copying the original page across would keep it as text.
- **The ink is a shape, not a line.** Every stroke is written as its outline. That is exact, but a
  page of many strokes gives a larger file than the same drawing as paths with a width.

## Updates and settings

- **Nothing is ever asked before it phones home.** The first start looks for a newer version
  without asking, and the only way not to is to turn the setting off afterwards.
- **An update is offered once.** If the bar is ignored, nothing brings it back until the next
  start, and there is no way to see what changed in the new version.
- **The settings are thin.** The paper new notebooks start with, the pressure curve of the pens and
  the keyboard shortcuts all belong there, and none of them are in it.

## Experience

- **The highlighter darkens where it crosses itself.** Translucent ink is blended per segment, so a
  stroke that loops over itself is darker there. Drawing all highlighter strokes into a separate
  layer and compositing that layer once would give an even colour.
- **The view of a page is forgotten when the notebook closes.** Each page returns to the zoom and
  the place it was left at, but only until the application is closed.
- **Pen pressure curve.** Width follows pressure directly. A curve per pen, and a way to tune it,
  belongs with the measurements on the target device.
- **Accessibility.** The tools have shortcuts now, but the focus order through the sidebar has not
  been checked, nothing has been tested with a screen reader, and the shortcuts cannot be changed.

## Verification that needs the target device or a real run

- **The Windows build of everything since the skeleton.** Sections, pages, the viewport, the
  background shader, the tabs and the tools have only ever been built and run on macOS.
- **Pen behaviour.** Latency, wobble, pressure, tilt and the eraser end of the pen are unmeasured,
  and the beta of the One Euro filter is a provisional value chosen on a simulated stroke.
- **Golden image tests.** Stroke appearance is checked by unit tests on the geometry, not by
  comparing rendered images against recordings from the real pen.
