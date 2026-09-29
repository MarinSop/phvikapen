#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "platform/speech/win/WindowsSpeech.hpp"

#include "core/Error.hpp"
#include "core/model/Recording.hpp"
#include "platform/speech/ISpeech.hpp"

#include <sapi.h>
#include <winrt/base.h>

#include <QAudioBuffer>
#include <QAudioDecoder>
#include <QBuffer>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QMetaObject>
#include <QString>
#include <QTimer>
#include <QUuid>
#include <Qt>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cwchar>
#include <exception>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace phvikapen::platform::speech {
namespace {

// The reader takes plain samples and nothing else, so the recording is turned into the one shape
// every recognizer Windows carries is sure to read.
constexpr int kSampleRate = 16000;
constexpr int kBitsPerSample = 16;
constexpr int kBytesPerSample = kBitsPerSample / 8;
constexpr quint32 kFormatBytes = 16;
constexpr quint16 kPlainSamples = 1;
constexpr int kRiffBesides = 36;

// How long to wait on the reader before looking again, and how much waiting in a row means the
// reader has nothing more to say. Short enough that giving up is answered at once rather than after
// the reader has finished thinking.
constexpr DWORD kWaitFor = 200;
constexpr int kQuietRounds = 50;

// How long the decoder may say nothing at all before the reading is given up. It is started again
// with every piece of sound that arrives, so a long recording is never cut short.
constexpr int kNoProgressFor = 4000;

constexpr std::int64_t kPerMilli = 10000;
constexpr std::int64_t kThousand = 1000;
constexpr int kHexRadix = 16;
constexpr std::size_t kMostDigits = 16;

[[nodiscard]] QByteArray wavHeader(const QAudioFormat& format, int bytes) {
    const int channels = std::max(1, format.channelCount());
    const int rate = format.sampleRate() > 0 ? format.sampleRate() : kSampleRate;
    QByteArray head;
    QDataStream out{&head, QIODevice::WriteOnly};
    out.setByteOrder(QDataStream::LittleEndian);
    out.writeRawData("RIFF", 4);
    out << static_cast<quint32>(bytes + kRiffBesides);
    out.writeRawData("WAVEfmt ", 8);
    out << kFormatBytes;
    out << kPlainSamples;
    out << static_cast<quint16>(channels);
    out << static_cast<quint32>(rate);
    out << static_cast<quint32>(rate * channels * kBytesPerSample);
    out << static_cast<quint16>(channels * kBytesPerSample);
    out << static_cast<quint16>(kBitsPerSample);
    out.writeRawData("data", 4);
    out << static_cast<quint32>(bytes);
    return head;
}

[[nodiscard]] WAVEFORMATEX waveOf(const QAudioFormat& format) {
    const auto channels = static_cast<WORD>(std::max(1, format.channelCount()));
    const auto rate =
        static_cast<DWORD>(format.sampleRate() > 0 ? format.sampleRate() : kSampleRate);
    WAVEFORMATEX wave{};
    wave.wFormatTag = static_cast<WORD>(WAVE_FORMAT_PCM);
    wave.nChannels = channels;
    wave.nSamplesPerSec = rate;
    wave.wBitsPerSample = static_cast<WORD>(kBitsPerSample);
    wave.nBlockAlign = static_cast<WORD>(channels * kBytesPerSample);
    wave.nAvgBytesPerSec = rate * wave.nBlockAlign;
    wave.cbSize = 0;
    return wave;
}

[[nodiscard]] std::string tagOf(const std::wstring& hex) {
    const unsigned long named = std::wcstoul(hex.c_str(), nullptr, kHexRadix);
    if (named == 0) {
        return {};
    }
    std::array<wchar_t, LOCALE_NAME_MAX_LENGTH> spelled{};
    if (LCIDToLocaleName(static_cast<LCID>(named), spelled.data(), static_cast<int>(spelled.size()),
                         0)
        == 0) {
        return {};
    }
    return QString::fromWCharArray(spelled.data()).toStdString();
}

// One recognizer names every language it reads, as hexadecimal numbers with a semicolon between.
[[nodiscard]] std::vector<std::string> tagsIn(const std::wstring& listed) {
    std::vector<std::string> tags;
    std::size_t from = 0;
    while (from <= listed.size()) {
        const std::size_t to = listed.find(L';', from);
        const std::size_t howMany = to == std::wstring::npos ? std::wstring::npos : to - from;
        if (std::string tag = tagOf(listed.substr(from, howMany)); !tag.empty()) {
            tags.push_back(std::move(tag));
        }
        if (to == std::wstring::npos) {
            break;
        }
        from = to + 1;
    }
    return tags;
}

[[nodiscard]] std::wstring hexOf(const std::string& language) {
    if (language.empty()) {
        return {};
    }
    const std::wstring asked = QString::fromStdString(language).toStdWString();
    const LCID named = LocaleNameToLCID(asked.c_str(), 0);
    if (named == 0) {
        return {};
    }
    std::array<wchar_t, kMostDigits> spelled{};
    const int written =
        std::swprintf(spelled.data(), spelled.size(), L"%X", static_cast<unsigned>(named));
    return written > 0 ? std::wstring{spelled.data(), static_cast<std::size_t>(written)}
                       : std::wstring{};
}

[[nodiscard]] winrt::com_ptr<IEnumSpObjectTokens> readersOf(const std::wstring& wanted) {
    winrt::com_ptr<ISpObjectTokenCategory> listed;
    winrt::check_hresult(CoCreateInstance(CLSID_SpObjectTokenCategory, nullptr, CLSCTX_ALL,
                                          IID_PPV_ARGS(listed.put())));
    winrt::check_hresult(listed->SetId(SPCAT_RECOGNIZERS, FALSE));
    winrt::com_ptr<IEnumSpObjectTokens> found;
    const std::wstring asked = wanted.empty() ? std::wstring{} : L"Language=" + wanted;
    if (FAILED(listed->EnumTokens(asked.empty() ? nullptr : asked.c_str(), nullptr, found.put()))) {
        return nullptr;
    }
    return found;
}

[[nodiscard]] winrt::com_ptr<ISpObjectToken> readerFor(const std::string& language) {
    const winrt::com_ptr<IEnumSpObjectTokens> found = readersOf(hexOf(language));
    if (!found) {
        return nullptr;
    }
    winrt::com_ptr<ISpObjectToken> one;
    ULONG got = 0;
    if (found->Next(1, one.put(), &got) != S_OK || got != 1) {
        return nullptr;
    }
    return one;
}

// Whether the machine carries any reader for speech at all. A machine with no speech language pack
// carries none, and listening to nothing would only wait.
[[nodiscard]] bool anyReaderAtAll() {
    const winrt::com_ptr<IEnumSpObjectTokens> found = readersOf({});
    if (!found) {
        return false;
    }
    ULONG how = 0;
    return SUCCEEDED(found->GetCount(&how)) && how > 0;
}

// What the reader carries between it and the recording. All three are held together, because the
// reading stops the moment any of them is let go of.
struct Listening {
    winrt::com_ptr<ISpRecognizer> reader;
    winrt::com_ptr<ISpRecoContext> talk;
    winrt::com_ptr<ISpRecoGrammar> grammar;
};

[[nodiscard]] Listening openListening(const std::wstring& file, const WAVEFORMATEX& wave,
                                      const std::string& language) {
    Listening open;
    winrt::check_hresult(CoCreateInstance(CLSID_SpInprocRecognizer, nullptr, CLSCTX_ALL,
                                          IID_PPV_ARGS(open.reader.put())));
    if (const winrt::com_ptr<ISpObjectToken> named = readerFor(language)) {
        winrt::check_hresult(open.reader->SetRecognizer(named.get()));
    }
    winrt::com_ptr<ISpStream> sound;
    winrt::check_hresult(
        CoCreateInstance(CLSID_SpStream, nullptr, CLSCTX_ALL, IID_PPV_ARGS(sound.put())));
    winrt::check_hresult(
        sound->BindToFile(file.c_str(), SPFM_OPEN_READONLY, &SPDFID_WaveFormatEx, &wave, 0));
    winrt::check_hresult(open.reader->SetInput(sound.get(), TRUE));
    winrt::check_hresult(open.reader->CreateRecoContext(open.talk.put()));
    winrt::check_hresult(open.talk->SetNotifyWin32Event());
    const ULONGLONG wantedEvents = SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM);
    winrt::check_hresult(open.talk->SetInterest(wantedEvents, wantedEvents));
    winrt::check_hresult(open.talk->CreateGrammar(1, open.grammar.put()));
    winrt::check_hresult(open.grammar->LoadDictation(nullptr, SPLO_STATIC));
    winrt::check_hresult(open.grammar->SetDictationState(SPRS_ACTIVE));
    winrt::check_hresult(open.reader->SetRecoState(SPRST_ACTIVE));
    return open;
}

