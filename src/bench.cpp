#include "render.hpp"
#include <vector>
#include <benchmark/benchmark.h>
#include <string>
#include <filesystem>

const char* reference_filename = "data/export/reference.png";
const char* images_filename = "data/export/input-0424.png";
const char* img_dir = "data/export";

namespace fs = std::filesystem;

void BM_Rendering_cpu(benchmark::State& st)
{
    int width;
    int height;
    int stride;
    char* ref_buffer = read_png(reference_filename, &width, &height, &stride);
    char* img_buffer = read_png(images_filename, NULL, NULL, NULL);

    gray_scale(ref_buffer, width, height, stride);
    gaussian_blur(ref_buffer, width, height, stride, 5);
    for (auto _ : st)
        render_cpu(ref_buffer, width, height, stride, img_buffer);

    st.counters["frame_rate"] = benchmark::Counter(st.iterations(), benchmark::Counter::kIsRate);
}

void BM_Rendering_gpu(benchmark::State& st)
{

    int width;
    int height;
    int stride;
    char* ref_buffer = read_png(reference_filename, &width, &height, &stride);
//    char* img_buffer = read_png(images_filename, NULL, NULL, NULL);
    for (const auto & entry : fs::directory_iterator(img_dir)){
        char* img_buffer = read_png(entry.path().generic_string().c_str(), NULL, NULL, NULL);
        for (auto _ : st)
            render(ref_buffer, width, height, stride, img_buffer, false);
    }

    st.counters["frame_rate"] = benchmark::Counter(st.iterations(), benchmark::Counter::kIsRate);
}

    BENCHMARK(BM_Rendering_cpu)
->Unit(benchmark::kMillisecond)
    ->UseRealTime();

    BENCHMARK(BM_Rendering_gpu)
->Unit(benchmark::kMillisecond)
    ->UseRealTime();

    BENCHMARK_MAIN();
