#pragma once

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/model/Link.hpp"
#include "core/model/Page.hpp"
#include "core/undo/UndoStack.hpp"

#include <optional>

namespace phvikapen::core {

class StorageThread;

class AddLinkCommand final : public ICommand {
public:
    AddLinkCommand(Page* page, StorageThread* storage, PlacedLink placed) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    Page* m_page;
    StorageThread* m_storage;
    PlacedLink m_placed;
};

class RemoveLinkCommand final : public ICommand {
public:
    RemoveLinkCommand(Page* page, StorageThread* storage, const Uuid& linkId) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    Page* m_page;
    StorageThread* m_storage;
    Uuid m_linkId;
    std::optional<PlacedLink> m_removed;
};

// Everything about a link that can be changed while it keeps its place in the order of the page:
// where it stands, how large it is, and where it goes.
class ChangeLinkCommand final : public ICommand {
public:
    ChangeLinkCommand(Page* page, StorageThread* storage, Link link) noexcept;

    Result<void> apply() override;
    Result<void> revert() override;
    [[nodiscard]] std::optional<Uuid> pageToShow() const override;

private:
    Page* m_page;
    StorageThread* m_storage;
    Link m_link;
    std::optional<Link> m_was;
};

}
