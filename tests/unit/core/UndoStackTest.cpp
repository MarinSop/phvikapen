#include "core/undo/UndoStack.hpp"

#include "core/Error.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace phvikapen::core {
namespace {

class RecordingCommand final : public ICommand {
public:
    RecordingCommand(std::vector<std::string>* log, std::string label) noexcept
        : m_log{log}, m_label{std::move(label)} {}

    void failApply() noexcept { m_applyFails = true; }

    void failRevert() noexcept { m_revertFails = true; }

    Result<void> apply() override {
        if (m_applyFails) {
            return makeError(ErrorCode::IoFailure, m_label + " could not be applied");
        }
        m_log->push_back(m_label + "+");
        return {};
    }

    Result<void> revert() override {
        if (m_revertFails) {
            return makeError(ErrorCode::IoFailure, m_label + " could not be reverted");
        }
        m_log->push_back(m_label + "-");
        return {};
    }

private:
    std::vector<std::string>* m_log;
    std::string m_label;
    bool m_applyFails{false};
    bool m_revertFails{false};
};

[[nodiscard]] std::unique_ptr<RecordingCommand> recording(std::vector<std::string>* log,
                                                          std::string label) {
    return std::make_unique<RecordingCommand>(log, std::move(label));
}

TEST(UndoStackTest, AnEmptyHistoryHasNothingToUndoOrRedo) {
    const UndoStack stack;

    EXPECT_FALSE(stack.canUndo());
    EXPECT_FALSE(stack.canRedo());
    EXPECT_EQ(stack.undoCount(), 0U);
    EXPECT_EQ(stack.redoCount(), 0U);
}

TEST(UndoStackTest, ShowsTheCommandsNextInLine) {
    std::vector<std::string> log;
    UndoStack stack;
    std::unique_ptr<RecordingCommand> first = recording(&log, "a");
    std::unique_ptr<RecordingCommand> second = recording(&log, "b");
    const ICommand* const firstAddress = first.get();
    const ICommand* const secondAddress = second.get();
    EXPECT_EQ(stack.nextUndo(), nullptr);

    ASSERT_TRUE(stack.run(std::move(first)));
    ASSERT_TRUE(stack.run(std::move(second)));
    EXPECT_EQ(stack.nextUndo(), secondAddress);
    EXPECT_EQ(stack.nextRedo(), nullptr);

    ASSERT_TRUE(stack.undo());
    EXPECT_EQ(stack.nextUndo(), firstAddress);
    EXPECT_EQ(stack.nextRedo(), secondAddress);
}

TEST(UndoStackTest, RunningACommandAppliesItOnce) {
    std::vector<std::string> log;
    UndoStack stack;

    EXPECT_TRUE(stack.run(recording(&log, "a")));

    EXPECT_EQ(log, std::vector<std::string>{"a+"});
    EXPECT_TRUE(stack.canUndo());
    EXPECT_FALSE(stack.canRedo());
}

TEST(UndoStackTest, ACommandThatFailsToApplyIsNotRemembered) {
    std::vector<std::string> log;
    UndoStack stack;
    std::unique_ptr<RecordingCommand> command = recording(&log, "a");
    command->failApply();

    const Result<void> result = stack.run(std::move(command));

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::IoFailure);
    EXPECT_TRUE(log.empty());
    EXPECT_FALSE(stack.canUndo());
}

TEST(UndoStackTest, AMissingCommandIsRejected) {
    UndoStack stack;

    const Result<void> result = stack.run(nullptr);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::InvalidArgument);
}

TEST(UndoStackTest, UndoTakesTheMostRecentCommandBackFirst) {
    std::vector<std::string> log;
    UndoStack stack;
    ASSERT_TRUE(stack.run(recording(&log, "a")));
    ASSERT_TRUE(stack.run(recording(&log, "b")));

    EXPECT_TRUE(stack.undo());
    EXPECT_TRUE(stack.undo());

    EXPECT_EQ(log, (std::vector<std::string>{"a+", "b+", "b-", "a-"}));
    EXPECT_FALSE(stack.canUndo());
    EXPECT_EQ(stack.redoCount(), 2U);
}

