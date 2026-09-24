# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- The words a page of handwriting was read into are kept in the notebook, beside the strokes they
  were written with, and searched without case, punctuation or the marks over letters: a search for
  `cehoslovacka` finds `Čehoslovačka`, and several words are found where they follow one another.
  Nothing reads handwriting yet; this is where what is read will be kept.
- Handwriting is read by Windows itself, through the reader it carries: a page of strokes goes in,
  and the words come back with the place they sit in and the strokes they were written with. The
  application can say which languages the machine reads, and where there is no reader, which is
  every machine that is not Windows, it says so instead.
- A notebook reads its own handwriting in the background, one page at a time, on a thread of its
  own: pages written on since they were last read are read again while the hand rests, and what is
  found can be searched for from the notebook.
- Find in Handwriting, from the Edit menu or Ctrl+F: type what was written, and every place it was
  written in this notebook is listed with the page it is on; choosing one opens that page. Where the
  machine cannot read handwriting, the window says so rather than finding nothing quietly.
- Copy as Text, from the Edit menu: what is picked with the loop is read and put on the clipboard as
  words, ready to be pasted anywhere else.
- A text tool. Pick it from the palette or press T, tap anywhere on the paper, and type. A box of
  text can be carried about by the bar above it, pulled wider by the grip on its right edge, and
  taken away with the cross beside it. The bar above the window sets the font, the size, the colour,
  bold, italic, underline, strikethrough and how the lines line up, either for the next box or for
  the one being worked on. What is typed is kept with the page, printed with it, shown in the small
  picture of it, and found by Find in Handwriting along with what was written by hand.
- Convert to Text, from the Edit menu or Ctrl+Shift+R: what is picked with the loop is read, the
  handwriting is taken away, and a box of type in its place says the same thing, in about the size
  the hand wrote it. It can be corrected and moved like any other typed text, and one undo brings
  the handwriting back.

- A menu of what can be done right here, opened by a right click, by the button on the barrel of a
  pen, or by holding the pen still for a moment. It offers what suits what is picked up: cut, copy,
  duplicate, delete, convert to text and turning for a piece of drawing; paste, select everything,
  undo, redo and a new page where nothing is picked. Holding still to open it can be turned off.
- What is picked up can now be made larger, smaller and turned. A frame with eight grips appears
  around it: the corners keep the drawing in proportion, the edges pull one way each, and the knob
  above turns it, in steps of fifteen degrees while Shift is held. Turn Left and Turn Right, in
  Edit ▸ Arrange or on Ctrl+[ and Ctrl+], square something up without dragging. It is all one
  change that one undo puts back exactly.
- Cut, Duplicate and Select All on the Page, from the Edit menu, the menu under the pointer and the
  keys. Duplicate leaves the clipboard alone.
- The eraser now takes either the part of a line it is rubbed over, as it always has, or the whole
  line at a touch. The two are chosen from the bar above the window, from Tools ▸ Switch What the
  Eraser Takes, or on Shift+E, and the choice is remembered. Taking whole lines reaches only a
  quarter as far as the eraser is wide, so a line beside the one meant is left alone, and the ring
  under the pointer shows that smaller reach.
- The color picker carries a glass. While it is in hand the page is shown six times over around the
  point that would be taken, with a cross on the very spot and the color that is in hand below it,
  so that a color can be picked exactly with the tip of a pen.
- Settings for how the application draws and how it is worked: whether how hard the pen presses
  changes the width of the line, what the eraser takes, whether holding still opens the menu, how
  much closer one step of the zoom brings the page, and how large the buttons, bars and menus are
  drawn — which is what makes the window comfortable to work with a pen rather than a mouse. All of
  them can be put back to what they came with in one press.
- A Tools menu, listing every tool with its key and what the eraser takes.

- A setting for versions that are still being tried out. Off, only finished versions are offered;
  on, the newest beta is offered as well and installs itself the same way.

### Fixed

- The frame around what is picked stood still while the drawing under it was turned or sized, and
  sat in a different place from the one the grips belonged to. There is one frame now: it stands
  where the canvas draws its own, and follows the ink through the whole drag, about the very point
  the strokes are turned about. The grips are smaller, are drawn white with a colored edge, and
  step aside while a drag is under way.
