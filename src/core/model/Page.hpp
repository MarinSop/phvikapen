#pragma once

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/ink/Stroke.hpp"
#include "core/ink/StrokeHitTest.hpp"
#include "core/model/Picture.hpp"
#include "core/model/StrokeGrid.hpp"
#include "core/model/TextBox.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace phvikapen::core {

struct PlacedStroke {
    std::int64_t ordinal{};
    Stroke stroke;
};

class Page {
public:
    explicit Page(const Uuid& id) noexcept;
    Page(const Uuid& id, std::vector<PlacedStroke> strokes, std::vector<PlacedText> texts = {},
         std::vector<PlacedPicture> pictures = {});

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

private:
    Uuid m_id;
    std::vector<PlacedStroke> m_strokes;
    std::vector<PlacedText> m_texts;
    std::vector<PlacedPicture> m_pictures;
    StrokeGrid m_grid;
};

}
