#pragma once

#include "app/cpp/HandwritingReader.hpp"
#include "app/cpp/LayerModels.hpp"
#include "app/cpp/OutlineModels.hpp"
#include "app/cpp/RecordingModels.hpp"
#include "app/cpp/TableModels.hpp"
#include "app/cpp/TextModels.hpp"
#include "core/Error.hpp"
#include "core/geometry/Distance.hpp"
#include "core/geometry/Rect.hpp"
#include "core/geometry/Viewport.hpp"
#include "core/id/ContentId.hpp"
#include "core/id/Uuid.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"
#include "core/ink/StrokeEraser.hpp"
#include "core/ink/StrokeHitTest.hpp"
#include "core/ink/StrokeTransform.hpp"
#include "core/model/Asset.hpp"
#include "core/model/Outline.hpp"
#include "core/model/Page.hpp"
#include "core/model/PageStyle.hpp"
#include "core/model/Table.hpp"
#include "core/model/TextBox.hpp"
#include "core/storage/StorageThread.hpp"
#include "core/text/WrittenText.hpp"
#include "core/undo/UndoStack.hpp"
#include "platform/ink/IInkBackend.hpp"
#include "platform/ink/qt/QtInkItem.hpp"
#include "platform/ocr/IReadPicture.hpp"
#include "platform/pdf/PdfRenderer.hpp"
#include "platform/render/PdfExporter.hpp"

#include <QColor>
#include <QImage>
#include <QObject>
#include <QPointF>
#include <QPointer>
#include <QQmlParserStatus>
#include <QRectF>
#include <QString>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <QtQmlIntegration>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace phvikapen::app {

class NotebookViewModel : public QObject, public QQmlParserStatus {
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_ELEMENT
    Q_PROPERTY(QString notebookPath READ notebookPath WRITE setNotebookPath NOTIFY
                   notebookPathChanged FINAL)
    Q_PROPERTY(QString name READ name NOTIFY notebookPathChanged FINAL)
    Q_PROPERTY(QString currentPageId READ currentPageId NOTIFY currentPageChanged FINAL)
    Q_PROPERTY(QString startPage READ startPage WRITE setStartPage NOTIFY startPageChanged FINAL)
    Q_PROPERTY(phvikapen::platform::ink::QtInkItem* canvas READ canvas WRITE setCanvas NOTIFY
                   canvasChanged FINAL)
    Q_PROPERTY(QVariantMap pointedWord READ pointedWord NOTIFY pointedWordChanged FINAL)
    Q_PROPERTY(bool loaded READ loaded NOTIFY loadedChanged FINAL)
    Q_PROPERTY(bool readsHandwriting READ readsHandwriting CONSTANT FINAL)
    Q_PROPERTY(int pagesToRead READ pagesToRead NOTIFY readingChanged FINAL)
    Q_PROPERTY(int picturesToRead READ picturesToRead NOTIFY readingPicturesChanged FINAL)
    Q_PROPERTY(QString title READ title NOTIFY outlineChanged FINAL)
    Q_PROPERTY(QString keptAt READ keptAt NOTIFY keptAtChanged FINAL)
    Q_PROPERTY(bool edited READ edited NOTIFY editedChanged FINAL)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged FINAL)
    Q_PROPERTY(int strokeCount READ strokeCount NOTIFY pageChanged FINAL)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged FINAL)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY historyChanged FINAL)
    Q_PROPERTY(phvikapen::app::OutlineListModel* sections READ sections CONSTANT FINAL)
    Q_PROPERTY(phvikapen::app::OutlineListModel* pages READ pages CONSTANT FINAL)
    Q_PROPERTY(int currentSection READ currentSection WRITE setCurrentSection NOTIFY
                   currentPageChanged FINAL)
    Q_PROPERTY(
        int currentPage READ currentPage WRITE setCurrentPage NOTIFY currentPageChanged FINAL)
    Q_PROPERTY(int sectionCount READ sectionCount NOTIFY outlineChanged FINAL)
    Q_PROPERTY(int pageCount READ pageCount NOTIFY outlineChanged FINAL)
    Q_PROPERTY(bool hasPreviousPage READ hasPreviousPage NOTIFY currentPageChanged FINAL)
    Q_PROPERTY(bool hasNextPage READ hasNextPage NOTIFY currentPageChanged FINAL)
    Q_PROPERTY(phvikapen::app::page_options::Paper paper READ paper WRITE setPaper NOTIFY
                   pageStyleChanged FINAL)
    Q_PROPERTY(phvikapen::app::page_options::Orientation orientation READ orientation WRITE
                   setOrientation NOTIFY pageStyleChanged FINAL)
    Q_PROPERTY(phvikapen::app::page_options::Background background READ background WRITE
                   setBackground NOTIFY pageStyleChanged FINAL)
    Q_PROPERTY(
        qreal lineSpacing READ lineSpacing WRITE setLineSpacing NOTIFY pageStyleChanged FINAL)
    Q_PROPERTY(
        qreal customWidth READ customWidth WRITE setCustomWidth NOTIFY pageStyleChanged FINAL)
    Q_PROPERTY(
        qreal customHeight READ customHeight WRITE setCustomHeight NOTIFY pageStyleChanged FINAL)
    Q_PROPERTY(QColor paperColor READ paperColor WRITE setPaperColor NOTIFY pageStyleChanged FINAL)
    Q_PROPERTY(QColor lineColor READ lineColor WRITE setLineColor NOTIFY pageStyleChanged FINAL)
    Q_PROPERTY(
        QColor marginColor READ marginColor WRITE setMarginColor NOTIFY pageStyleChanged FINAL)
    Q_PROPERTY(qreal lineWidth READ lineWidth WRITE setLineWidth NOTIFY pageStyleChanged FINAL)
    Q_PROPERTY(qreal marginAt READ marginAt WRITE setMarginAt NOTIFY pageStyleChanged FINAL)
    Q_PROPERTY(bool margin READ margin WRITE setMargin NOTIFY pageStyleChanged FINAL)
    Q_PROPERTY(bool exporting READ exporting NOTIFY exportingChanged FINAL)
    Q_PROPERTY(bool continuous READ continuous WRITE setContinuous NOTIFY continuousChanged FINAL)
    Q_PROPERTY(phvikapen::app::TrashListModel* trash READ trash CONSTANT FINAL)
    Q_PROPERTY(phvikapen::app::TextListModel* texts READ texts CONSTANT FINAL)
    Q_PROPERTY(
        QString pickedText READ pickedText WRITE setPickedText NOTIFY pickedTextChanged FINAL)
    Q_PROPERTY(QVariantMap pickedBox READ pickedBox NOTIFY pickedBoxChanged FINAL)
    Q_PROPERTY(QString pickedPicture READ pickedPicture WRITE setPickedPicture NOTIFY
                   pickedPictureChanged FINAL)
    Q_PROPERTY(QVariantMap pickedPictureBox READ pickedPictureBox NOTIFY pickedPictureChanged FINAL)
    Q_PROPERTY(phvikapen::app::TableListModel* tables READ tables CONSTANT FINAL)
    Q_PROPERTY(
        QString pickedTable READ pickedTable WRITE setPickedTable NOTIFY pickedTableChanged FINAL)
    Q_PROPERTY(QVariantMap pickedTableBox READ pickedTableBox NOTIFY pickedTableChanged FINAL)
    Q_PROPERTY(phvikapen::app::LayerListModel* layers READ layers CONSTANT FINAL)
    Q_PROPERTY(phvikapen::app::RecordingListModel* recordings READ recordings CONSTANT FINAL)
    Q_PROPERTY(phvikapen::app::SayingListModel* sayings READ sayings CONSTANT FINAL)
    Q_PROPERTY(QString shownRecording READ shownRecording WRITE showRecording NOTIFY
                   recordingsChanged FINAL)
    Q_PROPERTY(int shownReading READ shownReading NOTIFY recordingsChanged FINAL)
    Q_PROPERTY(QVariantList links READ links NOTIFY linksChanged FINAL)
    Q_PROPERTY(QVariantList pagesToLinkTo READ pagesToLinkTo NOTIFY outlineChanged FINAL)
    Q_PROPERTY(QString shownTrouble READ shownTrouble NOTIFY recordingsChanged FINAL)
    Q_PROPERTY(QString activeLayer READ activeLayer WRITE setActiveLayer NOTIFY layersChanged FINAL)
    Q_PROPERTY(QStringList markedLayers READ markedLayers NOTIFY markedLayersChanged FINAL)