- A box of text being typed in is now marked the way picked ink is: a thin frame with small white
  grips. The dark square beside it, which said nothing about what it did, is gone; taking the box
  away is a round mark with a cross on it, beside the knob that carries the box.
- A box of text can be pulled taller as well as wider, by a grip below it and one at its corner,
  and keeps the height it was given even where the words need less. Carrying a box by the knob
  above it now moves the box and the words with the pointer instead of only when it is let go, and
  carrying or pulling a box is one change rather than one for every step of the drag.
- The color picker passes over typed text. It now takes the color of a box of words, which is what
  is topmost where it was asked.
- A box of typed text can be picked up with the pick tool and corrected, without first reaching for
  the text tool.
- A new box of text is plain every time. The face of the last box was remembered and given to the
  next one, and kept between one run of the application and the next.
- The menu under the pointer was as wide as the widest menu of the window, whatever it said. Every
  line is now as wide as what it says.
- The keys did nothing on macOS. What was written beside a command — `⌘Z` and the like — was handed
  back to the toolkit as the keys the command should answer to, and nothing can read that back, so
  undo, redo, copy, paste, save, close, zoom and turning the page answered to no key at all. What a
  command is shown as and what it answers to are now two different things, and the keys are decided
  in one place for the menus, the palette, the bars and the keyboard alike.
- Copy as Text and Pages One Below the Other had both been given Ctrl+Shift+C, which the toolkit
  answers by refusing both. Every command now keeps its own keys, and a test says so.
- A tool whose key is a plain letter no longer takes that letter out of a box of words being typed
  in, and neither do cut, copy, paste, delete or undo, which belong to whoever is typing.
- Delete now also answers to Backspace, which is what a Mac keyboard sends.
- The text tool did nothing on paper that runs on without edges, and nothing beside a sheet: a tap
  that fell on no sheet was quietly dropped. A box now goes where it was put, measured from the page
  being read, and takes its usual width where there is no edge to measure from.
- Notebooks written before a page could be given its own paper open again. The paper went out
  without the number that tells a notebook what it carries, so those notebooks were read as though
  they held columns they never got, and would not open. What is missing is added when they are
  opened, whether they carry the columns already or not.
- Writing on Windows follows the pen. Windows hands over several positions of the pen at once, all
  stamped with the same time, and all but the first were dropped, which cut the corners of letters
  and set lines wobbling. Every position is kept now.

### Changed

- Smoothing averages each sample with its neighbours along the line instead of over time, so it
  evens out a shaky hand without pulling the line behind the pen or rounding off what was written.
  Both ends of a stroke stay where the pen was, the setting is gentle at first and strong only near
  the top of its scale, and its reach is measured on the screen, the same at every zoom.
- A pen rounds the turns it makes with its tip instead of meeting them in a point; drawn shapes keep
  square corners.
- A light touch still leaves a line: width follows a curve with a floor rather than the raw pressure.
- The edges of the ink, the highlighter included, are drawn with several samples to a pixel.
- Curves are cut into as many pieces as their bend needs, and round tips into as many sides as their
  size needs, so both stay round when zoomed in. Exported pages end their strokes round as well.

### Removed

- The One Euro filter, which smoothing no longer needs.

## [0.2.0] - 2026-09-19

### Added

- One Euro filter in the core library, with a pen filter that smooths position and pressure.
- Binary stroke format that stores samples as varint differences, ready for notebook files.
- Notebook files: a SQLite database per notebook, with a versioned schema and one transaction per
  stored stroke.
- What is drawn is kept: finished strokes go into a notebook file and come back when the
  application starts again.
- Undo and redo for drawing, clearing and erasing, from the tool bar and the standard shortcuts.
- Eraser that removes whole strokes, from the eraser tool or the eraser end of the pen. One sweep
  is one change for undo.
- Finished strokes are drawn along a spline as one continuous strip.
- Notebooks hold sections and pages. Pages carry their own paper size, orientation, background and
  line spacing, and can be added, deleted, moved and renamed, all of it undoable.
- The page is shown through a viewport that scrolls and zooms with the wheel, the trackpad, a pinch
  or the fingers on a touch screen, while the pen keeps drawing.
- Paper and its ruling are drawn on the GPU: a sheet on a desk, or an endless canvas, blank, lined,
  squared or dotted.
