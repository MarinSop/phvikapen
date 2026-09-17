#include "core/id/Uuid.hpp"
#include "core/id/Uuid7Generator.hpp"

#include <benchmark/benchmark.h>

namespace phvikapen::core {
namespace {

void generateUuid7(benchmark::State& state) {
    Uuid7Generator generator;

    for ([[maybe_unused]] auto iteration : state) {
        benchmark::DoNotOptimize(generator.next());
    }
}

BENCHMARK(generateUuid7);

void formatUuid(benchmark::State& state) {
    const Uuid uuid = Uuid7Generator{}.next();

    for ([[maybe_unused]] auto iteration : state) {
        benchmark::DoNotOptimize(uuid.toString());
    }
}

BENCHMARK(formatUuid);

}
}
