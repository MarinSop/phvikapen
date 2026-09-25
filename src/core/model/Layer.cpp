#include "core/model/Layer.hpp"

#include "core/id/Uuid.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <span>
#include <string>
#include <vector>

namespace phvikapen::core {
namespace {

[[nodiscard]] bool isTaken(std::span<const Layer> layers, const std::string& name) {
    return std::ranges::any_of(layers, [&name](const Layer& layer) { return layer.name == name; });
}

}

std::size_t placeOfLayer(std::span<const Layer> layers, const Uuid& id) noexcept {
    for (std::size_t step = 0; step < layers.size(); ++step) {
        if (layers[step].id == id) {
            return step;
        }
    }
    return layers.size();
}

const Layer* layerOf(std::span<const Layer> layers, const Uuid& id) noexcept {
    if (layers.empty()) {
        return nullptr;
    }
    const std::size_t place = placeOfLayer(layers, id);
    return place < layers.size() ? &layers[place] : &layers.front();
}

bool isShownOn(std::span<const Layer> layers, const Uuid& id) noexcept {
    const Layer* const layer = layerOf(layers, id);
    return layer == nullptr || layer->shown;
}

bool isLockedOn(std::span<const Layer> layers, const Uuid& id) noexcept {
    const Layer* const layer = layerOf(layers, id);
    return layer != nullptr && layer->locked;
}

bool isOpenToTheHand(std::span<const Layer> layers, const Uuid& id) noexcept {
    const Layer* const layer = layerOf(layers, id);
    return layer == nullptr || isOpenToTheHand(*layer);
}

std::string freeName(std::span<const Layer> layers, const std::string& wanted) {
    if (!isTaken(layers, wanted)) {
        return wanted;
    }
    for (std::size_t again = 2; again < Layer::kMostLayers + 2; ++again) {
        std::string tried = wanted + " " + std::to_string(again);
        if (!isTaken(layers, tried)) {
            return tried;
        }
    }
    return wanted;
}

std::vector<Layer> withLayerMoved(std::vector<Layer> layers, std::size_t from, std::size_t to) {
    if (from >= layers.size() || to >= layers.size() || from == to) {
        return layers;
    }
    const Layer moved = layers[from];
    layers.erase(std::next(layers.begin(), static_cast<std::ptrdiff_t>(from)));
    layers.insert(std::next(layers.begin(), static_cast<std::ptrdiff_t>(to)), moved);
    return layers;
}

}