- A sidebar with the sections and pages of the notebook and the page setup, and page navigation in
  the tool bar and from the keyboard.
- Several notebooks open at once in tabs, created, opened, renamed and deleted from the tab bar.
  The notebooks that were open, and the page each was left on, come back at the next start.
- Three pens, a highlighter and an eraser whose size can be set, with a colour palette. The tools
  are remembered between sessions.
- Messages that used to go only to the log are shown in the window.
- PDFs and pictures can be imported: a PDF adds a page per page of it, a picture adds one page, and
  both are drawn behind the ink so they can be written on. The files are kept inside the notebook.
- A notebook can be written out as a PDF: one page of the document per page of the notebook, each
  keeping its own size, with the paper, its ruling, imported pages and the ink on them.
- The application looks for a newer version when it starts and offers to install it and restart.
- A settings dialog: whether to look for updates at start, where the notebooks are kept, and which
  version this is.
- Deleted pages and sections wait in a trash that can be looked at, put back from, or emptied for
  good, which also removes imported files no page shows any more and shrinks the notebook file.
- Keyboard shortcuts for the tools: P, H and E, the pens on 1 to 3, and [ and ] for the width.
- Going back to a page returns to the zoom and the place it was left at.
- A copy of a notebook can be saved somewhere safe from inside the application.
- The paper, ruling and orientation new notebooks start on can be chosen in the settings.
- A tool that picks strokes: draw a loop around them, then drag them somewhere else or delete them,
  both undoable. It answers to S, and Delete removes what is picked. What is picked can also be copied and
  pasted, onto the same page or another one, and given another colour from the palette.
- Straight lines, boxes and ovals: the pen draws the shape that was chosen instead of the line that
  was actually drawn, and which one is chosen is remembered between sessions.
- A page can be duplicated with everything on it, and pages can be put in another order by dragging
  them in the sidebar.
- The sidebar shows a small picture of every page, with the imported page and the ink on it, drawn
  when the page comes into view and drawn again whenever the page changes.
- A section is read page after page in one column, scrolled through without leafing, and endless
  paper can be written on far past the sheet while the pages stay one below another.
- A notebook is a draft until it is saved: the first Save asks where it belongs, later ones write
  without a word, and closing a notebook or the application with unwritten changes asks first.
- A new notebook can start from a PDF or a picture, on the size the document itself is, or on a
  size chosen instead.
- The colour is one colour: it is picked from anywhere on the page or from the palette, and the pen
  and the highlighter both write in it.
- Boxes can be given rounded corners, from a slider or a typed number, and the same setting decides
  whether lines end flat or round.
- Pages, sections and notebook tabs are carried into another order by dragging them, with a line
  showing where the carried one will land.
- The sections and the pages have panels of their own that can be resized, hidden and brought back,
  from the View menu or from the edge of the panel.
- Paper a reader can choose: the colour of the paper, the colour and the width of its lines, and
  whether the line down the side is drawn, in what colour and how far in. It belongs to the section
  and can be set for new notebooks in the settings.
- The application carries its own picture, on the window and on the executable, and wears a light,
  a dark or the system theme, which it asks for the first time it is started.
- Sizes are typed in the unit that suits the work: pixels, millimetres, centimetres or inches.
- The settings are laid out in tabs, and say whether to open the notebooks that were left open or
  to start fresh every time.

### Changed

- A stroke is drawn along its curve while it is being written, not as a chain of separate pieces,
  so it no longer breaks up where it turns and no longer jumps when the pen is lifted.
- Ink can be written beside the sheet as well as on it. What a page of an exported document holds
  is a setting: the sheet alone, or the sheet with everything written around it.
- How much the pen is smoothed can be set.
- A new notebook is made through a dialog that asks for its name and the paper its pages start on.
- The application starts with no notebook open when none was left open, shows what to do instead of
  an empty sheet, and every notebook can be closed or deleted.
- Widths and sizes are typed as numbers with arrows instead of dragged on a slider.
- The keys for the commands can be changed in the settings, and a key that is already taken is
  refused with the name of the command that has it.
