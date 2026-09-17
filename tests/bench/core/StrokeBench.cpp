#include "core/id/Uuid.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"
#include "core/ink/StrokeMesh.hpp"

#include <benchmark/benchmark.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <vector>

namespace phvikapen::core {
namespace {

void appendSamples(benchmark::State& state) {
    const std::int64_t sampleCount = state.range(0);

    for ([[maybe_unused]] auto iteration : state) {
        Stroke stroke{Uuid{}};
        for (std::int64_t i = 0; i < sampleCount; ++i) {
            const auto position = static_cast<float>(i);
            stroke.append(InkSample{
                .x = position,
                .y = position * 0.5F,
                .timestamp = std::chrono::microseconds{i},
            });
        }
        benchmark::DoNotOptimize(stroke.boundingBox());
    }
    state.SetComplexityN(sampleCount);
}

BENCHMARK(appendSamples)->Range(64, 4096)->Complexity(benchmark::oN);

void tessellateStroke(benchmark::State& state) {
    const std::int64_t sampleCount = state.range(0);
    Stroke stroke{Uuid{}};
    for (std::int64_t i = 0; i < sampleCount; ++i) {
        const auto position = static_cast<float>(i);
        stroke.append(InkSample{
            .x = position * 3.0F,
            .y = std::sin(position * 0.1F) * 40.0F,
            .timestamp = std::chrono::microseconds{i * 8000},
        });
    }

    std::vector<InkVertex> vertices;
    for ([[maybe_unused]] auto iteration : state) {
        vertices.clear();
        appendStroke(vertices, stroke);
        benchmark::DoNotOptimize(vertices.data());
    }
    state.SetComplexityN(sampleCount);
}

BENCHMARK(tessellateStroke)->Range(64, 4096)->Complexity(benchmark::oN);

}
}
