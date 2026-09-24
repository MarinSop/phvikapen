# Backlog

Work that is understood but deliberately postponed, so that it is not forgotten once the features
of the current milestone are in place. Each entry says what is missing, why it was left out, and
what it would take. Items are picked up after the feature milestones, unless one of them turns
into a real problem earlier.

## Correctness and data safety

- **The trash is not undoable and keeps no order.** A page that is put back goes to the end of its
  section, not where it was, and emptying the trash cannot be undone, which is the point but is
  only guarded by asking twice.
- **The crash test only kills the writing thread's process.** A second process is killed while it
  writes and the notebook still opens with everything before the kill, but power loss, a full disk
  and a killed graphics driver are untested.
- **Backups are never suggested.** A copy of a notebook can be saved somewhere safe, but nothing
  ever reminds anyone to do it and there is no copy on a schedule.

## Performance

- **The shapes of a page are kept twice.** Every stroke keeps its shape so that a change only
  works out what it touched, but those shapes are then copied into one buffer for the graphics
  card, so a full page is held twice. Uploading per stroke would remove the copy.
- **Tiles for dry ink.** The design asks for finished strokes to be baked into GPU tiles with a
  memory budget, so that the cost of a new stroke does not grow with how full the page is. The
  canvas currently keeps one vertex buffer for the whole page.
- **Loading a large notebook.** Opening a page reads and decodes all of its strokes at once. A page
  with tens of thousands of strokes has not been measured, and there is no benchmark for it.

## Imported documents

- **The part in view is drawn again from scratch.** Only what is on screen is drawn, but panning
  past its edge throws the whole picture away and draws the new part from nothing, so there is a
  short wait instead of the old part staying while the new part arrives.
- **Nothing removes a file that no page shows any more.** Undoing an import leaves the file in the
  notebook, and emptying the trash should take it with it.

## Typed text

- **One face for a whole box.** A box of typed text wears one font, size and colour; a single word
  inside it cannot be made bold on its own. Runs of formatting would have to be kept beside the
  text, shown by the editor, printed by the painter and folded for searching.
- **Line spacing is kept but never set.** The face of a box carries how loose its lines are, and
  nothing changes it: the editor the window uses cannot show it, so no control offers it.
- **Only the text tool touches text.** With the loop or the pen in hand, a box of text cannot be
  picked up, moved or deleted; the text tool has to be chosen first. Making the loop pick up text
  as well as ink means teaching the canvas that something above it owns that part of the page.
- **A box cannot be turned.** Text sits square on the paper. Turning it needs an angle on the box,
  on the editor and in the painter.

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
- **The settings are thin.** The paper new notebooks start on is there now, but the pressure curve
  of the pens and the keyboard shortcuts are not, and the shortcuts cannot be changed.

## Picking strokes

- **A rectangle, and no resizing.** Strokes are picked with a rectangle, so a stroke cannot be
  picked out of a crowd by drawing a loop around it, and what is picked can be moved, copied,
  recoloured and deleted, but not resized or turned by its corners.
- **What was copied is only kept in the notebook it came from.** Strokes cannot be pasted into
  another notebook, and nothing is put on the system clipboard.
- **The picture of a picked stroke is worked out again after every move.** Moving rewrites each
  stroke in the notebook file and throws its shape away, so a large selection costs as much as
  drawing it again.

## Keys

- **Only the listed commands can be changed.** The settings list twenty commands; the rest of the
  keys, including the ones for the marker, the shape and the eraser tools, are fixed.
- **A key is taken or free, nothing in between.** There is no way to give one command two keys, or
  to clear a key so a command has none.

## Shapes

- **Once drawn, a shape is an ordinary stroke.** It cannot be turned back into what was drawn, or
  resized by its corners afterwards.
- **A box is always upright.** Boxes and circles are drawn between the two ends of the stroke, so
  they cannot be drawn at an angle.

## Pages

- **A small picture of an imported page is drawn from the whole file.** The PDF is opened and a
  page drawn at thumbnail size for every row that comes into view, and a picture is decoded whole
  before it is shrunk.
- **Every picture is drawn on the thread that draws the window.** A page with a lot of ink is
  painted small where the window runs, and nothing limits how many are kept.