public:
    explicit NotebookViewModel(QObject* parent = nullptr);
    NotebookViewModel(QString path, QString startPage, QObject* parent);
    ~NotebookViewModel() override;

    NotebookViewModel(const NotebookViewModel&) = delete;
    NotebookViewModel& operator=(const NotebookViewModel&) = delete;
    NotebookViewModel(NotebookViewModel&&) = delete;
    NotebookViewModel& operator=(NotebookViewModel&&) = delete;

    void classBegin() override {}

    void componentComplete() override;

    [[nodiscard]] QString notebookPath() const { return m_notebookPath; }

    [[nodiscard]] static bool readsHandwriting();

    [[nodiscard]] int pagesToRead() const { return m_pagesToRead; }

    // Every word of handwriting that holds what was typed, wherever it is in this notebook.
    Q_INVOKABLE void find(const QString& text);

    // Show the page one of the words found by the last search was written on.
    Q_INVOKABLE void goToFound(int index);

    // The word the reader was taken to, where it stands in the column, so that the page can point
    // at it. Empty where nothing is being pointed at.
    [[nodiscard]] QVariantMap pointedWord() const { return m_pointedWord; }

    // Stops pointing at it, once the page has finished saying where it was.
    Q_INVOKABLE void forgetPointedWord();

    void setNotebookPath(const QString& path);

    [[nodiscard]] QString name() const;

    [[nodiscard]] QString currentPageId() const;

    [[nodiscard]] QString startPage() const { return m_startPage; }

    void setStartPage(const QString& pageId);

    [[nodiscard]] bool renameTo(const QString& path);

    void applyStyle(const core::PageStyle& style);

    [[nodiscard]] platform::ink::QtInkItem* canvas() const { return m_canvas; }

    void setCanvas(platform::ink::QtInkItem* canvas);

    [[nodiscard]] bool loaded() const { return m_loaded; }

    [[nodiscard]] QString title() const;

    [[nodiscard]] QString errorMessage() const { return m_errorMessage; }

    [[nodiscard]] int strokeCount() const;

    [[nodiscard]] bool canUndo() const { return m_history.canUndo(); }

    [[nodiscard]] bool canRedo() const { return m_history.canRedo(); }

    [[nodiscard]] OutlineListModel* sections() { return &m_sectionsModel; }

    [[nodiscard]] OutlineListModel* pages() { return &m_pagesModel; }

    [[nodiscard]] int currentSection() const;
    void setCurrentSection(int index);
    [[nodiscard]] int currentPage() const;
    void setCurrentPage(int index);
    [[nodiscard]] int sectionCount() const;
    [[nodiscard]] int pageCount() const;
    [[nodiscard]] bool hasPreviousPage() const;
    [[nodiscard]] bool hasNextPage() const;

    [[nodiscard]] page_options::Paper paper() const;
    void setPaper(page_options::Paper paper);
    [[nodiscard]] page_options::Orientation orientation() const;
    void setOrientation(page_options::Orientation orientation);
    [[nodiscard]] page_options::Background background() const;
    void setBackground(page_options::Background background);
    [[nodiscard]] QColor paperColor() const;
    void setPaperColor(const QColor& color);
    [[nodiscard]] QColor lineColor() const;
    void setLineColor(const QColor& color);
    [[nodiscard]] QColor marginColor() const;
    void setMarginColor(const QColor& color);
    [[nodiscard]] qreal lineWidth() const;
    void setLineWidth(qreal width);
    [[nodiscard]] qreal marginAt() const;
    void setMarginAt(qreal millimeters);
    [[nodiscard]] bool margin() const;
    void setMargin(bool shown);

    [[nodiscard]] qreal lineSpacing() const;
    void setLineSpacing(qreal millimeters);
    [[nodiscard]] qreal customWidth() const;
    void setCustomWidth(qreal millimeters);
    [[nodiscard]] qreal customHeight() const;
    void setCustomHeight(qreal millimeters);

    Q_INVOKABLE void applyStyleToSection();

    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void clearPage();

    Q_INVOKABLE void previousPage();
    Q_INVOKABLE void nextPage();
    Q_INVOKABLE void addPage();
    Q_INVOKABLE void deletePage(int index);
    Q_INVOKABLE void movePage(int from, int to);

    // Moves a page of the section being read to the foot of another section. The last page of a
    // section stays, as it does when one is deleted.
    Q_INVOKABLE void movePageToSection(int from, int section);
    Q_INVOKABLE void duplicatePage(int index);
    Q_INVOKABLE void wantThumbnail(int index);

    Q_INVOKABLE void wantLayerPreview(const QString& layerId);

    // How many pixels across the document of a page was drawn with; nothing, where none is shown.
    Q_INVOKABLE [[nodiscard]] int mediaPixelsOn(int index) const;
    Q_INVOKABLE void renamePage(int index, const QString& title);

    Q_INVOKABLE void importDocument(const QUrl& fileUrl);

    // A notebook that starts from a document keeps that document's pages alone.
    Q_INVOKABLE void startFromDocument(const QUrl& fileUrl);

    [[nodiscard]] bool continuous() const { return m_continuous; }

    void setContinuous(bool continuous);

    [[nodiscard]] bool exporting() const { return m_exporting; }

    Q_INVOKABLE void exportToPdf(const QUrl& fileUrl, int scope = 0);

    // The notebook is written where the reader keeps it only when they ask for it. Until then
    // the work sits in the notebook's own file, safe from a crash but not yet theirs.
    Q_INVOKABLE bool save();

    Q_INVOKABLE void saveAs(const QUrl& fileUrl);

    [[nodiscard]] QString keptAt() const { return m_keptAt; }

    [[nodiscard]] bool edited() const { return m_edited; }

    [[nodiscard]] TrashListModel* trash() { return &m_trashModel; }

    [[nodiscard]] TextListModel* texts() { return &m_textsModel; }

    [[nodiscard]] TableListModel* tables() { return &m_tablesModel; }

    // The layers of the page being read, top first, the way a panel of layers is read.
    [[nodiscard]] LayerListModel* layers() { return &m_layersModel; }

    // The recordings made on the page being read, oldest first.
    [[nodiscard]] RecordingListModel* recordings() { return &m_recordingsModel; }

    // What was said in the recording being shown, run by run.
    [[nodiscard]] SayingListModel* sayings() { return &m_sayingsModel; }

    [[nodiscard]] QString shownRecording() const { return m_shownRecording; }

    Q_INVOKABLE void showRecording(const QString& recordingId);

    // How far the reading of the recording being shown has got, and why it could not be done.
    [[nodiscard]] int shownReading() const;
    [[nodiscard]] QString shownTrouble() const;

    // A recording begun: it is put on the page at once, with no sound in it yet, so that
    // everything written from now on can be tied to it. Its name comes back.
    Q_INVOKABLE QString beginRecording();

    // The sound of a recording that was begun, kept with the page it was made on. The sound goes
    // into the assets of the notebook, by what it contains, like every other run of bytes.
    Q_INVOKABLE void keepRecording(const QString& recordingId, const QByteArray& sound,
                                   qint64 length);

    // A recording that was begun and came to nothing, taken off the page again.
    Q_INVOKABLE void giveUpRecording(const QString& recordingId);

    Q_INVOKABLE void renameRecording(const QString& recordingId, const QString& name);

    Q_INVOKABLE void removeRecording(const QString& recordingId);

    // Asks for the sound of a recording. It comes back through `soundReady`, because it is read
    // from the notebook away from the window.
    Q_INVOKABLE void wantSound(const QString& recordingId);

    // While a recording runs, everything put on the page is tied to the moment it was put there.
    // An empty name says nothing is being recorded.
    Q_INVOKABLE void markFrom(const QString& recordingId, qint64 at);

    // Where in a recording a thing on the page was written, or -1 where it was not written while
    // anything was being recorded.
    Q_INVOKABLE [[nodiscard]] qint64 momentOf(const QString& thingId) const;

    // Which recording a thing on the page was written during, empty where there is none.
    Q_INVOKABLE [[nodiscard]] QString recordingOf(const QString& thingId) const;

    // What was on the page at a moment in a recording, so that playing can show what was being
    // written about. Empty where nothing had been written yet.
    Q_INVOKABLE [[nodiscard]] QString thingWrittenAt(const QString& recordingId, qint64 at) const;

    // What was said in a recording, in place of whatever was written down before.
    Q_INVOKABLE void keepSayings(const QString& recordingId, const QVariantList& sayings,
                                 const QString& language);

    Q_INVOKABLE void markReading(const QString& recordingId, int reading, const QString& trouble);

    // The links on the page being read, as the window draws them: where each stands on the page
    // and where it goes.
    [[nodiscard]] QVariantList links() const;

    // Every page of the notebook a link could be made to, named by what it is.
    [[nodiscard]] QVariantList pagesToLinkTo() const;

    // A link put over what the reader has hold of, or over a patch of the page where nothing is
    // held. `where` is a page of this notebook where `toPage` is true, and somewhere outside the
    // application otherwise.
    Q_INVOKABLE void addLink(const QString& where, bool toPage, const QString& label);

    Q_INVOKABLE void changeLink(const QString& linkId, const QString& where, bool toPage,
                                const QString& label);

    Q_INVOKABLE void removeLink(const QString& linkId);

    // The link under a place on the page, or nothing at all where there is none.
    Q_INVOKABLE [[nodiscard]] QVariantMap linkUnder(qreal columnX, qreal columnY) const;

    // Everything about one link, named by what it is.
    Q_INVOKABLE [[nodiscard]] QVariantMap aboutLink(const QString& linkId) const;

    // Follows a link: goes to the page it names, or says where outside the application it goes.
    Q_INVOKABLE void followLink(const QString& linkId);

    // The area of the page the reader has hold of, which is where a new link is put.
    Q_INVOKABLE [[nodiscard]] QVariantMap areaOfWhatIsPicked() const;

    // Everything the reader has hold of, carried off the page so that it can be kept somewhere
    // else and put down again later. Where it stood is kept with it, so that a handful put down
    // again keeps the shape it had.
    struct Handful {
        // A picture and what it is made of. The bytes are carried along because a picture is kept
        // once in the notebook it came from, and a handful may be put down in another one.
        struct Carried {
            core::PlacedPicture placed;
            std::shared_ptr<const std::vector<std::byte>> bytes;
        };

        std::vector<core::PlacedStroke> strokes;
        std::vector<core::PlacedText> texts;
        std::vector<core::PlacedTable> tables;
        std::vector<Carried> pictures;
        core::Rect area{};
    };

    using HandfulReady = std::function<void(Handful)>;

    [[nodiscard]] Handful handfulPicked() const;

    // The same handful, with whatever its pictures are made of fetched from the notebook first.
    // The answer comes back through `ready`, because the bytes are not to hand.
    void takeHandful(const HandfulReady& ready);

    // A handful put down in the middle of what is being looked at, everything in it given a name
    // of its own so that the same handful can be put down many times.
    void putDownHandful(const Handful& handful);

    // The same, put down with its top left corner where the pointer is rather than in the middle.
    void putDownHandfulAt(const Handful& handful, qreal columnX, qreal columnY);

    // The layer anything new is put on, which is the one the reader has chosen in the panel.
    [[nodiscard]] QString activeLayer() const { return m_activeLayer; }

    void setActiveLayer(const QString& layerId);

    // A layer put above the one in hand, and one taken away with everything standing on it.
    Q_INVOKABLE void addLayer();

    Q_INVOKABLE void removeLayer(const QString& layerId);

    // A layer put down again, with a copy of everything that stands on it.
    Q_INVOKABLE void duplicateLayer(const QString& layerId);

    Q_INVOKABLE void renameLayer(const QString& layerId, const QString& name);

    // Layers marked out beside the one in hand, so that hiding or locking reaches all of them at
    // once and counts as one thing done.
    [[nodiscard]] QStringList markedLayers() const { return m_markedLayers; }

    Q_INVOKABLE void markLayer(const QString& layerId, bool marked);
    Q_INVOKABLE void unmarkLayers();
    Q_INVOKABLE void showMarkedLayers(bool shown);
    Q_INVOKABLE void lockMarkedLayers(bool locked);

    Q_INVOKABLE void showLayer(const QString& layerId, bool shown);

    Q_INVOKABLE void lockLayer(const QString& layerId, bool locked);

    // A layer carried to another place in the order, counted from the bottom.
    Q_INVOKABLE void moveLayer(const QString& layerId, int to);

    // One thing on the page carried to another layer, which is how a picture is put over a table
    // or under it.
    Q_INVOKABLE void moveToLayer(const QString& thingId, const QString& layerId);

    // Whatever is picked up just now carried to another layer.
    Q_INVOKABLE void movePickedToLayer(const QString& layerId);

    // Which box of text the reader is working on, or nothing when none is.
    [[nodiscard]] QString pickedText() const { return m_pickedText; }

    void setPickedText(const QString& textId);

    // Everything the window needs to type in the box that is picked: where it stands in the
    // column of sheets, how wide it runs, what it says and the face it wears.
    [[nodiscard]] QVariantMap pickedBox() const;

    // Put an empty box where the reader tapped, in the coordinates of the column of sheets.
    Q_INVOKABLE void addTextAt(qreal columnX, qreal columnY, const QVariantMap& style);

    // Words put on the page in the middle of what is being looked at, settled at once rather than
    // left to be typed into: what the parts of the window that make words of their own hand over.
    Q_INVOKABLE void writeDown(const QString& said, const QVariantMap& style, bool formula);

    // The same words, put where they are asked for rather than in the middle of what is being
    // looked at.
    Q_INVOKABLE void writeDownAt(const QString& said, qreal columnX, qreal columnY,
                                 const QVariantMap& style);

    // A curve drawn on the page as ink, in a square in the middle of what is being looked at, with
    // its two axes. `runs` holds one list of points per run of the curve, counted the way
    // arithmetic counts, and `frame` says which part of the graph is being looked at.
    Q_INVOKABLE void drawCurve(const QVariantList& runs, const QVariantMap& frame,
                               const QColor& colour);

    // What a box says once the reader stops typing; a box left empty is dropped.
    Q_INVOKABLE void finishText(const QString& textId, const QString& text, qreal height);

    // Where a box stands and how much room it takes, after it was dragged or pulled wider.
    Q_INVOKABLE void placeText(const QString& textId, qreal columnX, qreal columnY, qreal width,
                               qreal height);

    Q_INVOKABLE void styleText(const QString& textId, const QVariantMap& style);

    Q_INVOKABLE void removeText(const QString& textId);

    Q_INVOKABLE [[nodiscard]] QVariantMap styleOfText(const QString& textId) const;

    // What a box says, asked for by name. The editor loads a box through this rather than from
    // whatever was last published, so that it can never come up holding nothing by mistake.
    Q_INVOKABLE [[nodiscard]] QString wordsOf(const QString& textId) const;

    // Read what is picked and put it on the page as text, taking the handwriting away.
    Q_INVOKABLE void convertSelectionToText(QVariantMap style);

    // Read what is picked as arithmetic, work it out, and write the answer beside it. The
    // handwriting stays where it is.
    Q_INVOKABLE void solveSelection(QVariantMap style);

    // A box of type put where the reader is looking, ready for a sum to be typed into it.
    Q_INVOKABLE void addEquation(const QVariantMap& style);

    // Put a picture from a file on the page being read, as large as it fits.
    Q_INVOKABLE void addPicture(const QUrl& fileUrl);

    // Whether this machine can read the words in a picture at all.
    Q_INVOKABLE [[nodiscard]] bool canReadPictures() const;

    Q_INVOKABLE [[nodiscard]] QStringList pictureLanguages() const;

    // Reads the words in a picture. What comes back arrives through `pictureRead`, because
    // reading takes long enough that nothing may wait for it. Asking again for a picture already
    // read hands back what was read before.
    Q_INVOKABLE void readPicture(const QString& pictureId, const QString& language);

    // What was read out of a picture before, empty where it has not been read.
    Q_INVOKABLE [[nodiscard]] QString wordsInPicture(const QString& pictureId) const;

    Q_INVOKABLE void forgetWordsInPicture(const QString& pictureId);

    // Each run of words read out of a picture and where it sits within it, as shares of its width
    // and height, so that the runs can be drawn over the picture whatever size it is shown at.
    Q_INVOKABLE [[nodiscard]] QVariantList wordsFoundInPicture(const QString& pictureId) const;

    // Reads every picture in this notebook that nothing has been read out of yet, one at a time, so
    // that searching reaches the words in all of them rather than only the ones asked about by
    // hand.
    Q_INVOKABLE void readEveryPicture(const QString& language);

    Q_INVOKABLE void giveUpReadingPictures();

    [[nodiscard]] int picturesToRead() const { return static_cast<int>(m_pictureQueue.size()); }

    [[nodiscard]] QString pickedPicture() const { return m_pickedPicture; }

    void setPickedPicture(const QString& pictureId);

    [[nodiscard]] QVariantMap pickedPictureBox() const;

    // Which picture stands under a point of the column of sheets, if any.
    Q_INVOKABLE [[nodiscard]] QString pictureUnder(qreal columnX, qreal columnY) const;

    // Rule a table on the page being read, as wide as most of the sheet.
    Q_INVOKABLE void addTable(int rows, int columns);

    [[nodiscard]] QString pickedTable() const { return m_pickedTable; }

    void setPickedTable(const QString& tableId);

    [[nodiscard]] QVariantMap pickedTableBox() const;

    // Which table stands under a point of the column of sheets, if any.
    Q_INVOKABLE [[nodiscard]] QString tableUnder(qreal columnX, qreal columnY) const;

    // Where a table has been carried and pulled to becomes where it stands, in one change.
    Q_INVOKABLE void placeTable(const QString& tableId, const QVariantMap& where);

    Q_INVOKABLE void removeTable(const QString& tableId);

    Q_INVOKABLE [[nodiscard]] QString wordsOfCell(const QString& tableId, int row,
                                                  int column) const;

    Q_INVOKABLE void writeCell(const QString& tableId, int row, int column, const QString& words);

    Q_INVOKABLE void addRow(const QString& tableId, int at);

    Q_INVOKABLE void addColumn(const QString& tableId, int at);

    Q_INVOKABLE void removeRow(const QString& tableId, int at);

    Q_INVOKABLE void removeColumn(const QString& tableId, int at);

    // Which box of a table a point of the column of sheets falls in, if any.
    Q_INVOKABLE [[nodiscard]] QVariantMap cellUnder(const QString& tableId, qreal columnX,
                                                    qreal columnY) const;

    // The columns and rows of a table measured out one by one, for a rule pulled about on its own.
    Q_INVOKABLE void spreadTable(const QString& tableId, const QVariantList& widths,
                                 const QVariantList& heights);

    Q_INVOKABLE void alignCell(const QString& tableId, int row, int column, int align);

    // Every box of a stretch of the table changed at once: how its words line up across the box
    // and down it, the colour behind it and the colour of its words, whether it is bold or
    // slanted, everything of its own let go of, and what is typed in it rubbed out.
    Q_INVOKABLE void alignCells(const QString& tableId, int fromRow, int fromColumn, int toRow,
                                int toColumn, int align);

    Q_INVOKABLE void riseCells(const QString& tableId, int fromRow, int fromColumn, int toRow,
                               int toColumn, int rise);

    Q_INVOKABLE void fillCells(const QString& tableId, int fromRow, int fromColumn, int toRow,
                               int toColumn, const QColor& fill);

    Q_INVOKABLE void inkCells(const QString& tableId, int fromRow, int fromColumn, int toRow,
                              int toColumn, const QColor& ink);

    Q_INVOKABLE void weighCells(const QString& tableId, int fromRow, int fromColumn, int toRow,
                                int toColumn, bool bold);

    Q_INVOKABLE void slantCells(const QString& tableId, int fromRow, int fromColumn, int toRow,
                                int toColumn, bool italic);

    Q_INVOKABLE void plainCells(const QString& tableId, int fromRow, int fromColumn, int toRow,
                                int toColumn);

    Q_INVOKABLE void emptyCells(const QString& tableId, int fromRow, int fromColumn, int toRow,
                                int toColumn);

    // A row or a column put down again just after itself, with everything its boxes say and are
    // shown in.
    Q_INVOKABLE void duplicateRow(const QString& tableId, int at);

    Q_INVOKABLE void duplicateColumn(const QString& tableId, int at);

    // The colour and thickness of the rules a whole table is drawn with.
    Q_INVOKABLE void ruleTable(const QString& tableId, const QColor& rule, qreal width);

    // How the box at a place is shown, for the bar of options to show back what is already set.
    Q_INVOKABLE [[nodiscard]] QVariantMap cellLook(const QString& tableId, int row,
                                                   int column) const;

    // Every box of a stretch of the table joined into one, and a joined box let go of again.
    Q_INVOKABLE void mergeCells(const QString& tableId, int fromRow, int fromColumn, int toRow,
                                int toColumn);

    Q_INVOKABLE void splitCell(const QString& tableId, int row, int column);

    // How far the box at a place reaches over the ones beside and below it.
    Q_INVOKABLE [[nodiscard]] QVariantMap cellSpan(const QString& tableId, int row,
                                                   int column) const;

    // Where a picture stands, how large it is drawn and how far it is turned. The change is read
    // from `columnX`, `columnY`, `boxWidth`, `boxHeight` and `turn`.
    Q_INVOKABLE void placePicture(const QString& pictureId, const QVariantMap& where);
    Q_INVOKABLE void removePicture(const QString& pictureId);

    Q_INVOKABLE void deleteSelection();
    Q_INVOKABLE void copySelection();

    // Copy and take away in one go, and copy beside itself without touching the clipboard.
    Q_INVOKABLE void cutSelection();
    Q_INVOKABLE void duplicateSelection();

    // Where what is picked stands, in the coordinates of the column of sheets.
    Q_INVOKABLE [[nodiscard]] QRectF selectionArea() const;

    // Turn what is picked about the middle of it, by so many degrees clockwise.
    Q_INVOKABLE void turnSelection(qreal degrees);

    // Move, size and turn what is picked. The change is read from `pivotX`, `pivotY` (the point
    // that stays still, in the coordinates of the column), `dx`, `dy`, `wide`, `tall` and `turn`.
    // While the reader is still dragging, `show` puts it on the page without remembering it; when
    // the drag ends, `apply` makes it a change that can be undone.
    Q_INVOKABLE void showTransform(const QVariantMap& change);
    Q_INVOKABLE void applyTransform(const QVariantMap& change);
    Q_INVOKABLE void dropTransform();

    // Read what is picked and put it on the clipboard as text.
    Q_INVOKABLE void copySelectionAsText();
    Q_INVOKABLE void pasteStrokes();

    Q_INVOKABLE void pasteStrokesAt(qreal x, qreal y);
    Q_INVOKABLE void recolourSelection(const QColor& color);

    Q_PROPERTY(bool hasCopiedStrokes READ hasCopiedStrokes NOTIFY clipboardChanged FINAL)

    [[nodiscard]] bool hasCopiedStrokes() const { return !m_clipboard.empty(); }

    Q_INVOKABLE void refreshTrash();
    Q_INVOKABLE void restoreTrashed(int index);
    Q_INVOKABLE void emptyTrash();

    Q_INVOKABLE void addSection();
    Q_INVOKABLE void deleteSection(int index);
    Q_INVOKABLE void moveSection(int from, int to);
    Q_INVOKABLE void renameSection(int index, const QString& title);

