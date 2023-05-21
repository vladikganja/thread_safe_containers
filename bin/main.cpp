#include <argparse/argparse.hpp>
#include <benchmark/benchmark.h>

#include <benches/queue_benches/quick_sort.hpp>

int main(int argc, char* argv[]) 
try {
    // argparse::ArgumentParser parser("ts_conainers");
    // parser.add_argument("cores").default_value(1).scan<'i', uint64_t>();
    // parser.add_argument("size").default_value(1).scan<'i', uint64_t>();
    // parser.parse_args(argc, argv);
    // auto cores = parser.get<uint64_t>("cores");
    // auto size = parser.get<uint64_t>("size");

    std::vector<int> a{4, 7, 2, 7, 4, 5, 2, 6, 52425, 4, 264, 326};

    //auto start = std::chrono::high_resolution_clock::now();
    bench::quickSort(2, a.begin(), a.end());
    INFO(a);
    // auto end = std::chrono::high_resolution_clock::now();
    // auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    //INFO(us, " microseconds");

    return EXIT_SUCCESS;

} catch(std::exception& ex) {
    INFO(ex.what());
} catch(...) {
    INFO("undefined error");
}

std::vector<int> generate(uint64_t size) {
    std::vector<int> res(size);
    for (int i = 0; i < size; i++) {
        res[i] = rand() % size;
    }
    return res;
}

static void QuickSortBenchmark(benchmark::State& state, uint64_t size, uint64_t cores) {
    auto a = generate(size);

        // Perform setup here
    while (state.KeepRunning()) {
        // This code gets timed
        bench::quickSort(cores, a.begin(), a.end());
    }
}

BENCHMARK_CAPTURE(QuickSortBenchmark, 1, 100'000, 1)->Iterations(1);
BENCHMARK_CAPTURE(QuickSortBenchmark, 2, 100'000, 2)->Iterations(1);
BENCHMARK_CAPTURE(QuickSortBenchmark, 3, 100'000, 3)->Iterations(1);
BENCHMARK_CAPTURE(QuickSortBenchmark, 4, 100'000, 4)->Iterations(1);
BENCHMARK_CAPTURE(QuickSortBenchmark, 5, 100'000, 5)->Iterations(1);
BENCHMARK_CAPTURE(QuickSortBenchmark, 6, 100'000, 6)->Iterations(1);
BENCHMARK_CAPTURE(QuickSortBenchmark, 7, 100'000, 7)->Iterations(1);
BENCHMARK_CAPTURE(QuickSortBenchmark, 8, 100'000, 8)->Iterations(1);
BENCHMARK_CAPTURE(QuickSortBenchmark, 9, 100'000, 9)->Iterations(1);
BENCHMARK_CAPTURE(QuickSortBenchmark, 10, 100'000, 10)->Iterations(1);
BENCHMARK_CAPTURE(QuickSortBenchmark, 11, 100'000, 11)->Iterations(1);
BENCHMARK_CAPTURE(QuickSortBenchmark, 12, 100'000, 12)->Iterations(1);
BENCHMARK_CAPTURE(QuickSortBenchmark, 13, 100'000, 13)->Iterations(1);
BENCHMARK_CAPTURE(QuickSortBenchmark, 14, 100'000, 14)->Iterations(1);
BENCHMARK_CAPTURE(QuickSortBenchmark, 15, 100'000, 15)->Iterations(1);
BENCHMARK_CAPTURE(QuickSortBenchmark, 16, 100'000, 16)->Iterations(1);

// Run the benchmark
//BENCHMARK_MAIN();
