# Backlog

Work that is understood but deliberately postponed, so that it is not forgotten once the features
of the current milestone are in place. Each entry says what is missing, why it was left out, and
what it would take. Items are picked up after the feature milestones, unless one of them turns
into a real problem earlier.

## Correctness and data safety

- **Emptying the trash.** Deleted pages and sections are only marked as trashed, and their strokes
  stay in the notebook file forever. A notebook needs a view of what was deleted, a way to put it
  back, and a way to delete it for good, followed by `VACUUM` so the file actually shrinks.
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

## Experience

- **The highlighter darkens where it crosses itself.** Translucent ink is blended per segment, so a
  stroke that loops over itself is darker there. Drawing all highlighter strokes into a separate
  layer and compositing that layer once would give an even colour.
- **Each page remembers its view.** Going back to a page fits it again instead of returning to the
  zoom and the position it was left at.
- **Pen pressure curve.** Width follows pressure directly. A curve per pen, and a way to tune it,
  belongs with the measurements on the target device.
- **Keyboard and accessibility.** Tools have no shortcuts, the focus order through the sidebar has
  not been checked, and nothing has been tested with a screen reader.

## Verification that needs the target device or a real run

- **The Windows build of everything since the skeleton.** Sections, pages, the viewport, the
  background shader, the tabs and the tools have only ever been built and run on macOS.
- **Pen behaviour.** Latency, wobble, pressure, tilt and the eraser end of the pen are unmeasured,
  and the beta of the One Euro filter is a provisional value chosen on a simulated stroke.
- **Golden image tests.** Stroke appearance is checked by unit tests on the geometry, not by
  comparing rendered images against recordings from the real pen.