signals:
    void recordingsChanged();
    void linksChanged();
    void pictureRead(const QString& pictureId, const QString& words);
    void pictureUnread(const QString& pictureId, const QString& why);
    void readingPicture(const QString& pictureId);
    void readingPicturesChanged();
    // A link goes somewhere outside the application, and the window is to open it.
    void goingOut(const QUrl& where);
    void soundReady(const QString& recordingId, const QByteArray& sound);
    void soundMissing(const QString& recordingId);
    void notebookPathChanged();
    void readingChanged();
    void found(const QVariantList& words);
    void pointedWordChanged();
    void copiedAsText(const QString& text);
    void convertedToText(const QString& text);
    void pickedTextChanged();
    void pickedBoxChanged();
    void pickedPictureChanged();
    void pickedTableChanged();

    void layersChanged();
    void textAdded(const QString& textId);
    void startPageChanged();
    void canvasChanged();
    void markedLayersChanged();
    void loadedChanged();
    void errorMessageChanged();
    void pageChanged();
    void historyChanged();
    void outlineChanged();
    void currentPageChanged();
    void pageStyleChanged();
    void exportingChanged();
    void continuousChanged();
    void clipboardChanged();
    void colourPicked(const QColor& colour);
    void pageAdded(int index);
    void sectionAdded(int index);
    void exported(const QString& path);
    void copied(const QString& path);
    void saved(const QString& path);
    void keptAtChanged();
    void editedChanged();
    void nameWanted(const QString& name);
    void documentStarted();

