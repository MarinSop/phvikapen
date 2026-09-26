#include "platform/audio/Chime.hpp"

#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSink>
#include <QBuffer>
#include <QMediaDevices>

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace phvikapen::platform::audio {
namespace {

constexpr int kRate = 44100;
constexpr int kQuietest = -32767;
constexpr int kLoudest = 32767;
constexpr qsizetype kPerSample = 2;
constexpr unsigned kLowByte = 0xFFU;
constexpr unsigned kByteBits = 8U;

// Two notes, a fifth apart, each held for a moment: enough to be heard across a room and short
// enough not to be in the way.
constexpr std::array<double, 2> kNotes{880.0, 1318.5};
constexpr double kNoteFor = 0.18;
constexpr double kHowLoud = 0.35;

// How much of each note is spent coming up and going down, as a share of it. Without this the note
// begins and ends with a click.
constexpr double kEdge = 0.08;

[[nodiscard]] double howLoudAt(double through) {
    if (through < kEdge) {
        return through / kEdge;
    }
    if (through > 1.0 - kEdge) {
        return (1.0 - through) / kEdge;
    }
    return 1.0;
}

[[nodiscard]] QByteArray madeUp() {
    const auto perNote = static_cast<qsizetype>(kRate * kNoteFor);
    QByteArray noise;
    noise.reserve(perNote * static_cast<qsizetype>(kNotes.size()) * kPerSample);
    for (const double note : kNotes) {
        for (qsizetype step = 0; step < perNote; ++step) {
            const double through = static_cast<double>(step) / static_cast<double>(perNote);
            const double turn = 2.0 * std::numbers::pi * note * static_cast<double>(step)
                                / static_cast<double>(kRate);
            const double loud = std::sin(turn) * howLoudAt(through) * kHowLoud;
            const auto sample =
                static_cast<quint16>(static_cast<qint16>(std::clamp(loud * kLoudest,
                                                                    static_cast<double>(kQuietest),
                                                                    static_cast<double>(kLoudest))));
            const unsigned raw = sample;
            noise.append(static_cast<char>(raw & kLowByte));
            noise.append(static_cast<char>((raw >> kByteBits) & kLowByte));
        }
    }
    return noise;
}

[[nodiscard]] QAudioFormat wayItIsMade() {
    QAudioFormat form;
    form.setSampleRate(kRate);
    form.setChannelCount(1);
    form.setSampleFormat(QAudioFormat::Int16);
    return form;
}

}

Chime::Chime(QObject* parent) : QObject(parent), m_noise{madeUp()} {
    const QAudioDevice out = QMediaDevices::defaultAudioOutput();
    if (out.isNull()) {
        return;
    }
    const QAudioFormat form = wayItIsMade();
    if (!out.isFormatSupported(form)) {
        return;
    }
    m_out = std::make_unique<QAudioSink>(out, form);
    m_held = std::make_unique<QBuffer>(&m_noise);
}

Chime::~Chime() {
    if (m_out) {
        m_out->stop();
    }
}

bool Chime::canRing() const {
    return m_out != nullptr;
}

void Chime::ring() {
    if (!canRing()) {
        return;
    }
    m_out->stop();
    m_held->close();
    if (!m_held->open(QIODevice::ReadOnly)) {
        return;
    }
    m_held->seek(0);
    m_out->start(m_held.get());
}

}
