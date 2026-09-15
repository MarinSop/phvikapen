#include "core/ink/Stroke.hpp"

#include <benchmark/benchmark.h>

#include <chrono>
#include <cstdint>

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

} // namespace
} // namespace phvikapen::core