private:
    class Sink final : public platform::ink::IInkSink {
    public:
        explicit Sink(NotebookViewModel* owner) noexcept : m_owner{owner} {}

        void strokeStarted(const core::InkSample& sample) override;
        void sampleAdded(const core::InkSample& sample) override;
        void strokeFinished(const core::InkSample& sample) override;
        void strokeCompleted(const core::Stroke& stroke, int sheet) override;
        void strokeCancelled() override;
        void eraserMoved(const core::InkSample& from, const core::InkSample& to, float radius,
                         core::EraseMode mode, int sheet) override;
        void eraseFinished() override;
        void selectionDrawn(std::span<const core::Point> shape) override;

        void thingTouched(const core::Point& at, float reach) override;
        void selectionMoved(float dx, float dy) override;
        void colourWanted(const core::InkSample& at, int sheet) override;
        void colourSeen(const core::Color& colour) override;

    private:
        NotebookViewModel* m_owner;
    };

    struct CopyJob {
        std::filesystem::path source;
        std::filesystem::path target;
        QString path;
    };

    struct ExportJob {
        std::filesystem::path notebook;
        std::filesystem::path target;
        QString path;
        platform::render::ExportScope scope{platform::render::ExportScope::Everything};
    };

    void openNotebook();
    void showLoadedOutline(std::uint64_t opening, core::Result<core::NotebookOutline> outline);
    void showLoadedPage(std::uint64_t opening, const core::Uuid& pageId,
                        core::Result<core::LoadedPage> loaded);
    void copyPage(const core::PageInfo& original, std::span<const core::PlacedStroke> strokes,
                  std::span<const core::PlacedText> texts);

    // A picture of a page being drawn away from the window, with what it is made of carried
    // along rather than pointed at, so that nothing can move under it while it is drawn.
    struct ThumbnailPicture {
        core::Picture placed;
        QImage picture;
        core::Uuid layer;
    };

    struct ThumbnailWork {
        core::PageInfo page;
        std::vector<core::PlacedStroke> strokes;
        std::vector<core::PlacedText> texts;
        std::vector<ThumbnailPicture> pictures;
        std::vector<core::PlacedTable> tables;
        std::vector<core::Layer> layers;
        QImage media;
    };

    void takePictures(ThumbnailWork& work, std::span<const core::PlacedPicture> pictures) const;
    void gatherThumbnail(const std::shared_ptr<ThumbnailWork>& work);
    void thumbnailAsset(const std::shared_ptr<ThumbnailWork>& work, core::Asset asset);
    void thumbnailPage(const std::shared_ptr<ThumbnailWork>& work,
                       const platform::pdf::PageImage& image);
    void paintThumbnail(const ThumbnailWork& work);
    void paintLayerPreview(const ThumbnailWork& work, const core::Uuid& layerId);
    void forgetThumbnail(const core::Uuid& pageId);
    void forgetLayerPreviews();
    void goToPage(const core::Uuid& pageId);
    void goToPlace(std::size_t section, std::size_t page);
    void setLoaded(bool loaded);

    [[nodiscard]] core::Page* currentPageData();
    [[nodiscard]] const core::Page* currentPageData() const;
    [[nodiscard]] const core::PageInfo* currentPageInfo() const;
    [[nodiscard]] std::optional<core::PagePlace> currentPlace() const;
    [[nodiscard]] std::optional<std::size_t> currentSectionIndex() const;
    void changeStyle(const core::PageStyle& style);
    void changeStyleOfPage(const core::PageStyle& style);

    void storeStroke(const core::Stroke& stroke, int sheet);
    // Which page a sheet of the column is, or the page being read when the sheet is not one.
    [[nodiscard]] core::Uuid pageOfSheet(int sheet) const;
    void selectInside(std::span<const core::Point> polygon);

    void pickUnder(const core::Point& at, float reach);
    void moveSelection(float dx, float dy);
    void pickColour(const core::InkSample& at, int sheet);
    void showPickedColour(const core::Color& colour);
    void pickFromMedia(const core::InkSample& at);
    [[nodiscard]] bool noteTouched(const core::Page& page, const core::EraserSweep& sweep,
                                   bool whole);
    void erase(const core::InkSample& from, const core::InkSample& to, float radius,
               core::EraseMode mode, int sheet);
    void finishErasing();
    void runCommand(std::unique_ptr<core::ICommand> command);
    void paste(std::optional<core::Point> at);

    void finishChange(const core::Result<void>& change, std::optional<core::Uuid> pageToShow,
                      bool redrawsPage = true);
    [[nodiscard]] QString freePageName(std::size_t section) const;
    void nameEveryPage(std::size_t section);
    void publishOutline();
    void dropStartingPage();
    void markEdited();
    void publishFound(std::vector<core::FoundWord> hits);
    void pointAtWhatWasFound();

    // A place on a page, found from a point in the column of sheets.
    struct TextPlace {
        core::Uuid page;
        core::Point at;
        int sheet{};
    };

    // A box that was only just put down: it becomes part of the page, and of what can be undone,
    // once something is typed in it.
    struct Draft {
        core::Uuid page;
        core::TextBox box;
    };

    [[nodiscard]] core::Transform transformOf(const QVariantMap& change) const;
    [[nodiscard]] std::vector<core::Stroke> pickedStrokes() const;
    // What the canvas must leave out, and what it must draw instead, while ink is being rubbed
    // out or dragged about.
    [[nodiscard]] std::vector<core::Uuid> setAside() const;
    [[nodiscard]] std::vector<core::Uuid> outOfReach() const;
    [[nodiscard]] std::vector<core::Stroke> standingIn() const;
    [[nodiscard]] std::optional<TextPlace> placeInColumn(QPointF column) const;
    void putHandful(const Handful& handful, const std::optional<core::Point>& corner);
    void settleDraft();
    [[nodiscard]] int sheetOfPage(const core::Uuid& pageId) const;
    [[nodiscard]] int sheetCount() const;
    // The page a box of text belongs to, and the box itself.
    [[nodiscard]] std::optional<std::pair<core::Uuid, core::TextBox>>
    textById(const QString& textId) const;
    void changeText(const core::Uuid& pageId, core::TextBox box);
    void publishTexts();
    // The page a picture stands on, and the picture itself.
    [[nodiscard]] std::optional<std::pair<core::Uuid, core::Picture>>
    pictureById(const QString& pictureId) const;
    void changePicture(const core::Uuid& pageId, core::Picture picture);
    // Asks for whatever a page's pictures are made of, and draws them once it is there.
    void wantPicturesFor(const core::Page& page);
    void usePictureAsset(core::Result<core::Asset> asset);
    void publishPictures();
    // The page a table stands on, and the table itself.
    [[nodiscard]] std::optional<std::pair<core::Uuid, core::Table>>
    tableById(const QString& tableId) const;
    void changeTable(const core::Uuid& pageId, core::Table table);
    // A table with one row or column more or less, put down as one change.
    void changeRange(const QString& tableId, int fromRow, int fromColumn, int toRow, int toColumn,
                     const std::function<core::Result<core::Table>(core::Table, core::CellAt,
                                                                   core::CellAt)>& change);

    void reshapeTable(const QString& tableId,
                      const std::function<core::Result<core::Table>(core::Table)>& reshaped);
    void publishTables();

    void publishLayers();
    void changeMarkedLayers(const std::function<bool(core::Layer&)>& change);
    void publishRecordings();
    void publishLinks();
    [[nodiscard]] const core::Link* linkNamed(const QString& linkId) const;
    [[nodiscard]] const core::Picture* pictureNamed(const QString& pictureId) const;
    void readWhatIsInPicture(const QString& pictureId, const core::ContentId& source,
                             std::uint64_t opening, const QString& language,
                             core::Result<core::Asset> asset);
    void hearWhatIsInPicture(const QString& pictureId, const core::ContentId& source,
                             std::uint64_t opening,
                             core::Result<std::vector<platform::ocr::Found>> found);
    void askTheReaderAboutPicture(const QString& pictureId, const core::ContentId& source,
                                  const QString& language);
    void keepWhatIsInPicture(const core::ContentId& source,
                             std::span<const platform::ocr::Found> found);
    void readTheNextPicture(const QString& language);
    void hearTheNextPicture(const core::ContentId& source, const QString& language,
                            std::uint64_t opening,
                            core::Result<std::vector<platform::ocr::Found>> found);
    [[nodiscard]] std::optional<core::Rect> areaOfPickedInk(const core::Page& page) const;
    [[nodiscard]] std::optional<core::Rect> areaOfPickedThing(const core::Page& page) const;
    // The moment a thing was put on the page, where a recording is running.
    void noteTheMoment(const core::Uuid& thing);
    [[nodiscard]] const core::Recording* recordingNamed(const QString& recordingId) const;

    // The layers of the page being read, and the layer anything new is put on.
    [[nodiscard]] std::vector<core::Layer> layersHere() const;

    // The layers a page is opened with: the ones written down, or one named in the language the
    // reader is being spoken to, because a page written before there were layers has none.
    [[nodiscard]] std::vector<core::Layer> layersOrOne(std::vector<core::Layer> layers);

    [[nodiscard]] core::Uuid layerForNewThings() const;

    [[nodiscard]] core::Uuid layerForNewThings(const core::Page& page) const;

    void changeLayers(std::vector<core::Layer> wanted);

    void letGoOfWhatIsShut();

    [[nodiscard]] std::vector<core::Uuid> whatIsInHand() const;

    [[nodiscard]] bool canPutSomethingDown();

    // What was read as arithmetic, worked out and written beside the hand that asked it.
    void answerWhatWasAsked(const core::Uuid& pageId, std::span<const core::InkWord> words,
                            const QVariantMap& style);
    void solveWhatIsTyped();
    void markAsFormula(const QString& textId);
    [[nodiscard]] core::TextBox answerBeside(const core::TextBlock& asked,
                                             const std::string& answer, core::TextStyle face);
    void readKeptAt();
    [[nodiscard]] bool writeTo(const QString& path);
    void refreshCanvas();
    void refreshMedia();
    void showAsset(std::uint64_t opening, core::Result<core::Asset> asset);
    void showPicture(std::uint64_t opening, const core::ContentId& asset, const QImage& picture);
    void drawMedia();
    void redrawMedia();
    void showPageMedia(const core::Uuid& page, const QImage& picture, const QRectF& area);
    void publishMedia();
    void wantNeighbours();
    void wantMediaFor(const core::PageInfo& page);
    [[nodiscard]] bool hasWholeMedia(const core::PageInfo& page) const;
    // How big a page is: its own paper, or, where it has none, the document it carries.
    [[nodiscard]] std::optional<core::PaperSize> sizeOfPage(const core::PageInfo& page) const;
    void rememberDocument(const core::ContentId& asset,
                          const std::vector<platform::pdf::PageSize>& sizes);
    void forgetFarMedia(std::span<const core::PageInfo> pages, int here);
    [[nodiscard]] qreal drawScale(const core::PaperSize& paper, qreal least) const;
    [[nodiscard]] qreal columnScale(const core::PaperSize& paper) const;
    void drawColumnMedia(const core::Uuid& page, const core::PageStyle& style, int index,
                         core::Asset asset);
    void drawColumnPage(const core::Uuid& page, int index, const core::ContentId& asset);
    void showColumn();
    void goToShownPage(int index);
    void reportError(const QString& message);
    void finishExport(const QString& path, const core::Result<int>& written);
    void showTrash(std::uint64_t opening, core::Result<std::vector<core::TrashedItem>> items);
    void reloadOutline();
    void applyOutline(std::uint64_t opening, core::Result<core::NotebookOutline> outline);

    Sink m_sink{this};
    core::Uuid7Generator m_ids;
    core::Outline m_outline;
    std::map<core::Uuid, std::unique_ptr<core::Page>> m_pages;
    std::optional<core::StorageThread> m_storage;
    core::UndoStack m_history;
    std::vector<core::Uuid> m_erasing;
    std::vector<core::EraserSweep> m_sweeps;
    std::map<core::Uuid, std::vector<core::Stroke>> m_erasePieces;
    std::optional<platform::pdf::PdfRenderer> m_pdf;
    core::ContentId m_openAsset;
    std::map<core::Uuid, platform::ink::QtInkItem::MediaPiece> m_shownMedia;
    std::map<core::Uuid, qreal> m_drawnAt;
    std::map<core::ContentId, std::vector<platform::pdf::PageSize>> m_documentSizes;
    std::set<core::Uuid> m_wantedMedia;
    std::set<core::Uuid> m_drawing;
    std::set<core::Uuid> m_wantedPages;
    bool m_continuous{false};
    QTimer m_mediaTimer;
    qreal m_mediaScale{0.0};
    std::jthread m_export;
    std::jthread m_pictures;
    bool m_exporting{false};
    core::Uuid m_currentPage;
    core::Uuid m_startingPage;
    core::Uuid m_erasedPage;
    QString m_keptAt;
    HandwritingReader m_reader{this};
    std::vector<core::FoundWord> m_found;
    std::optional<core::FoundWord> m_pointed;
    QVariantMap m_pointedWord;
    int m_pagesToRead{0};
    bool m_edited{false};
    std::map<core::Uuid, core::Viewport> m_views;
    std::map<core::Uuid, int> m_thumbnails;
    std::map<core::Uuid, int> m_layerPreviews;
    // What has been read out of the pictures on this page. It is worked out again whenever it is
    // wanted after the notebook is closed, because it is made from the picture rather than
    // written by the reader.
    std::map<core::ContentId, QString> m_pictureWords;
    std::vector<core::ContentId> m_pictureQueue;
    std::map<core::ContentId, std::vector<core::PictureWord>> m_pictureFound;
    std::unique_ptr<platform::ocr::IReadPicture> m_pictureReader;
    // Asked of the reader once, because a reader is there on every machine and a language for it to
    // read is not.
    QStringList m_pictureLanguages;
    RecordingListModel m_recordingsModel;
    SayingListModel m_sayingsModel;
    QString m_shownRecording;
    core::Uuid m_markingInto;
    qint64 m_markingAt{0};
    int m_thumbnailRevision{0};
    std::vector<core::Stroke> m_clipboard;
    std::vector<core::TrashedItem> m_trashed;
    TrashListModel m_trashModel;
    TextListModel m_textsModel;
    TableListModel m_tablesModel;
    LayerListModel m_layersModel;
    QString m_activeLayer;
    QStringList m_markedLayers;
    QString m_pickedTable;
    std::map<core::ContentId, QImage> m_pictureImages;
    std::set<core::ContentId> m_wantedPictures;
    QString m_pickedPicture;
    std::vector<core::Uuid> m_previewIds;
    std::vector<core::Stroke> m_preview;
    std::optional<Draft> m_draft;
    QString m_pickedText;
    OutlineListModel m_sectionsModel;
    OutlineListModel m_pagesModel;
    QPointer<platform::ink::QtInkItem> m_canvas;
    QString m_notebookPath;
    QString m_startPage;
    QString m_errorMessage;
    std::uint64_t m_opening{0};
    bool m_completed{false};
    bool m_loaded{false};
};

}
