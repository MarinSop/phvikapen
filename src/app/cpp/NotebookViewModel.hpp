#pragma once

#include "app/cpp/OutlineModels.hpp"
#include "core/Error.hpp"
#include "core/geometry/Distance.hpp"
#include "core/geometry/Rect.hpp"
#include "core/geometry/Viewport.hpp"
#include "core/id/ContentId.hpp"
#include "core/id/Uuid.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"
#include "core/ink/StrokeHitTest.hpp"
#include "core/model/Asset.hpp"
#include "core/model/Outline.hpp"
#include "core/model/Page.hpp"
#include "core/model/PageStyle.hpp"
#include "core/storage/StorageThread.hpp"
#include "core/undo/UndoStack.hpp"
#include "platform/ink/IInkBackend.hpp"
#include "platform/ink/qt/QtInkItem.hpp"
#include "platform/pdf/PdfRenderer.hpp"
#include "platform/render/PdfExporter.hpp"

#include <QColor>
#include <QImage>
#include <QObject>
#include <QPointer>
#include <QQmlParserStatus>
#include <QString>
#include <QTimer>
#include <QUrl>
#include <QtQmlIntegration>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <thread>
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
    Q_PROPERTY(bool loaded READ loaded NOTIFY loadedChanged FINAL)
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
    Q_PROPERTY(bool exporting READ exporting NOTIFY exportingChanged FINAL)
    Q_PROPERTY(bool continuous READ continuous WRITE setContinuous NOTIFY continuousChanged FINAL)
    Q_PROPERTY(phvikapen::app::TrashListModel* trash READ trash CONSTANT FINAL)

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
    Q_INVOKABLE void duplicatePage(int index);
    Q_INVOKABLE void wantThumbnail(int index);

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

    Q_INVOKABLE void deleteSelection();
    Q_INVOKABLE void copySelection();
    Q_INVOKABLE void pasteStrokes();
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
    void notebookPathChanged();
    void startPageChanged();
    void canvasChanged();
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
        void strokeCompleted(const core::Stroke& stroke) override;
        void strokeCancelled() override;
        void eraserMoved(const core::InkSample& from, const core::InkSample& to,
                         float radius) override;
        void eraseFinished() override;
        void selectionDrawn(std::span<const core::Point> shape) override;
        void selectionMoved(float dx, float dy) override;
        void colourWanted(const core::InkSample& at) override;

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
                        core::Result<std::vector<core::PlacedStroke>> strokes);
    void copyPage(const core::PageInfo& original, std::span<const core::PlacedStroke> strokes);

    struct ThumbnailWork {
        core::PageInfo page;
        std::vector<core::PlacedStroke> strokes;
        QImage media;
    };

    void gatherThumbnail(const std::shared_ptr<ThumbnailWork>& work);
    void thumbnailAsset(const std::shared_ptr<ThumbnailWork>& work, core::Asset asset);
    void thumbnailPage(const std::shared_ptr<ThumbnailWork>& work,
                       const platform::pdf::PageImage& image);
    void paintThumbnail(const ThumbnailWork& work);
    void forgetThumbnail(const core::Uuid& pageId);
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

    void storeStroke(const core::Stroke& stroke);
    void selectInside(std::span<const core::Point> polygon);
    void moveSelection(float dx, float dy);
    void pickColour(const core::InkSample& at);
    void pickFromMedia(const core::InkSample& at);
    void erase(const core::InkSample& from, const core::InkSample& to, float radius);
    void finishErasing();
    void runCommand(std::unique_ptr<core::ICommand> command);
    void finishChange(const core::Result<void>& change, std::optional<core::Uuid> pageToShow);
    [[nodiscard]] QString freePageName(std::size_t section) const;
    void nameEveryPage(std::size_t section);
    void publishOutline();
    void dropStartingPage();
    void markEdited();
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
    QString m_keptAt;
    bool m_edited{false};
    std::map<core::Uuid, core::Viewport> m_views;
    std::map<core::Uuid, int> m_thumbnails;
    int m_thumbnailRevision{0};
    std::vector<core::Stroke> m_clipboard;
    std::vector<core::TrashedItem> m_trashed;
    TrashListModel m_trashModel;
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