- The window follows the shape of a desktop drawing application: text menus across the top, a
  palette of tools down the left with an icon and a tooltip for each, the pages of the notebook and
  the page setup in panels either side of the sheet, and the page and the zoom in a bar at the
  bottom. Commands that are not tools moved out of the tool bar into the menus they belong to.
- A tool for dragging the page under the window.
- An imported page is drawn at the size it is shown at, so its text stays sharp however far the
  page is zoomed in, instead of a picture of a fixed size being stretched over it.
- Erasing, undoing and leafing back to a page reuse the shapes of the strokes that did not change
  instead of working all of them out again.
- Pictures are read on their own thread, so a large photograph no longer holds up the window.
- The highlighter keeps one even colour where a stroke crosses itself: translucent ink is drawn
  into a layer of its own and laid over the page once, underneath the pen.

- The ink canvas draws and stores smoothed samples, while ink backends keep reporting the raw ones.
- Notebook files are read and written on a storage thread, so drawing never waits for the disk.
- Every stroke keeps a fixed place on its page; notebooks from earlier schema versions are upgraded.
- Notebooks live in the application data folder instead of the local application data folder,
  which the Windows installer deletes on uninstall.
- Settings are stored under the application's own domain instead of the placeholder Qt falls back to.
- The tool bar carries tools alone: no line of text naming the one in use, a shape chosen by its
  own icon rather than from a list, marks large enough to read, and a tool that stays chosen when
  it is pressed again. Where the pen will write and how wide it is is shown under the pointer.
- The Window and the Tools menus are gone and what they held moved to the menus it belongs to. The
  application's picture sits at the top left beside the notebook tabs, which are thinner and carry
  the name alone, and the bar at the bottom no longer repeats the name of the notebook.
- The page setup belongs to the section: every page of it changes together, in one undoable step.
- A stroke is drawn from a round tip, with its corners mitred where it turns, so a line is an even
  ribbon and a tap leaves a dot rather than a square.
- The tool bar presets show their colour on a filled backing, and their marks grow and shrink with
  the width they stand for.

### Fixed

- Removing an ink canvas no longer runs code on the half destroyed item.
- What is drawn belongs to the sheet it was started on: pressing on another page neither moves the
  view nor carries the line onto a page it was not written on.
- Scrolling no longer resets the zoom or pulls the view onto a page of its own accord.
- Pages of an imported document no longer stay blank, blurred or half drawn while a section is
  read, and are drawn again whenever the sheet under them changes size.
- Pages no longer overlap or leave the sheet empty when the paper size changes, and the page being
  looked at changes with the rest instead of at the next turn of the page.
- A notebook started from a document no longer keeps an empty first page.
- A page keeps its name when it is carried into another order, and lands where the line says.
- A new section is no longer named with a number in brackets after it, and the name offered for a
  new page or section is taken when the work carries on elsewhere.
- A box closes with square corners and an oval closes without a gap.
- The buttons in the dialogs and the buttons that close things light up under the pointer.

## [0.1.0] - 2026-09-16

### Added

- Repository conventions: EditorConfig, line-ending normalization, clang-format, clang-tidy,
  qmlformat and qmllint settings, and pre-commit hooks.
- CMake build system with presets for macOS and Windows ARM64, a vcpkg manifest with a pinned
  baseline, warning, sanitizer and sccache configuration, and layering checks.
- Core library with the error type, `Result` alias and generated version header.
- Ink sample and stroke types with incremental bounding boxes, and a monotonic UUIDv7 generator.
- Google Benchmark suite for the core library, built by the release presets.
- Platform interfaces for ink backends, PDF documents and updates, a placeholder for the native
  Windows ink backend, and the Velopack startup hook.
- Qt Quick ink backend that captures pen and mouse input and renders wet ink incrementally
  through QRhi, backed by pressure-aware stroke tessellation in the core library.
- Application window with a tool bar, notebook tabs and the ink canvas, C++ view models exposed to
  QML, and logging to a rotating file.
- Qt Quick tests for the view models and the tool bar, and qmllint as part of every build.
- Ink recorder tool that writes raw pen and mouse events to CSV for filter tuning.
- Continuous integration on macOS and Windows on ARM, a release workflow that publishes the
  Windows installer and update packages with their checksums, and Dependabot updates.
- README with prerequisites, build and release instructions, and architecture decision records for
  the technology stack, the ink backend and the update framework.
