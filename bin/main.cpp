#include <lib/include/queues/classic_b_queue.hpp>
#include <lib/include/queues/lf_b_queue.hpp>
#include <lib/include/common/thread_pool.hpp>
#include <argparse/argparse.hpp>

int main(int argc, char* argv[]) {
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
