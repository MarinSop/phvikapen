#include "platform/ocr/win/WindowsReadPicture.hpp"

#include "core/Error.hpp"
#include "platform/ocr/IReadPicture.hpp"

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.Ocr.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/base.h>

#include <QMetaObject>
#include <Qt>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace phvikapen::platform::ocr {
namespace {

using winrt::Windows::Foundation::Rect;
using winrt::Windows::Globalization::Language;
using winrt::Windows::Graphics::Imaging::BitmapAlphaMode;
using winrt::Windows::Graphics::Imaging::BitmapDecoder;
using winrt::Windows::Graphics::Imaging::BitmapPixelFormat;
using winrt::Windows::Graphics::Imaging::BitmapTransform;
using winrt::Windows::Graphics::Imaging::ColorManagementMode;
using winrt::Windows::Graphics::Imaging::ExifOrientationMode;
using winrt::Windows::Graphics::Imaging::SoftwareBitmap;
using winrt::Windows::Media::Ocr::OcrEngine;
using winrt::Windows::Media::Ocr::OcrLine;
using winrt::Windows::Media::Ocr::OcrResult;
using winrt::Windows::Media::Ocr::OcrWord;
using winrt::Windows::Storage::Streams::DataWriter;
using winrt::Windows::Storage::Streams::InMemoryRandomAccessStream;

// A picture larger than the reader accepts is made smaller before it is read. The corners come back
// as shares of the whole, so making it smaller changes nothing about where the words are said to
// be.
[[nodiscard]] std::uint32_t narrowedTo(std::uint32_t side, std::uint32_t longest,
                                       std::uint32_t most) {
    if (longest <= most || longest == 0) {
        return std::max<std::uint32_t>(1, side);
    }
    const double by = static_cast<double>(most) / static_cast<double>(longest);
    return std::max<std::uint32_t>(1, static_cast<std::uint32_t>(static_cast<double>(side) * by));
}

[[nodiscard]] InMemoryRandomAccessStream streamOf(std::span<const std::byte> picture) {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(picture.size());
    for (const std::byte one : picture) {
        bytes.push_back(static_cast<std::uint8_t>(one));
    }
    const InMemoryRandomAccessStream stream;
    DataWriter writer{stream};
    writer.WriteBytes(bytes);
    writer.StoreAsync().get();
    writer.DetachStream();
    stream.Seek(0);
    return stream;
}

[[nodiscard]] SoftwareBitmap shownIn(const InMemoryRandomAccessStream& stream, std::uint32_t most) {
    const BitmapDecoder decoder = BitmapDecoder::CreateAsync(stream).get();
    const std::uint32_t wide = decoder.PixelWidth();
    const std::uint32_t high = decoder.PixelHeight();
    const std::uint32_t longest = std::max(wide, high);
    const BitmapTransform transform;
    transform.ScaledWidth(narrowedTo(wide, longest, most));
    transform.ScaledHeight(narrowedTo(high, longest, most));
    return decoder
        .GetSoftwareBitmapAsync(BitmapPixelFormat::Bgra8, BitmapAlphaMode::Premultiplied, transform,
                                ExifOrientationMode::RespectExifOrientation,
                                ColorManagementMode::ColorManageToSRgb)
        .get();
}

[[nodiscard]] OcrEngine readerFor(const std::string& language) {
    if (!language.empty()) {
        const winrt::hstring tag = winrt::to_hstring(language);
        if (Language::IsWellFormed(tag)) {
            if (const OcrEngine reader = OcrEngine::TryCreateFromLanguage(Language{tag})) {
                return reader;
            }
        }
    }
    return OcrEngine::TryCreateFromUserProfileLanguages();
}

[[nodiscard]] float shareOf(double along, double whole) {
    if (whole <= 0.0) {
        return 0.0F;
    }
    return static_cast<float>(std::clamp(along / whole, 0.0, 1.0));
}

[[nodiscard]] std::vector<Found> foundIn(const OcrResult& read, double wide, double high) {
    std::vector<Found> found;
    for (const OcrLine& line : read.Lines()) {
        std::string said = winrt::to_string(line.Text());
        if (said.empty()) {
            continue;
        }
        bool any = false;
        double left = wide;
        double top = high;
        double right = 0.0;
        double bottom = 0.0;
        for (const OcrWord& word : line.Words()) {
            const Rect box = word.BoundingRect();
            left = std::min(left, static_cast<double>(box.X));
            top = std::min(top, static_cast<double>(box.Y));
            right = std::max(right, static_cast<double>(box.X) + box.Width);
            bottom = std::max(bottom, static_cast<double>(box.Y) + box.Height);
            any = true;
        }
        if (!any) {
            continue;
        }
        found.push_back(Found{
            .text = std::move(said),
            .left = shareOf(left, wide),
            .top = shareOf(top, high),
            .right = shareOf(right, wide),
            .bottom = shareOf(bottom, high),
        });
    }
    std::ranges::stable_sort(found, {}, &Found::top);
    return found;
}

[[nodiscard]] core::Result<std::vector<Found>> readThere(std::span<const std::byte> picture,
                                                         const std::string& language) {
    const OcrEngine reader = readerFor(language);
    if (!reader) {
        return core::makeError(core::ErrorCode::Unsupported,
                               "this machine cannot read words in that language");
    }
    const SoftwareBitmap shown = shownIn(streamOf(picture), OcrEngine::MaxImageDimension());
    if (!shown) {
        return core::makeError(core::ErrorCode::Unsupported,
                               "this picture is not in a form the machine can open");
    }
    const OcrResult read = reader.RecognizeAsync(shown).get();
    return foundIn(read, static_cast<double>(shown.PixelWidth()),
                   static_cast<double>(shown.PixelHeight()));
}

// The whole reading happens here, away from the thread that asked, so nothing may be thrown out of
// it: a thread that throws ends the application rather than the reading.
[[nodiscard]] core::Result<std::vector<Found>> whatIsIn(std::span<const std::byte> picture,
                                                        const std::string& language) noexcept {
    bool ours = false;
    try {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        ours = true;
    } catch (const winrt::hresult_error&) {
        // The thread was already put in an apartment by somebody else, which is theirs to end.
    }
    core::Result<std::vector<Found>> got =
        core::makeError(core::ErrorCode::Unknown, "the words in this picture could not be read");
    try {
        got = readThere(picture, language);
    } catch (const winrt::hresult_error& failure) {
        got = core::makeError(core::ErrorCode::Unknown, winrt::to_string(failure.message()));
    } catch (const std::exception& failure) {
        got = core::makeError(core::ErrorCode::Unknown, failure.what());
    }
    if (ours) {
        winrt::uninit_apartment();
    }
    return got;
}

}

