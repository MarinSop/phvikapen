# Backlog

Work that is understood but deliberately postponed, so that it is not forgotten once the features
of the current milestone are in place. Each entry says what is missing, why it was left out, and
what it would take. Items are picked up after the feature milestones, unless one of them turns
into a real problem earlier.

## Correctness and data safety

- **Emptying the trash cannot be undone.** A page or a section put back goes back where it stood,
  because where it stood is written down beside it, but emptying the trash is final, which is the
  point and is only guarded by asking twice.
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

- **The question about looking for new versions is asked at the first start, not before the first
  window.** The reader is asked once, nothing is sent until they have answered, and the answer can be
  changed in the settings. The question stands over the window rather than being part of a first run
  that introduces the application.
- **An update is offered once.** If the bar is ignored, nothing brings it back until the next
  start, and there is no way to see what changed in the new version.
- **The settings are thin.** The paper new notebooks start on is there now, but the pressure curve
  of the pens and the keyboard shortcuts are not, and the shortcuts cannot be changed.

## Picking strokes

- **Ink is picked with a rectangle, not with a loop.** What is picked can be moved, copied,
  recoloured, deleted, sized by its eight grips and turned by its knob, but the pick itself is a
  rectangle: a stroke cannot be taken out of a crowd by drawing a loop around it, which needs a
  point-in-polygon test against every stroke rather than a box.
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

- **A page moves into another section through its menu, not by being dragged there.** A page is
  dragged to reorder it inside its own section, and its menu moves it to the foot of any other
  section, which can be taken back. Dragging it across to the list of sections would want one drag
  reckoned over two panels. Sections themselves are still moved through their own menu.
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

- **Two windows over the same patch of screen both offer a carried panel a place.** A drag is now
  reckoned on the screens, so a panel carried out of its own window lands in another one, and every
  window draws the lines and the name. Where two windows overlap, both report what is under the hand
  and the one with the stronger claim wins rather than the one actually in front. A panel carried
  *into* a free window is refused as well: a free window shows one panel, and what a second one
  would do to it is not settled.
- **A carried tab leaves a gap of its own width behind.** The tab is not drawn in the strip while it
  is carried, so it is never in two places, but the space it came out of stays open at its old width
  rather than closing up behind it, and the strip does not draw an outline of where it will land
  beyond the gap the other tabs open.
- **Marked layers are hidden and locked together, but not moved or deleted together.** Several layers
  are marked out with Shift or Ctrl held and hidden or locked in one go, which counts as one thing to
  take back. Reordering and deleting still reach one layer at a time, and nothing on the panel says
  that a key is what marks one out.
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

- **Only the word followed to is pointed at.** Choosing what was found brings the word into view and
  marks it out for a moment. The other words found on the same page are not marked, and there is no
  way to step from one to the next without going back to the list.
- **The language is whatever the machine has.** The reader Windows carries reads the languages whose
  handwriting is installed; the application neither says which those are nor offers to install one.
- **Nothing is read on macOS**, so the searching can only be tried on the target device.
- **Words are read, not text.** A page holds strokes; there is no typed text on a page yet, so what
  is read can be copied out but not put back onto the paper.

## Things a page cannot hold yet

Each of these is a new kind of thing on a page, and each needs the same five parts: something to
put in the model, a column in the notebook and a version to go with it, a command or two to undo,
something to draw it in the window and in what is exported, and a place in the menu that opens
under the pointer. The change of place, size and angle that the loop tool now uses is already
written so that all of them can share it. They are listed in the order they are worth doing.

- **A picture and the ink around it.** A picture is put down, moved, sized, turned and taken away,
  but the loop tool cannot pick a picture and some ink together, and a picture cannot be copied,
  cut or duplicated. It also cannot be stretched out of shape: the corner grips keep what it came
  with, which is right for a photograph and wrong for a diagram meant to fill a box.
- **What a picture costs while it is on the screen.** Each picture holds a texture on the graphics
  card, and one larger than four thousand pixels along its longest side is kept smaller than the
  camera made it. A page with dozens of photographs has not been measured, and the reading of a
  picture is done where the window waits for it rather than on a thread of its own.
