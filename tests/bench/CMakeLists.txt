find_package(benchmark CONFIG REQUIRED)

add_executable(phvikapen_bench
    core/StrokeBench.cpp
    core/UuidBench.cpp
)

target_link_libraries(phvikapen_bench
    PRIVATE
        PhvikaPen::core
        benchmark::benchmark_main
        phvikapen_warnings
        phvikapen_sanitizers
)

set_target_properties(phvikapen_bench PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
