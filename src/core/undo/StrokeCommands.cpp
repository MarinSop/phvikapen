#include "core/undo/StrokeCommands.hpp"

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"
#include "core/ink/StrokeSelection.hpp"
#include "core/ink/StrokeTransform.hpp"
#include "core/model/Page.hpp"
#include "core/model/TextBox.hpp"
#include "core/storage/NotebookStore.hpp"
#include "core/storage/StorageThread.hpp"

#include <cstddef>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

namespace phvikapen::core {
namespace {

[[nodiscard]] Result<void> restoreStrokes(Page& page, StorageThread& storage,
                                          std::vector<PlacedStroke> strokes) {
    for (PlacedStroke& placed : strokes) {
        if (const Result<void> inserted = page.insert(placed); !inserted) {
            return inserted;
        }
        storage.insertStroke(page.id(), std::move(placed));
    }
    return {};
}

}

AddStrokeCommand::AddStrokeCommand(Page* page, StorageThread* storage, PlacedStroke placed) noexcept
    : m_page{page}, m_storage{storage}, m_placed{std::move(placed)} {}

Result<void> AddStrokeCommand::apply() {
    if (const Result<void> inserted = m_page->insert(m_placed); !inserted) {
        return inserted;
    }
    m_storage->insertStroke(m_page->id(), m_placed);
    return {};
}

Result<void> AddStrokeCommand::revert() {
    if (const Result<PlacedStroke> removed = m_page->remove(m_placed.stroke.id()); !removed) {
        return std::unexpected{removed.error()};
    }
    m_storage->removeStroke(m_page->id(), m_placed.stroke.id());
    return {};
}

ClearPageCommand::ClearPageCommand(Page* page, StorageThread* storage) noexcept
    : m_page{page}, m_storage{storage} {}

Result<void> ClearPageCommand::apply() {
    m_removed = m_page->takeAll();
    m_removedTexts = m_page->takeAllTexts();
    m_storage->removeStrokesOfPage(m_page->id());
    m_storage->submit([pageId = m_page->id()](NotebookStore& store) {
        return store.removeTextsOfPage(pageId).transform([](std::size_t) {});
    });
    return {};
}

Result<void> ClearPageCommand::revert() {
    for (PlacedText& placed : std::exchange(m_removedTexts, {})) {
        if (const Result<void> inserted = m_page->insertText(placed); !inserted) {
            return inserted;
        }
        m_storage->submit([pageId = m_page->id(), placed](NotebookStore& store) {
            return store.insertText(pageId, placed);
        });
    }
    return restoreStrokes(*m_page, *m_storage, std::exchange(m_removed, {}));
}

EraseStrokesCommand::EraseStrokesCommand(Page* page, StorageThread* storage,
                                         std::vector<Uuid> strokeIds) noexcept
    : m_page{page}, m_storage{storage}, m_strokeIds{std::move(strokeIds)} {}

MoveStrokesCommand::MoveStrokesCommand(Page* page, StorageThread* storage,
                                       std::vector<Uuid> strokeIds, float dx, float dy) noexcept
    : m_page{page}, m_storage{storage}, m_strokeIds{std::move(strokeIds)}, m_dx{dx}, m_dy{dy} {}

Result<void> MoveStrokesCommand::shift(float dx, float dy) {
    std::vector<PlacedStroke> moved;
    moved.reserve(m_strokeIds.size());
    for (const Uuid& strokeId : m_strokeIds) {
        Result<PlacedStroke> taken = m_page->remove(strokeId);
        if (!taken) {
            for (PlacedStroke& placed : moved) {
                std::ignore = m_page->insert(std::move(placed));
            }
            return std::unexpected{taken.error()};
        }
        moved.push_back(PlacedStroke{
            .ordinal = taken->ordinal,
            .stroke = core::moved(taken->stroke, dx, dy),
        });
    }

    for (PlacedStroke& placed : moved) {
        const Uuid strokeId = placed.stroke.id();
        if (const Result<void> put = m_page->insert(placed); !put) {
            return put;
        }
        m_storage->removeStroke(m_page->id(), strokeId);
        m_storage->insertStroke(m_page->id(), std::move(placed));
    }
    return {};
}

Result<void> MoveStrokesCommand::apply() {
    return shift(m_dx, m_dy);
}

Result<void> MoveStrokesCommand::revert() {
    return shift(-m_dx, -m_dy);
}

std::optional<Uuid> MoveStrokesCommand::pageToShow() const {
    return m_page->id();
}

TransformStrokesCommand::TransformStrokesCommand(Page* page, StorageThread* storage,
                                                 std::vector<Uuid> strokeIds,
                                                 Transform transform) noexcept
    : m_page{page}, m_storage{storage}, m_strokeIds{std::move(strokeIds)}, m_transform{transform} {}

Result<void> TransformStrokesCommand::apply() {
    std::vector<PlacedStroke> before;
    std::vector<PlacedStroke> after;
    before.reserve(m_strokeIds.size());
    after.reserve(m_strokeIds.size());

    for (const Uuid& strokeId : m_strokeIds) {
        Result<PlacedStroke> taken = m_page->remove(strokeId);
        if (!taken) {
            for (PlacedStroke& placed : before) {
                std::ignore = m_page->insert(std::move(placed));
            }
            return std::unexpected{taken.error()};
        }
        after.push_back(PlacedStroke{
            .ordinal = taken->ordinal,
            .stroke = transformed(taken->stroke, m_transform),
        });
        before.push_back(std::move(*taken));
    }

    for (PlacedStroke& placed : after) {
        const Uuid strokeId = placed.stroke.id();
        if (const Result<void> put = m_page->insert(placed); !put) {
            return put;
        }
        m_storage->removeStroke(m_page->id(), strokeId);
        m_storage->insertStroke(m_page->id(), std::move(placed));
    }
    m_before = std::move(before);
    return {};
}

Result<void> TransformStrokesCommand::revert() {
    for (const PlacedStroke& placed : m_before) {
        if (const Result<PlacedStroke> taken = m_page->remove(placed.stroke.id()); !taken) {
            return std::unexpected{taken.error()};
        }
        m_storage->removeStroke(m_page->id(), placed.stroke.id());
    }
    std::vector<PlacedStroke> back = std::exchange(m_before, {});
    return restoreStrokes(*m_page, *m_storage, std::move(back));
}

std::optional<Uuid> TransformStrokesCommand::pageToShow() const {
    return m_page->id();
}

AddStrokesCommand::AddStrokesCommand(Page* page, StorageThread* storage,
                                     std::vector<PlacedStroke> placed) noexcept
    : m_page{page}, m_storage{storage}, m_placed{std::move(placed)} {}

Result<void> AddStrokesCommand::apply() {
    std::vector<PlacedStroke> copies = m_placed;
    return restoreStrokes(*m_page, *m_storage, std::move(copies));
}

Result<void> AddStrokesCommand::revert() {
    for (const PlacedStroke& placed : m_placed) {
        if (const Result<PlacedStroke> taken = m_page->remove(placed.stroke.id()); !taken) {
            return std::unexpected{taken.error()};
        }
        m_storage->removeStroke(m_page->id(), placed.stroke.id());
    }
    return {};
}

std::optional<Uuid> AddStrokesCommand::pageToShow() const {
    return m_page->id();
}

RestyleStrokesCommand::RestyleStrokesCommand(Page* page, StorageThread* storage,
                                             std::vector<Uuid> strokeIds,
                                             StrokeStyle style) noexcept
    : m_page{page}, m_storage{storage}, m_strokeIds{std::move(strokeIds)}, m_style{style} {}

Result<void> RestyleStrokesCommand::apply() {
    std::vector<PlacedStroke> before;
    std::vector<PlacedStroke> after;
    before.reserve(m_strokeIds.size());
    after.reserve(m_strokeIds.size());

    for (const Uuid& strokeId : m_strokeIds) {
        Result<PlacedStroke> taken = m_page->remove(strokeId);
        if (!taken) {
            for (PlacedStroke& placed : before) {
                std::ignore = m_page->insert(std::move(placed));
            }
            return std::unexpected{taken.error()};
        }
        Stroke restyled{taken->stroke.id(), m_style};
        for (const InkSample& sample : taken->stroke.samples()) {
            restyled.append(sample);
        }
        after.push_back(PlacedStroke{.ordinal = taken->ordinal, .stroke = std::move(restyled)});
        before.push_back(std::move(*taken));
    }

    for (PlacedStroke& placed : after) {
        const Uuid strokeId = placed.stroke.id();
        if (const Result<void> put = m_page->insert(placed); !put) {
            return put;
        }
        m_storage->removeStroke(m_page->id(), strokeId);
        m_storage->insertStroke(m_page->id(), std::move(placed));
    }
    m_before = std::move(before);
    return {};
}

Result<void> RestyleStrokesCommand::revert() {
    for (const PlacedStroke& placed : m_before) {
        if (const Result<PlacedStroke> taken = m_page->remove(placed.stroke.id()); !taken) {
            return std::unexpected{taken.error()};
        }
        m_storage->removeStroke(m_page->id(), placed.stroke.id());
    }
    std::vector<PlacedStroke> back = std::exchange(m_before, {});
    return restoreStrokes(*m_page, *m_storage, std::move(back));
}

std::optional<Uuid> RestyleStrokesCommand::pageToShow() const {
    return m_page->id();
}

SplitStrokesCommand::SplitStrokesCommand(Page* page, StorageThread* storage,
                                         std::vector<Uuid> strokeIds,
                                         std::vector<PlacedStroke> pieces) noexcept
    : m_page{page}, m_storage{storage}, m_strokeIds{std::move(strokeIds)},
      m_pieces{std::move(pieces)} {}

Result<void> SplitStrokesCommand::apply() {
    std::vector<PlacedStroke> erased;
    erased.reserve(m_strokeIds.size());
    for (const Uuid& strokeId : m_strokeIds) {
        Result<PlacedStroke> taken = m_page->remove(strokeId);
        if (!taken) {
            for (PlacedStroke& placed : erased) {
                std::ignore = m_page->insert(std::move(placed));
            }
            return std::unexpected{taken.error()};
        }
        erased.push_back(std::move(*taken));
        m_storage->removeStroke(m_page->id(), strokeId);
    }
    m_erased = std::move(erased);

    std::vector<PlacedStroke> copies = m_pieces;
    return restoreStrokes(*m_page, *m_storage, std::move(copies));
}

Result<void> SplitStrokesCommand::revert() {
    for (const PlacedStroke& placed : m_pieces) {
        if (const Result<PlacedStroke> taken = m_page->remove(placed.stroke.id()); !taken) {
            return std::unexpected{taken.error()};
        }
        m_storage->removeStroke(m_page->id(), placed.stroke.id());
    }
    return restoreStrokes(*m_page, *m_storage, std::exchange(m_erased, {}));
}

std::optional<Uuid> SplitStrokesCommand::pageToShow() const {
    return m_page->id();
}

Result<void> EraseStrokesCommand::apply() {
    std::vector<PlacedStroke> erased;
    erased.reserve(m_strokeIds.size());
    for (const Uuid& strokeId : m_strokeIds) {
        Result<PlacedStroke> removed = m_page->remove(strokeId);
        if (!removed) {
            for (PlacedStroke& placed : erased) {
                std::ignore = m_page->insert(std::move(placed));
            }
            return std::unexpected{removed.error()};
        }
        erased.push_back(std::move(*removed));
    }
    for (const Uuid& strokeId : m_strokeIds) {
        m_storage->removeStroke(m_page->id(), strokeId);
    }
    m_erased = std::move(erased);
    return {};
}

Result<void> EraseStrokesCommand::revert() {
    return restoreStrokes(*m_page, *m_storage, std::exchange(m_erased, {}));
}

std::optional<Uuid> AddStrokeCommand::pageToShow() const {
    return m_page->id();
}

std::optional<Uuid> ClearPageCommand::pageToShow() const {
    return m_page->id();
}

std::optional<Uuid> EraseStrokesCommand::pageToShow() const {
    return m_page->id();
}

}
