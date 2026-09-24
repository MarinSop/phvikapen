#pragma once

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/storage/NotebookStore.hpp"
#include "platform/text/IHandwriting.hpp"

#include <QObject>
#include <QString>

#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <stop_token>
#include <thread>

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
    [[nodiscard]] static core::Result<void> readPage(core::NotebookStore& store,
                                                    platform::text::IHandwriting& reader,
                                                    const core::Uuid& pageId);
    void report(const core::Error& error);

    std::filesystem::path m_path;
    std::atomic_bool m_stumbled{false};
    std::mutex m_mutex;
    std::condition_variable_any m_wanted;
    bool m_asked{false};
    std::jthread m_thread;
};

}
