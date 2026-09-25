#pragma once

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/model/Layer.hpp"
#include "core/model/Page.hpp"
#include "core/undo/UndoStack.hpp"

#include <optional>
#include <vector>

namespace phvikapen::core {

class StorageThread;

// Every change to the layers of a page is the same change: the list of them as it was becomes the
// list of them as it is to be. Adding one, taking one away, renaming one, hiding one, locking one
// and reordering them are all that, so there is one thing to get right and one thing to take back.
class ChangeLayersCommand final : public ICommand {
public:
    ChangeLayersCommand(Page* page, StorageThread* storage, std::vector<Layer> wanted);

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    [[nodiscard]] Result<void> write(std::vector<Layer> layers);

    Page* m_page;
    StorageThread* m_storage;
    std::vector<Layer> m_after;
    std::vector<Layer> m_before;
};

// One thing on a page carried from one layer to another.
class MoveToLayerCommand final : public ICommand {
public:
    MoveToLayerCommand(Page* page, StorageThread* storage, const Uuid& thingId,
                       const Uuid& layerId) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    [[nodiscard]] Result<void> carry(const Uuid& layerId);

    Page* m_page;
    StorageThread* m_storage;
    Uuid m_thingId;
    Uuid m_layerId;
    std::optional<Uuid> m_stood;
};

}
