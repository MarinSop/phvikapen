#include "platform/text/win/WindowsHandwriting.hpp"

#include "core/Error.hpp"
#include "core/geometry/Rect.hpp"
#include "core/id/Uuid.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"
#include "core/text/InkWord.hpp"
#include "platform/text/IHandwriting.hpp"

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.Numerics.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Input.Inking.Analysis.h>
#include <winrt/Windows.UI.Input.Inking.h>
#include <winrt/base.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace phvikapen::platform::text {
namespace {

using winrt::Windows::Foundation::Point;
using winrt::Windows::Foundation::Rect;
using winrt::Windows::Foundation::Numerics::float3x2;
using winrt::Windows::UI::Input::Inking::InkPoint;
using winrt::Windows::UI::Input::Inking::InkRecognizerContainer;
using winrt::Windows::UI::Input::Inking::InkStroke;
using winrt::Windows::UI::Input::Inking::InkStrokeBuilder;
using winrt::Windows::UI::Input::Inking::Analysis::InkAnalysisInkWord;
using winrt::Windows::UI::Input::Inking::Analysis::InkAnalysisNodeKind;
using winrt::Windows::UI::Input::Inking::Analysis::InkAnalyzer;

// A dot is written with a single point, and a stroke needs two to exist.
constexpr float kDotWidth = 0.05F;

[[nodiscard]] core::Error asError(const winrt::hresult_error& failure) {
    return core::Error{
        .code = core::ErrorCode::Unknown,
        .message = winrt::to_string(failure.message()),
    };
}

[[nodiscard]] std::vector<InkPoint> pointsOf(const core::Stroke& stroke) {
    const std::span<const core::InkSample> samples = stroke.samples();
    std::vector<InkPoint> points;
    points.reserve(samples.size() + 1);
    for (const core::InkSample& sample : samples) {
        points.emplace_back(Point{sample.x, sample.y}, std::clamp(sample.pressure, 0.0F, 1.0F));
    }
    if (points.size() == 1) {
        points.emplace_back(Point{samples.front().x + kDotWidth, samples.front().y},
                            std::clamp(samples.front().pressure, 0.0F, 1.0F));
    }
    return points;
}

[[nodiscard]] core::Rect boxOf(const Rect& rect) {
    return core::Rect{
        .left = rect.X,
        .top = rect.Y,
        .right = rect.X + rect.Width,
        .bottom = rect.Y + rect.Height,
    };
}

}

WindowsHandwriting::WindowsHandwriting() {
    try {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        m_apartment = true;
    } catch (const winrt::hresult_error&) {
        // The thread was already put in an apartment by somebody else, which is theirs to end.
    }
}

WindowsHandwriting::~WindowsHandwriting() {
    if (m_apartment) {
        winrt::uninit_apartment();
    }
}

core::Result<std::vector<core::InkWord>>
WindowsHandwriting::read(std::span<const core::Stroke> strokes) {
    std::vector<core::InkWord> words;
    if (strokes.empty()) {
        return words;
    }
    try {
        InkStrokeBuilder builder;
        InkAnalyzer analyzer;
        std::unordered_map<std::uint32_t, core::Uuid> ours;
        for (const core::Stroke& stroke : strokes) {
            if (stroke.empty()) {
                continue;
            }
            const InkStroke made = builder.CreateStrokeFromInkPoints(
                winrt::single_threaded_vector<InkPoint>(pointsOf(stroke)), float3x2::identity());
            ours.emplace(made.Id(), stroke.id());
            analyzer.AddDataForStroke(made);
        }
        if (ours.empty()) {
            return words;
        }

        analyzer.AnalyzeAsync().get();
        for (const auto& node : analyzer.AnalysisRoot().FindNodes(InkAnalysisNodeKind::InkWord)) {
            const auto word = node.as<InkAnalysisInkWord>();
            std::string text = winrt::to_string(word.RecognizedText());
            if (text.empty()) {
                continue;
            }
            std::vector<core::Uuid> written;
            for (const std::uint32_t id : word.GetStrokeIds()) {
                if (const auto found = ours.find(id); found != ours.end()) {
                    written.push_back(found->second);
                }
            }
            words.push_back(core::InkWord{
                .text = std::move(text),
                .box = boxOf(word.BoundingRect()),
                .strokes = std::move(written),
            });
        }
        return words;
    } catch (const winrt::hresult_error& failure) {
        return std::unexpected{asError(failure)};
    }
}

std::vector<std::string> WindowsHandwriting::languages() {
    std::vector<std::string> names;
    try {
        InkRecognizerContainer container;
        if (!container) {
            return names;
        }
        for (const auto& recognizer : container.GetRecognizers()) {
            names.push_back(winrt::to_string(recognizer.Name()));
        }
    } catch (const winrt::hresult_error&) {
        names.clear();
    }
    return names;
}

std::unique_ptr<IHandwriting> openHandwriting() {
    return std::make_unique<WindowsHandwriting>();
}

}
