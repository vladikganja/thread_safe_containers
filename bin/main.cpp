#include <argparse/argparse.hpp>

#include <benches/queue_benches/quick_sort.hpp>

#include <lib/queues/uniQueue.hpp>

std::vector<int> generate(uint64_t size) {
    std::vector<int> res(size);
    for (int i = 0; i < size; i++) {
        res[i] = rand() % size;
    }
    return res;
}

int main(int argc, char* argv[]) 
try {
    uniQueue<Task, uniQSpec(Base::Array, Bounded::No, Priority::Yes)> queue;

    return 0;

    argparse::ArgumentParser parser("ts_conainers");
    parser.add_argument("cores").default_value(1).scan<'i', uint64_t>();
    parser.add_argument("size").default_value(1).scan<'i', uint64_t>();
    parser.parse_args(argc, argv);
    auto cores = parser.get<uint64_t>("cores");
    auto size = parser.get<uint64_t>("size");

    auto a = generate(size);

    //auto start = std::chrono::high_resolution_clock::now();
    bench::quickSort(cores, a.begin(), a.end());
    //auto end = std::chrono::high_resolution_clock::now();
    //auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    //INFO(us, " microseconds");

    return EXIT_SUCCESS;

} catch(std::exception& ex) {
    INFO(ex.what());
} catch(...) {
    INFO("undefined error");
}