[[nodiscard]] std::int64_t atMoment(std::uint64_t bytes, std::int64_t perSecond) {
    if (perSecond <= 0) {
        return 0;
    }
    return static_cast<std::int64_t>(bytes) * kThousand / perSecond;
}

void gather(const SPEVENT& happening, std::int64_t perSecond, std::vector<core::Saying>& words) {
    if (happening.eEventId != SPEI_RECOGNITION || happening.elParamType != SPET_LPARAM_IS_OBJECT) {
        return;
    }
    auto* const said = reinterpret_cast<ISpRecoResult*>(happening.lParam);
    SPPHRASE* phrase = nullptr;
    if (said == nullptr || FAILED(said->GetPhrase(&phrase)) || phrase == nullptr) {
        return;
    }
    const std::int64_t began = atMoment(phrase->ullAudioStreamPosition, perSecond);
    const std::span<const SPPHRASEELEMENT> elements{phrase->pElements,
                                                    phrase->Rule.ulCountOfElements};
    for (const SPPHRASEELEMENT& element : elements) {
        if (element.pszDisplayText == nullptr) {
            continue;
        }
        std::string spoken = QString::fromWCharArray(element.pszDisplayText).toStdString();
        if (spoken.empty()) {
            continue;
        }
        const std::int64_t from =
            began + (static_cast<std::int64_t>(element.ulAudioTimeOffset) / kPerMilli);
        words.push_back(core::Saying{
            .from = from,
            .to = from + (static_cast<std::int64_t>(element.ulAudioSizeTime) / kPerMilli),
            .text = std::move(spoken),
        });
    }
    CoTaskMemFree(phrase);
}

