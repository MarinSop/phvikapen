#pragma once

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/ink/Stroke.hpp"
#include "core/ink/StrokeHitTest.hpp"
#include "core/model/Layer.hpp"
#include "core/model/Link.hpp"
#include "core/model/Picture.hpp"
#include "core/model/Recording.hpp"
#include "core/model/StrokeGrid.hpp"
#include "core/model/Table.hpp"
#include "core/model/TextBox.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace phvikapen::core {

struct PlacedStroke {
    std::int64_t ordinal{};
    Stroke stroke;
    Uuid layer{kNilUuid};
};

class Page {
public:
    explicit Page(const Uuid& id);
    Page(const Uuid& id, std::vector<PlacedStroke> strokes, std::vector<PlacedText> texts = {},
         std::vector<PlacedPicture> pictures = {}, std::vector<PlacedTable> tables = {},
         std::vector<Layer> layers = {});

    [[nodiscard]] const Uuid& id() const noexcept { return m_id; }

    [[nodiscard]] std::span<const PlacedStroke> strokes() const noexcept { return m_strokes; }

    [[nodiscard]] std::int64_t nextOrdinal() const noexcept;

    [[nodiscard]] Result<void> insert(PlacedStroke placed);

    [[nodiscard]] Result<PlacedStroke> remove(const Uuid& strokeId);

    [[nodiscard]] std::vector<PlacedStroke> takeAll() noexcept;

    [[nodiscard]] std::vector<PlacedText> takeAllTexts() noexcept;

    [[nodiscard]] std::vector<Uuid> strokesTouchedBy(const EraserSweep& sweep) const;

    [[nodiscard]] std::span<const PlacedText> texts() const noexcept { return m_texts; }

    [[nodiscard]] std::int64_t nextTextOrdinal() const noexcept;

    [[nodiscard]] Result<void> insertText(PlacedText placed);

    [[nodiscard]] Result<PlacedText> removeText(const Uuid& textId);

    [[nodiscard]] Result<void> replaceText(TextBox box);

    [[nodiscard]] const TextBox* textAt(const Uuid& textId) const noexcept;

    // The box a tap lands in: the last one written, where several lie over one another.
    [[nodiscard]] const TextBox* textUnder(Point at) const noexcept;

    // Pictures stand under the ink, so that what is written on one stays on top of it.
    [[nodiscard]] std::span<const PlacedPicture> pictures() const noexcept { return m_pictures; }

    [[nodiscard]] std::int64_t nextPictureOrdinal() const noexcept;

    [[nodiscard]] Result<void> insertPicture(PlacedPicture placed);

    [[nodiscard]] Result<PlacedPicture> removePicture(const Uuid& pictureId);

    [[nodiscard]] Result<void> replacePicture(Picture picture);

    [[nodiscard]] const Picture* pictureAt(const Uuid& pictureId) const noexcept;

    // The picture a tap lands on: the last one put down, where several lie over one another.
    [[nodiscard]] const Picture* pictureUnder(Point at) const noexcept;

    [[nodiscard]] std::vector<PlacedPicture> takeAllPictures() noexcept;

    // Tables stand over the ink, as typed text does: a table is a thing in its own right, and its
    // boxes are empty of everything but what is typed into them.
    [[nodiscard]] std::span<const PlacedTable> tables() const noexcept { return m_tables; }

    [[nodiscard]] std::int64_t nextTableOrdinal() const noexcept;

    [[nodiscard]] Result<void> insertTable(PlacedTable placed);

    [[nodiscard]] Result<PlacedTable> removeTable(const Uuid& tableId);

    [[nodiscard]] Result<void> replaceTable(Table table);

    [[nodiscard]] const Table* tableAt(const Uuid& tableId) const noexcept;

    // The table a tap lands in: the last one put down, where several lie over one another.
    [[nodiscard]] const Table* tableUnder(Point at) const noexcept;

    [[nodiscard]] std::vector<PlacedTable> takeAllTables() noexcept;

    // The patches of the page that take a reader somewhere else.
    [[nodiscard]] std::span<const PlacedLink> links() const noexcept { return m_links; }

    [[nodiscard]] std::int64_t nextLinkOrdinal() const noexcept;

    [[nodiscard]] Result<void> insertLink(PlacedLink placed);

    [[nodiscard]] Result<PlacedLink> removeLink(const Uuid& linkId);

    [[nodiscard]] Result<void> replaceLink(Link link);

    [[nodiscard]] const Link* linkAt(const Uuid& linkId) const noexcept;

    // The link a tap lands on: the last one put down, where several lie over one another, and
    // never one on a layer that is hidden or held still.
    [[nodiscard]] const Link* linkUnder(Point at) const noexcept;

    [[nodiscard]] std::vector<PlacedLink> takeAllLinks() noexcept;

    void setLinks(std::vector<PlacedLink> links);

    // The layers of the page, bottom first. A page always has at least one: everything written
    // down before there were layers belongs to it.
    [[nodiscard]] std::span<const Layer> layers() const noexcept { return m_layers; }

    void setLayers(std::vector<Layer> layers);

    // The recordings made while this page was being written on, oldest first.
    [[nodiscard]] std::span<const Recording> recordings() const noexcept { return m_recordings; }

    void setRecordings(std::vector<Recording> recordings);

    [[nodiscard]] const Recording* recording(const Uuid& id) const noexcept;

    // What was said in a recording, and how far the reading of it has got.
    [[nodiscard]] Result<void> setSaid(const Uuid& id, Said said);

    [[nodiscard]] Result<void> addRecording(Recording made);

    [[nodiscard]] Result<Recording> removeRecording(const Uuid& id);

    [[nodiscard]] Result<void> changeRecording(const Uuid& id, Recording made);

    // What was written down while a recording was running, and when.
    [[nodiscard]] std::span<const Mark> marks() const noexcept { return m_marks; }

    void setMarks(std::vector<Mark> marks);

    // A thing tied to a moment in a recording. A thing already tied to that recording is tied
    // again at the same moment rather than twice.
    [[nodiscard]] Result<void> addMark(Mark mark);

    // Everything tied to a recording, let go of at once: what is done when the recording goes.
    void forgetMarksOf(const Uuid& recording);

    // Everything tied to a thing, let go of at once: what is done when the thing goes.
    void forgetMarksOfThing(const Uuid& thing);

    // Which layer a thing on the page belongs to, whether it is a stroke of ink, a box of type, a
    // picture or a table. What comes back is the layer it stood on before, so that the move can be
    // taken back exactly.
    [[nodiscard]] Result<Uuid> moveToLayer(const Uuid& thingId, const Uuid& layerId);

    // What stands on a layer, counted so that the panel can say whether one is empty.
    [[nodiscard]] int countOnLayer(const Uuid& layerId) const noexcept;

    [[nodiscard]] std::optional<Uuid> layerOfThing(const Uuid& thingId) const noexcept;

    [[nodiscard]] bool isThingOpenToTheHand(const Uuid& thingId) const noexcept;

private:
    Uuid m_id;
    std::vector<PlacedStroke> m_strokes;
    std::vector<PlacedText> m_texts;
    std::vector<PlacedPicture> m_pictures;
    std::vector<PlacedTable> m_tables;
    std::vector<Layer> m_layers;
    std::vector<Recording> m_recordings;
    std::vector<Mark> m_marks;
    std::vector<PlacedLink> m_links;
    StrokeGrid m_grid;
};

}
