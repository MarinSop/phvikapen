#pragma once

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/ink/Stroke.hpp"
#include "core/storage/NotebookStore.hpp"
#include "core/text/InkWord.hpp"
#include "platform/text/IHandwriting.hpp"

#include <QObject>
#include <QString>

#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <functional>
#include <mutex>
#include <stop_token>
#include <thread>
#include <vector>

namespace phvikapen::app {

// Reads the handwriting of the pages written on since they were last read, one at a time, on a
// thread of its own with its own hold on the notebook file.
class HandwritingReader : public QObject {
    Q_OBJECT

public:
    explicit HandwritingReader(QObject* parent = nullptr);
    ~HandwritingReader() override;

    HandwritingReader(const HandwritingReader&) = delete;
    HandwritingReader& operator=(const HandwritingReader&) = delete;
    HandwritingReader(HandwritingReader&&) = delete;
    HandwritingReader& operator=(HandwritingReader&&) = delete;

    void open(const std::filesystem::path& path);
    void close();

    // Something was written or rubbed out: look again for pages that want reading.
    void nudge();

    using WordsHandler = std::function<void(core::Result<std::vector<core::InkWord>>)>;

    // Read these strokes before anything else, and hand the words back on the thread that asked.
    void readSoon(std::vector<core::Stroke> strokes, WordsHandler done);

    [[nodiscard]] static bool availableHere();

signals:
    void pagesWaitingChanged(int waiting);
    void pageRead();
    void failed(const QString& message);

private:
    void run(const std::stop_token& stopToken);
    [[nodiscard]] bool waitForWork(const std::stop_token& stopToken);
    void readPages(const std::stop_token& stopToken, core::NotebookStore& store,
                   platform::text::IHandwriting& reader);
    void readWhatWasAsked(platform::text::IHandwriting& reader);
    [[nodiscard]] static core::Result<void> readPage(core::NotebookStore& store,
                                                     platform::text::IHandwriting& reader,
                                                     const core::Uuid& pageId);
    void report(const core::Error& error);

    struct Asked {
        std::vector<core::Stroke> strokes;
        WordsHandler done;
    };

    std::filesystem::path m_path;
    std::vector<Asked> m_soon;
    std::atomic_bool m_stumbled{false};
    std::mutex m_mutex;
    std::condition_variable_any m_wanted;
    bool m_asked{false};
    std::jthread m_thread;
};

}
