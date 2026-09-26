#pragma once

#include <QByteArray>
#include <QObject>

#include <memory>

class QAudioSink;
class QBuffer;

namespace phvikapen::platform::audio {

// A short noise the application can make to say that something has finished. The noise is worked
// out rather than carried as a file, so nothing has to be shipped, found or decoded for it.
class Chime : public QObject {
    Q_OBJECT

public:
    explicit Chime(QObject* parent = nullptr);
    ~Chime() override;

    Chime(const Chime&) = delete;
    Chime& operator=(const Chime&) = delete;
    Chime(Chime&&) = delete;
    Chime& operator=(Chime&&) = delete;

    // Whether this machine has anything to make a noise with.
    [[nodiscard]] bool canRing() const;

    // Makes the noise, from the beginning, however far a previous one had got.
    void ring();

private:
    QByteArray m_noise;
    std::unique_ptr<QBuffer> m_held;
    std::unique_ptr<QAudioSink> m_out;
};

}
