#pragma once

#include "platform/ocr/IReadPicture.hpp"

#include <QObject>

#include <atomic>
#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <thread>
#include <vector>

namespace phvikapen::platform::ocr {

// The words in a picture, read by the reader Windows carries. It is given the bytes of the file
// rather than a decoded picture, so that whatever the machine can open, it can read.
class WindowsReadPicture final : public IReadPicture {
public:
    WindowsReadPicture() = default;
    ~WindowsReadPicture() override;

    WindowsReadPicture(const WindowsReadPicture&) = delete;
    WindowsReadPicture& operator=(const WindowsReadPicture&) = delete;
    WindowsReadPicture(WindowsReadPicture&&) = delete;
    WindowsReadPicture& operator=(WindowsReadPicture&&) = delete;

    void read(std::span<const std::byte> picture, const std::string& language, Read read) override;

    void giveUp() override;

    [[nodiscard]] std::vector<std::string> languages() override;

private:
    // Reading happens away from the thread that asked, and the answer is handed back through this
    // one, so that it arrives on the thread that asked and dies with the reader.
    QObject m_home;
    std::jthread m_reading;
    std::shared_ptr<std::atomic_bool> m_wanted;
};

}