void letGoOf(const SPEVENT& happening) {
    switch (happening.elParamType) {
    case SPET_LPARAM_IS_POINTER:
    case SPET_LPARAM_IS_STRING:
        CoTaskMemFree(reinterpret_cast<void*>(happening.lParam));
        break;
    case SPET_LPARAM_IS_OBJECT:
    case SPET_LPARAM_IS_TOKEN:
        if (happening.lParam != 0) {
            reinterpret_cast<IUnknown*>(happening.lParam)->Release();
        }
        break;
    default:
        break;
    }
}

[[nodiscard]] std::vector<core::Saying> listenThrough(const Listening& open, std::int64_t perSecond,
                                                      const std::atomic_bool& wanted) {
    std::vector<core::Saying> words;
    int quiet = 0;
    bool ended = false;
    while (!ended && wanted.load() && quiet < kQuietRounds) {
        if (open.talk->WaitForNotifyEvent(kWaitFor) != S_OK) {
            ++quiet;
            continue;
        }
        quiet = 0;
        SPEVENT happening{};
        ULONG got = 0;
        while (open.talk->GetEvents(1, &happening, &got) == S_OK && got == 1) {
            if (happening.eEventId == SPEI_END_SR_STREAM) {
                ended = true;
            }
            gather(happening, perSecond, words);
            letGoOf(happening);
            happening = SPEVENT{};
            got = 0;
        }
    }
    return words;
}

[[nodiscard]] core::Result<std::vector<core::Saying>> heardThere(const std::wstring& file,
                                                                 const QAudioFormat& format,
                                                                 const std::string& language,
                                                                 const std::atomic_bool& wanted) {
    if (!anyReaderAtAll()) {
        return core::makeError(core::ErrorCode::Unsupported,
                               "this machine carries no reader for speech; one comes with a "
                               "speech language pack");
    }
    const WAVEFORMATEX wave = waveOf(format);
    const Listening open = openListening(file, wave, language);
    const std::vector<core::Saying> words =
        listenThrough(open, static_cast<std::int64_t>(wave.nAvgBytesPerSec), wanted);
    std::vector<core::Saying> lines = core::linesOf(words);
    return lines;
}