- **Only pages can be dragged, and only inside their section.** Sections are still moved through
  their menu, a page cannot be dragged into another section, and nothing is shown between the rows
  to say where the page would land.
- **A duplicated page copies the whole page.** Its strokes are written again, so duplicating a full
  page costs as much room as the page itself even where nothing was changed.

## Reading page after page

- **Only the pages around the one being read are held.** Two pages either side are read from the
  file and kept; the rest stand as empty sheets until they come near, and nothing throws far pages
  away again, so a long section still grows the longer it is read.
- **A document beside the one being read is drawn once, at twice the size of its sheet.** Only the
  page being read is drawn again as the window is zoomed in, so a neighbour looks soft until it
  becomes the page being read.
- **Sixteen sheets at a time.** The window draws at most sixteen sheets, which is more than fits at
  the smallest zoom for A4, but a much smaller paper would leave the ones past that without their
  sheet.
- **A page without a sheet stands on its own.** A notebook whose pages are endless keeps one page
  at a time, and where a section mixes the two the endless page takes the room of the first sheet
  in the section.

## The window

- **The panels cannot be moved or resized.** The pages and the page setup sit at a fixed width on
  their side of the sheet and can only be turned on and off from the View menu.
- **There is no tool for text.** The palette holds only what the application can do: pick, drag the
  page, draw, highlight, draw a shape and erase.
- **The menus are only checked by their commands.** Tests trigger the commands behind the menus, not
  the menus themselves, and on macOS the menu bar is the one at the top of the screen, which no test
  opens.

## Writing

- **The tip of a stroke is a straight piece.** The body of a stroke being written is redrawn along
  its curve about eighty times a second; the newest samples are strung on straight until the next
  redraw, which is visible only on a very fast hand.
- **Smoothing is one reach.** Samples are averaged along the line as far as the setting reaches,
  the same at every speed of the pen; how far each step should reach, and whether a slow hand needs
  more than a fast one, is still to be measured on the target device.
- **Ink beside the sheet is not in the small pictures.** The sidebar draws the sheet, so anything
  written around it is left out there even when an exported document takes it in.

## Experience

- **The eraser cuts at the samples it was given.** A piece is cut where the samples are, so a long
  straight stroke drawn with few samples is cut coarsely, and the pieces are written to the notebook
  as whole new strokes rather than as a shortening of the old one.

- **The highlighter layer is as large as the window.** Translucent ink is drawn into a picture the
  size of the whole canvas every frame it changes, and always goes under the pen, so a highlighter
  stroke can never cover ink drawn before it.
- **The view of a page is forgotten when the notebook closes.** Each page returns to the zoom and
  the place it was left at, but only until the application is closed.
- **Pen pressure curve.** Every pen follows the same curve from pressure to width, with a floor so
  a light touch still shows. A curve per pen, and a way to tune it, belongs with the measurements on
  the target device.
- **Accessibility.** The tools have shortcuts now, but the focus order through the sidebar has not
  been checked, nothing has been tested with a screen reader, and the shortcuts cannot be changed.

## Reading handwriting

- **A found word is not pointed at.** Choosing what was found opens the page it is on, but nothing
  shows where on the page the word sits, although the place is kept with it.
- **The language is whatever the machine has.** The reader Windows carries reads the languages whose
  handwriting is installed; the application neither says which those are nor offers to install one.
- **Nothing is read on macOS**, so the searching can only be tried on the target device.
- **Words are read, not text.** A page holds strokes; there is no typed text on a page yet, so what
  is read can be copied out but not put back onto the paper.

## Verification that needs the target device or a real run

- **The Windows build of everything since the skeleton.** Sections, pages, the viewport, the
  background shader, the tabs and the tools have only ever been built and run on macOS.
- **Pen behaviour.** Latency, wobble, pressure, tilt and the eraser end of the pen are unmeasured,
  and the reach of the smoothing was chosen on simulated strokes.
- **Golden image tests.** Stroke appearance is checked by unit tests on the geometry, not by
  comparing rendered images against recordings from the real pen.
- **Reading handwriting.** Everything but the reader itself is tested on the development machine;
  what Windows makes of real handwriting, in Croatian or any other language, has never been seen.