- **Cropping a picture.** A picture keeps the whole of what it was given. Cropping would be a
  rectangle kept beside the rest, and can be added without moving anything already kept.
- **A size of its own for one box of a table.** A box can be given a colour behind it, a colour for
  its words, bold, slant and where it sits between top and foot, but the size of the type and the
  family belong to the whole table. A size per box is another column beside the rest, in the same
  shape as the ones already there.
- **Ruling a table box by box.** The colour and thickness of the rules belong to the whole table.
  Ruling one box differently from its neighbours, and a rule that is dashed rather than solid, both
  need the ruling to be kept per box and drawn per edge instead of per box outline. What the window
  draws a rule with cannot dash a rectangle, so this needs a shape rather than a rectangle.
- **A table under the ink, on the glass.** Layers order what is printed exactly, layer by layer.
  On the glass the ink is drawn by the canvas and a table by the window over it, so a table cannot
  be shown underneath ink however the layers are ordered. Removing that means drawing ink, pictures,
  tables and type through one renderer.
- **Ink on a locked layer can still be picked with the loop.** What answers a tap is decided by the
  page for pictures, tables and boxes of type; ink is picked by the canvas, which is handed strokes
  without being told which layer they stand on. Hiding a layer does hide its ink.
- **What the Windows picture reader makes of a real picture has never been seen.** Windows reads a
  picture with the reader it carries, against the same interface macOS uses, and the build is
  proven on Windows on ARM by the pipeline. What it reads out of real handwriting or a real
  photograph, in Croatian or any other language, has only been read on macOS so far. The reader
  also reads only the languages whose packs the machine has, so a machine without a Croatian pack
  reads Croatian words as whatever language it does have.
- **Reading every picture is asked for, not done on its own.** One command reads every picture in a
  notebook that nothing has been read out of yet, and it can be stopped, but nothing starts it by
  itself, and a picture nothing could be read out of is tried once more the next time the notebook
  is opened, because only words are written down and there are none to write.
- **A run of words on a picture is picked out with a key held down, and nothing says so.** Several
  runs are picked out together with Shift or Ctrl held, and the menu offers every run at once, the
  runs picked out as one block of type or as one thing copied, and everything read out of the
  picture. Nothing on the page says that a key is what picks one out by hand.
- **The loop takes a picture only where the picture stands wholly inside the ink.** Ink, words,
  tables and pictures are all kept, and everything standing wholly inside the box around the picked
  ink is taken with it, so an element holds as many pictures as the loop went round. A picture the
  ink only crosses is left behind, and the loop is reckoned as the box around the ink rather than
  the shape the hand drew.
- **The outline of where an element will land is drawn only once the element has been listed.** How
  much room an element takes is worked out when its small picture is drawn, so an element whose
  picture has not been drawn yet is carried with no outline under the pointer, and the outline shows
  the room it takes rather than what stands in it.
- **A link points at a page, and nothing finer or wider.** Another notebook, a section, and a place
  within a page are all names wider than a page identifier, and none of them is needed to move
  about one notebook.
- **A link is a rectangle.** It cannot follow the shape of a line of handwriting that wraps, so a
  link over two lines covers the space between them as well.
- **The Windows speech reader is an older one than the Mac's.** Windows carries only one reader that
  can be handed a recording rather than a microphone, and it is the Speech API the system has
  carried for many years. It reads dictation with whatever speech packs the machine has, it is
  poorer at that than the reader macOS carries, and a machine with no pack reads nothing and says
  so. The newer recognizer cannot be given a recording at all.
- **A recording is decoded in full before it is read on Windows.** The reader takes plain samples,
  so a long recording is held in memory twice over: once as it was recorded, once as samples.
  Reading it a piece at a time would need the reader to be fed as the decoding runs.
- **Where the Windows reader puts a word in time has not been checked against a long recording.**
  Every word comes back with the reader's own offset within the run it was heard in, and the run
  with its place in the sound. Whether those two add up over an hour has only been reasoned about.