// The whole reading happens here, away from the thread that asked, so nothing may be thrown out of
// it: a thread that throws ends the application rather than the reading.
[[nodiscard]] core::Result<std::vector<core::Saying>>
whatWasSaid(const std::wstring& file, const QAudioFormat& format, const std::string& language,
            const std::atomic_bool& wanted) noexcept {
    bool ours = false;
    try {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        ours = true;
    } catch (const winrt::hresult_error&) {
        // The thread was already put in an apartment by somebody else, which is theirs to end.
    }
    core::Result<std::vector<core::Saying>> got =
        core::makeError(core::ErrorCode::Unknown, "this recording could not be read");
    try {
        got = heardThere(file, format, language, wanted);
    } catch (const winrt::hresult_error& failure) {
        got = core::makeError(core::ErrorCode::Unsupported,
                              "this machine could not read the recording: "
                                  + winrt::to_string(failure.message()));
    } catch (const std::exception& failure) {
        got = core::makeError(core::ErrorCode::Unknown, failure.what());
    }
    if (ours) {
        winrt::uninit_apartment();
    }
    return got;
}

}

WindowsSpeech::WindowsSpeech() {
    try {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        m_apartment = true;
    } catch (const winrt::hresult_error&) {
        // The thread was already put in an apartment by somebody else, which is theirs to end.
    }
}

WindowsSpeech::~WindowsSpeech() {
    giveUp();
    if (m_apartment) {
        winrt::uninit_apartment();
    }
}

void WindowsSpeech::read(std::span<const std::byte> sound, const std::string& language,
                         Heard heard) {
    giveUp();
    m_answer = std::make_shared<Heard>(std::move(heard));
    if (sound.empty()) {
        (*m_answer)(
            core::makeError(core::ErrorCode::InvalidArgument, "there is nothing here to read"));
        m_answer.reset();
        return;
    }
    m_language = language;
    m_sound.resize(static_cast<qsizetype>(sound.size()));
    qsizetype at = 0;
    for (const std::byte one : sound) {
        m_sound[at++] = static_cast<char>(std::to_integer<unsigned char>(one));
    }
    m_held = std::make_unique<QBuffer>(&m_sound);
    if (!m_held->open(QIODevice::ReadOnly)) {
        (*m_answer)(core::makeError(core::ErrorCode::IoFailure,
                                    "the recording could not be handed to the reader"));
        m_answer.reset();
        return;
    }
    QAudioFormat wanted;
    wanted.setSampleRate(kSampleRate);
    wanted.setChannelCount(1);
    wanted.setChannelConfig(QAudioFormat::ChannelConfigMono);
    wanted.setSampleFormat(QAudioFormat::Int16);
    m_decoder = std::make_unique<QAudioDecoder>();
    m_decoder->setAudioFormat(wanted);
    m_decoder->setSourceDevice(m_held.get());
    QObject::connect(m_decoder.get(), &QAudioDecoder::bufferReady, &m_home,
                     [this] { takeSamples(); });
    // The decoder is let go of once it is done, which cannot happen while it is still saying so.
    QObject::connect(
        m_decoder.get(), &QAudioDecoder::finished, &m_home, [this] { samplesDone(); },
        Qt::QueuedConnection);
    // The decoder names its trouble with a signal that shares its name with the reading of it, so
    // the one that carries a reason is asked for by hand.
    QObject::connect(
        m_decoder.get(), QOverload<QAudioDecoder::Error>::of(&QAudioDecoder::error), &m_home,
        [this] {
            const std::string why =
                m_decoder ? m_decoder->errorString().toStdString() : std::string{};
            const Answer answer = m_answer;
            giveUp();
            if (answer) {
                (*answer)(core::makeError(
                    core::ErrorCode::Unsupported,
                    why.empty() ? "this recording could not be turned into samples" : why));
            }
        },
        Qt::QueuedConnection);
    m_waiting = std::make_unique<QTimer>();
    m_waiting->setSingleShot(true);
    m_waiting->setInterval(kNoProgressFor);
    // The timer is let go of as part of giving up, which cannot happen while it is still firing.
    QObject::connect(
        m_waiting.get(), &QTimer::timeout, &m_home, [this] { nothingCameOfIt(); },
        Qt::QueuedConnection);
    m_decoder->start();
    m_waiting->start();
}

