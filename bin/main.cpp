#include "../lib/include/classic_b_queue.hpp"
#include "../lib/include/thread_pool.hpp"

int main() {
    ClassicBQueue<Task> queue(1024);

    ProducerPool<Task> pPool(2, 10, queue);
    ConsumerPool<Task> cPool(1, 10, queue);

    pPool.start_produce();
    cPool.start_consume();

    pPool.finish_produce();
    cPool.finish_consume();

    std::cout << cPool.consumed() << '\n';
}