- **A transcript is as fine as a line, not as fine as a word.** The reader gives a moment for every
  word, and words spoken together are gathered into lines at a pause or a full stop. Tapping moves
  the playing to a line; a word inside one cannot be tapped on its own.
- **A reading is not tested against a real recording by the test suite.** Reading needs the
  person's permission and a bundle that asks for it, so the tests cover gathering words into lines
  and the refusals, and the reading itself is tried by hand.
- **A mark is as fine as a thing on the page, not as fine as a word.** A box of type typed over
  four minutes carries the moment it was begun. Marking inside the words is the thing that would
  not survive the words being corrected.
- **A recording cannot be trimmed, joined or taken out on its own.** It is made, played, renamed
  and taken away, and the sound lives in the notebook file, so a long recording makes a large
  notebook and only deleting it gets the room back.
- **A countdown makes one noise and it cannot be changed.** Two notes worked out as they are
  played. It can be turned off, but not chosen, and no other part of the application makes a noise.
- **The clock stops when the application does.** How long a countdown was set for is remembered;
  how far along it was is not, because a count that went on while the application was shut would be
  a lie about time nobody was keeping.
- **An equation with more than one letter is drawn but not solved.** A statement with two letters
  is a curve, and the curve is drawn; solving one of them in terms of the other, and solving two
  equations together, both need rearranging rather than gathering into powers.
- **A curve drawn on the page carries no figures.** The curve, its two axes and the rules between
  them go on the page as strokes, so they are moved, recoloured and erased like anything drawn by
  hand, but the numbers along the axes are not written: they would be boxes of type rather than
  ink, and would scatter the graph into a score of separate things to pick up. Drawing the whole
  graph as one picture instead is the other way round, and would give up the ink.
- **A drawn graph is a handful of strokes, not one thing.** The loop picks them all together, so it
  is moved and deleted like any other drawing, but it cannot be sized by its corners, it does not
  change again when the graph beside the notes is moved, and nothing ties the strokes together as
  the graph they came from.
- **A power higher than a square is searched, not solved.** A straight line and a square come out
  exactly; a cube and above are found by halving between the turning points, so the answers are as
  near as a double can get rather than written as a formula. A power above the eighth is refused,
  and an answer that is not a plain number is never found, because only real numbers are searched.
- **Mathematics that is not written on one line.** A sum written across is read, worked out and
  answered. A fraction written as one number over another, a power written small and raised, a root
  drawn over what it covers and anything else arranged in two dimensions are not read, because the
  reader the platform carries reads a line of characters. That needs a recognizer built for
  mathematics, which is a dependency and needs a decision of its own written down. Nothing else in
  the chain moves when it comes: it replaces the first link only.
- **A sum drawn with more than the four operations.** Fractions, powers, roots and brackets are
  drawn the way they are written. A sum sign, an integral, a matrix and anything else that stands
  over a range are not, and each needs a shape of its own in the structure before it can be drawn.
- **Solving for an unknown.** Only arithmetic is answered. An equation with an x in it to be solved
  for is what a student most wants, and it works on the same structure that is already kept. It is
  held up by the reading, not by the working out: a letter x is read as a times sign, because
  multiplication is what people write far more often.

## Verification that needs the target device or a real run

- **The Windows build of everything since the skeleton.** Sections, pages, the viewport, the
  background shader, the tabs and the tools have only ever been built and run on macOS.
- **Pen behaviour.** Latency, wobble, pressure, tilt and the eraser end of the pen are unmeasured,
  and the reach of the smoothing was chosen on simulated strokes.
- **Golden image tests.** Stroke appearance is checked by unit tests on the geometry, not by
  comparing rendered images against recordings from the real pen.
- **Reading handwriting.** Everything but the reader itself is tested on the development machine;
  what Windows makes of real handwriting, in Croatian or any other language, has never been seen.
- **Reading a picture and a recording on Windows.** Both readers are built and linked by the
  pipeline on Windows on ARM, and everything above them is tested on the development machine. What
  either one makes of a real photograph or a real recording has only been seen on macOS.