WindowsReadPicture::~WindowsReadPicture() {
    giveUp();
}

void WindowsReadPicture::read(std::span<const std::byte> picture, const std::string& language,
                              Read read) {
    giveUp();
    const auto answer = std::make_shared<Read>(std::move(read));
    if (picture.empty()) {
        (*answer)(
            core::makeError(core::ErrorCode::InvalidArgument, "there is no picture here to read"));
        return;
    }
    // The bytes are carried by the reading rather than borrowed, because whoever asked is free to
    // let go of the picture the moment the asking is over.
    const auto bytes =
        std::make_shared<const std::vector<std::byte>>(picture.begin(), picture.end());
    m_wanted = std::make_shared<std::atomic_bool>(true);
    m_reading = std::jthread{[this, bytes, answer, wanted = m_wanted, spoken = language] {
        core::Result<std::vector<Found>> got = whatIsIn(*bytes, spoken);
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

void WindowsReadPicture::giveUp() {
    if (m_wanted) {
        m_wanted->store(false);
    }
    if (m_reading.joinable()) {
        m_reading.join();
    }
}

std::vector<std::string> WindowsReadPicture::languages() {
    std::vector<std::string> said;
    try {
        for (const Language& one : OcrEngine::AvailableRecognizerLanguages()) {
            std::string tag = winrt::to_string(one.LanguageTag());
            if (!tag.empty()) {
                said.push_back(std::move(tag));
            }
        }
    } catch (const winrt::hresult_error&) {
        said.clear();
    }
    std::ranges::sort(said);
    return said;
}

std::unique_ptr<IReadPicture> openReadPicture() {
    return std::make_unique<WindowsReadPicture>();
}

}
