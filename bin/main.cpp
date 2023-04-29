#include "../lib/include/classic_b_queue.hpp"
#include "../lib/include/thread_pool.hpp"

int main() {
#if __cplusplus >= 201703L
    std::cout << "It's C++17 or higher" << std::endl;
#else
    std::cout << "It's C++" << std::endl;
#endif

    ClassicBQueue<Task> queue;

    ProducerPool<Task> pool(1, 100, queue);

    pool.start();



}
