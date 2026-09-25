#pragma once

#include "core/id/Uuid.hpp"

#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace phvikapen::core {

// One layer of a page. Everything on a page belongs to exactly one of them, and they are kept in
// the order they are drawn: the first stands at the bottom and the last on top.
//
// A layer that is not shown is drawn nowhere and printed nowhere. A locked layer is drawn, but
// nothing on it can be taken hold of, written on or rubbed out, so that a page traced over a
// drawing cannot move the drawing by accident.
struct Layer {
    static constexpr std::size_t kMostLayers = 64;

    Uuid id;
    std::string name;
    bool shown{true};
    bool locked{false};

    friend bool operator==(const Layer&, const Layer&) = default;
};

// Whether a layer may be drawn on: one that is hidden or locked may not.
[[nodiscard]] constexpr bool isOpenToTheHand(const Layer& layer) noexcept {
    return layer.shown && !layer.locked;
}

// Where a layer stands in the order, or as many as there are where it is not one of them.
[[nodiscard]] std::size_t placeOfLayer(std::span<const Layer> layers, const Uuid& id) noexcept;

// The layer a thing belongs to, which is the one named unless no layer of that name is there: then
// it is the bottom one, so that anything written down before there were layers still has a place.
[[nodiscard]] const Layer* layerOf(std::span<const Layer> layers, const Uuid& id) noexcept;

// Whether a thing on a layer is drawn at all.
[[nodiscard]] bool isShownOn(std::span<const Layer> layers, const Uuid& id) noexcept;

[[nodiscard]] bool isLockedOn(std::span<const Layer> layers, const Uuid& id) noexcept;

// A name no other layer of the page carries, made from the one asked for.
[[nodiscard]] std::string freeName(std::span<const Layer> layers, const std::string& wanted);

// The same layers with one moved to another place in the order. Somewhere that is not a place
// leaves them as they were.
[[nodiscard]] std::vector<Layer> withLayerMoved(std::vector<Layer> layers, std::size_t from,
                                                std::size_t to);

}