void WindowsSpeech::nothingCameOfIt() {
    const Answer answer = m_answer;
    giveUp();
    if (answer) {
        (*answer)(core::makeError(core::ErrorCode::Unsupported,
                                  "this recording could not be turned into samples"));
    }
}

void WindowsSpeech::takeSamples() {
    if (m_waiting) {
        m_waiting->start();
    }
    while (m_decoder && m_decoder->bufferAvailable()) {
        const QAudioBuffer got = m_decoder->read();
        if (!got.isValid()) {
            break;
        }
        if (!m_format.isValid()) {
            m_format = got.format();
        }
        m_samples.append(got.constData<char>(), got.byteCount());
    }
}

void WindowsSpeech::samplesDone() {
    stopDecoding();
    if (!m_answer) {
        return;
    }
    if (m_samples.isEmpty() || m_format.sampleFormat() != QAudioFormat::Int16) {
        const Answer answer = m_answer;
        m_answer.reset();
        (*answer)(core::makeError(core::ErrorCode::Unsupported,
                                  "this recording could not be turned into samples the reader "
                                  "takes"));
        return;
    }
    listen();
}

void WindowsSpeech::listen() {
    const QString path = QDir::tempPath() + QStringLiteral("/phvikapen-")
                         + QUuid::createUuid().toString(QUuid::Id128) + QStringLiteral(".wav");
    QFile file{path};
    if (!file.open(QIODevice::WriteOnly)) {
        const Answer answer = m_answer;
        m_answer.reset();
        (*answer)(core::makeError(core::ErrorCode::IoFailure,
                                  "the recording could not be handed to the reader"));
        return;
    }
    file.write(wavHeader(m_format, static_cast<int>(m_samples.size())));
    file.write(m_samples);
    file.close();
    m_samples.clear();

    m_wanted = std::make_shared<std::atomic_bool>(true);
    m_reading = std::jthread{
        [this, path, format = m_format, spoken = m_language, answer = m_answer, wanted = m_wanted] {
            core::Result<std::vector<core::Saying>> got =
                whatWasSaid(path.toStdWString(), format, spoken, *wanted);
            QFile::remove(path);
            QMetaObject::invokeMethod(
                &m_home,
                [answer, wanted, got = std::move(got)]() mutable {
                    if (wanted->load()) {
                        (*answer)(std::move(got));
                    }
                },
                Qt::QueuedConnection);
        }};
}

void WindowsSpeech::stopDecoding() {
    m_waiting.reset();
    if (m_decoder) {
        m_decoder->stop();
        m_decoder.reset();
    }
    m_held.reset();
}

void WindowsSpeech::giveUp() {
    if (m_wanted) {
        m_wanted->store(false);
    }
    stopDecoding();
    if (m_reading.joinable()) {
        m_reading.join();
    }
    m_answer.reset();
    m_samples.clear();
    m_format = QAudioFormat{};
}

std::vector<std::string> WindowsSpeech::languages() {
    std::vector<std::string> said;
    try {
        const winrt::com_ptr<IEnumSpObjectTokens> found = readersOf({});
        if (!found) {
            return said;
        }
        winrt::com_ptr<ISpObjectToken> one;
        ULONG got = 0;
        while (found->Next(1, one.put(), &got) == S_OK && got == 1) {
            winrt::com_ptr<ISpDataKey> about;
            if (SUCCEEDED(one->OpenKey(L"Attributes", about.put()))) {
                LPWSTR listed = nullptr;
                if (SUCCEEDED(about->GetStringValue(L"Language", &listed)) && listed != nullptr) {
                    for (std::string& tag : tagsIn(std::wstring{listed})) {
                        said.push_back(std::move(tag));
                    }
                    CoTaskMemFree(listed);
                }
            }
            one = nullptr;
            got = 0;
        }
    } catch (const winrt::hresult_error&) {
        said.clear();
    }
    std::ranges::sort(said);
    const auto spare = std::ranges::unique(said);
    said.erase(spare.begin(), spare.end());
    return said;
}

std::unique_ptr<ISpeech> openSpeech() {
    return std::make_unique<WindowsSpeech>();
}

}
