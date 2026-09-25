#include "core/undo/LayerCommands.hpp"

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/model/Layer.hpp"
#include "core/model/Page.hpp"
#include "core/storage/NotebookStore.hpp"
#include "core/storage/StorageThread.hpp"

#include <optional>
#include <utility>
#include <vector>

namespace phvikapen::core {

ChangeLayersCommand::ChangeLayersCommand(Page* page, StorageThread* storage,
                                         std::vector<Layer> wanted)
    : m_page{page}, m_storage{storage}, m_after{std::move(wanted)} {}

Result<void> ChangeLayersCommand::apply() {
    const std::span<const Layer> standing = m_page->layers();
    m_before.assign(standing.begin(), standing.end());
    return write(m_after);
}

Result<void> ChangeLayersCommand::revert() {
    if (m_before.empty()) {
        return makeError(ErrorCode::InvalidArgument, "there are no layers to put back");
    }
    return write(m_before);
}

Result<void> ChangeLayersCommand::write(std::vector<Layer> layers) {
    m_page->setLayers(layers);
    m_storage->submit([pageId = m_page->id(), layers = std::move(layers)](NotebookStore& store) {
        return store.writeLayers(pageId, layers);
    });
    return {};
}

std::optional<Uuid> ChangeLayersCommand::pageToShow() const {
    return m_page->id();
}

MoveToLayerCommand::MoveToLayerCommand(Page* page, StorageThread* storage, const Uuid& thingId,
                                       const Uuid& layerId) noexcept
    : m_page{page}, m_storage{storage}, m_thingId{thingId}, m_layerId{layerId} {}

Result<void> MoveToLayerCommand::apply() {
    Result<Uuid> stood = m_page->moveToLayer(m_thingId, m_layerId);
    if (!stood) {
        return std::unexpected{stood.error()};
    }
    m_stood = *stood;
    m_storage->submit(
        [pageId = m_page->id(), thingId = m_thingId, layerId = m_layerId](NotebookStore& store) {
            return store.moveToLayer(pageId, thingId, layerId);
        });
    return {};
}

Result<void> MoveToLayerCommand::revert() {
    if (!m_stood) {
        return makeError(ErrorCode::InvalidArgument, "there is no layer to put it back on");
    }
    return carry(*m_stood);
}

Result<void> MoveToLayerCommand::carry(const Uuid& layerId) {
    if (const Result<Uuid> moved = m_page->moveToLayer(m_thingId, layerId); !moved) {
        return std::unexpected{moved.error()};
    }
    m_storage->submit([pageId = m_page->id(), thingId = m_thingId, layerId](NotebookStore& store) {
        return store.moveToLayer(pageId, thingId, layerId);
    });
    return {};
}

std::optional<Uuid> MoveToLayerCommand::pageToShow() const {
    return m_page->id();
}

}