TEST(UndoStackTest, RedoAppliesTheCommandsAgainInTheOrderTheyWereRun) {
    std::vector<std::string> log;
    UndoStack stack;
    ASSERT_TRUE(stack.run(recording(&log, "a")));
    ASSERT_TRUE(stack.run(recording(&log, "b")));
    ASSERT_TRUE(stack.undo());
    ASSERT_TRUE(stack.undo());
    log.clear();

    EXPECT_TRUE(stack.redo());
    EXPECT_TRUE(stack.redo());

    EXPECT_EQ(log, (std::vector<std::string>{"a+", "b+"}));
    EXPECT_FALSE(stack.canRedo());
}

TEST(UndoStackTest, RunningACommandDropsWhatWasUndone) {
    std::vector<std::string> log;
    UndoStack stack;
    ASSERT_TRUE(stack.run(recording(&log, "a")));
    ASSERT_TRUE(stack.undo());

    ASSERT_TRUE(stack.run(recording(&log, "b")));

    EXPECT_FALSE(stack.canRedo());
    EXPECT_EQ(stack.undoCount(), 1U);
    EXPECT_EQ(log, (std::vector<std::string>{"a+", "a-", "b+"}));
}

TEST(UndoStackTest, UndoWithoutAHistoryFails) {
    UndoStack stack;

    const Result<void> result = stack.undo();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::NotFound);
}

TEST(UndoStackTest, RedoWithoutAnUndoFails) {
    std::vector<std::string> log;
    UndoStack stack;
    ASSERT_TRUE(stack.run(recording(&log, "a")));

    const Result<void> result = stack.redo();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::NotFound);
}

TEST(UndoStackTest, AFailedUndoLeavesTheHistoryUntouched) {
    std::vector<std::string> log;
    UndoStack stack;
    std::unique_ptr<RecordingCommand> command = recording(&log, "a");
    command->failRevert();
    ASSERT_TRUE(stack.run(std::move(command)));

    const Result<void> result = stack.undo();

    ASSERT_FALSE(result);
    EXPECT_TRUE(stack.canUndo());
    EXPECT_FALSE(stack.canRedo());
}

TEST(UndoStackTest, AFailedRedoLeavesTheHistoryUntouched) {
    std::vector<std::string> log;
    UndoStack stack;
    std::unique_ptr<RecordingCommand> command = recording(&log, "a");
    RecordingCommand* observed = command.get();
    ASSERT_TRUE(stack.run(std::move(command)));
    ASSERT_TRUE(stack.undo());
    observed->failApply();

    const Result<void> result = stack.redo();

    ASSERT_FALSE(result);
    EXPECT_TRUE(stack.canRedo());
    EXPECT_FALSE(stack.canUndo());
}

TEST(UndoStackTest, TheOldestCommandIsForgottenOnceTheLimitIsReached) {
    std::vector<std::string> log;
    UndoStack stack{2};
    ASSERT_TRUE(stack.run(recording(&log, "a")));
    ASSERT_TRUE(stack.run(recording(&log, "b")));
    ASSERT_TRUE(stack.run(recording(&log, "c")));
    log.clear();

    EXPECT_EQ(stack.undoCount(), 2U);
    EXPECT_TRUE(stack.undo());
    EXPECT_TRUE(stack.undo());
    EXPECT_FALSE(stack.undo());
    EXPECT_EQ(log, (std::vector<std::string>{"c-", "b-"}));
}

TEST(UndoStackTest, AHistoryOfNoCommandsStillRemembersOne) {
    std::vector<std::string> log;
    UndoStack stack{0};

    ASSERT_TRUE(stack.run(recording(&log, "a")));

    EXPECT_EQ(stack.undoCount(), 1U);
}

TEST(UndoStackTest, ClearingForgetsEverything) {
    std::vector<std::string> log;
    UndoStack stack;
    ASSERT_TRUE(stack.run(recording(&log, "a")));
    ASSERT_TRUE(stack.run(recording(&log, "b")));
    ASSERT_TRUE(stack.undo());

    stack.clear();

    EXPECT_FALSE(stack.canUndo());
    EXPECT_FALSE(stack.canRedo());
}

}
}
