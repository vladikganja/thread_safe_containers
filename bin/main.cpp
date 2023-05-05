#include <lib/queues/classic_b_queue.hpp>
#include <lib/queues/lf_b_queue.hpp>
#include <lib/common/thread_pools/thread_pool.hpp>
#include <argparse/argparse.hpp>
#include <lib/common/quick_sort.hpp>

int main(int argc, char* argv[]) {
    std::vector<int> a{3, 8, 4, 6, 6, 8, 3, 5, 3, 6, 4, 5, 4, 5, 6, 885, 53, 624, 24, 246, 24, 42, 5, 4, 5, 5, 4, 3, 0};

    bench::quick_sort(2, a.begin(), a.end());

    INFO(a);

    return 0;
    argparse::ArgumentParser parser("ts_conainers");
    parser.add_argument("prod").default_value(1).scan<'i', uint64_t>();
    parser.add_argument("cons").default_value(1).scan<'i', uint64_t>();
    parser.add_argument("tasks").default_value(100).scan<'i', uint64_t>();

    parser.parse_args(argc, argv);

    auto prod = parser.get<uint64_t>("prod");
    auto cons = parser.get<uint64_t>("cons");
    auto tasks = parser.get<uint64_t>("tasks");

    ClassicBQueue<Task> queue(32);
    LockFreeBQueue_basic<Task> lfqueue(32);

    ProducerPool<Task, LockFreeBQueue_basic> pPool(prod, tasks, lfqueue);
    ConsumerPool<Task, LockFreeBQueue_basic> cPool(cons, tasks, lfqueue);

    INFO("launch bench!");

    cPool.start_consume();
    pPool.start_produce();

    pPool.finish_produce();
    cPool.finish_consume();

    std::cout << cPool.consumed() << '\n';
}
