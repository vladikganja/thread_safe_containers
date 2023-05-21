#pragma once

#include <lib/common/common.h>

namespace hp {

constexpr uint64_t THREADS_COUNT = 16;

namespace {

constexpr uint64_t HPTRS_PER_THREAD = 2;
constexpr uint64_t TOTAL_HPTRS_COUNT = HPTRS_PER_THREAD * THREADS_COUNT;
constexpr uint64_t RETIRED_COUNT = HPTRS_PER_THREAD * THREADS_COUNT * 2;

std::mutex mut;
uint64_t id = 0;

} // namespace

struct ThreadLocalHazardManager {
    std::array<ThreadLocalHazardManager*, 
        THREADS_COUNT>& globalManagerRef;

    ThreadLocalHazardManager(std::array<ThreadLocalHazardManager*, 
                             THREADS_COUNT>& globalManager)
            : globalManagerRef(globalManager) {
        std::lock_guard<std::mutex> guard(mut);
        globalManagerRef[id++]= this;
    }

    uint64_t rCount{};

    std::array<void*, HPTRS_PER_THREAD> hptrs;

    std::array<void*, RETIRED_COUNT> retired;

    void RetireNode(void* node) {
        retired[rCount++] = node;
        if (rCount == RETIRED_COUNT) {
            Scan();
        }
    }

    void Scan() {
        uint64_t p = 0;
        uint64_t newRCount = 0;
        void* hptr;
        void* hpList[TOTAL_HPTRS_COUNT];
        void* newRetired[TOTAL_HPTRS_COUNT];

        // Stage 1 – проходим по всем hptrs всех потоков
        // Собираем общий массив hpList защищенных указателей
        for (auto& thread : globalManagerRef) {
            for (uint64_t i = 0; i < HPTRS_PER_THREAD; ++i) {
                hptr = thread->hptrs[i];
                if (hptr != nullptr) {
                    hpList[p++] = hptr;
                }
            }
        }

        // Stage 2 – сортировка hazard pointer'ов
        // Сортировка нужна для последующего бинарного поиска
        std::sort(hpList, hpList + TOTAL_HPTRS_COUNT);

        // Stage 3 – удаление элементов, не объявленных как hazard
        for (uint64_t i = 0; i < RETIRED_COUNT; ++i) {
            // Если retired[i] отсутствует в списке hpList всех Hazard Pointer’ов
            // то retired[i] может быть удален
            if (std::binary_search(hpList, hpList + TOTAL_HPTRS_COUNT, retired[i])) {
                newRetired[newRCount++] = retired[i];
            } else {
                free(retired[i]);
            }
        }

        // Stage 4 – формирование нового массива отложенных элементов.
        for (uint64_t i = 0; i < newRCount; ++i) {
            retired[i] = newRetired[i];
        }

        rCount = newRCount;
    }
};


} // hp
