#include "core/undo/UndoStack.hpp"

#include "core/Error.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <utility>

namespace phvikapen::core {

UndoStack::UndoStack(std::size_t depthLimit) noexcept
    : m_depthLimit{std::max<std::size_t>(1, depthLimit)} {}

Result<void> UndoStack::run(std::unique_ptr<ICommand> command) {
    if (command == nullptr) {
        return makeError(ErrorCode::InvalidArgument, "an undo stack cannot run a missing command");
    }
    if (const Result<void> applied = command->apply(); !applied) {
        return applied;
    }

    m_commands.erase(std::next(m_commands.begin(), static_cast<std::ptrdiff_t>(m_applied)),
                     m_commands.end());
    m_commands.push_back(std::move(command));
    if (m_commands.size() > m_depthLimit) {
        m_commands.erase(m_commands.begin());
    }
    m_applied = m_commands.size();
    return {};
}

Result<void> UndoStack::undo() {
    if (!canUndo()) {
        return makeError(ErrorCode::NotFound, "there is nothing to undo");
    }
    if (const Result<void> reverted = m_commands[m_applied - 1]->revert(); !reverted) {
        return reverted;
    }
    --m_applied;
    return {};
}

Result<void> UndoStack::redo() {
    if (!canRedo()) {
        return makeError(ErrorCode::NotFound, "there is nothing to redo");
    }
    if (const Result<void> applied = m_commands[m_applied]->apply(); !applied) {
        return applied;
    }
    ++m_applied;
    return {};
}

void UndoStack::clear() noexcept {
    m_commands.clear();
    m_applied = 0;
}

}
