/*
 * ClauseAllocator.h
 *
 *  Created on: Feb 20, 2017
 *      Author: markus
 */

#ifndef SRC_CANDY_GlobalClauseAllocator_H_
#define SRC_CANDY_GlobalClauseAllocator_H_

#include <vector>
#include <array>
#include <cstdint>
#include <cstring>
#include <numeric>
#include <mutex>
#include <thread>
#include <unordered_map>

#include <candy/core/clauses/Clause.h>
#include <candy/core/clauses/ClauseAllocator.h>
#include <candy/core/Statistics.h>

namespace Candy {

class GlobalClauseAllocator {
public:
    GlobalClauseAllocator() : allocator(), active(false), alock(), ready() { }
    ~GlobalClauseAllocator() { }

    inline void enroll() {
        alock.lock();
        ready[std::this_thread::get_id()] = false;
        alock.unlock();
    }

    inline void absorb(ClauseAllocator& other) {
        alock.lock();
        allocator[int(active)].import(other);
        other.reset(false);
        ready[std::this_thread::get_id()] = true;

        bool everybody_ready = false;
        for (auto it : ready) {
            everybody_ready &= it.second;
        }

        if (everybody_ready) { // all threads use active allocator now
            allocator[int(!active)].free(); // free inactive pages
            allocator[int(!active)].import(allocator[int(active)]); // import active pages
            allocator[int(!active)].reallocate(ready.size() == 1); // reallocate (without free); free in single threaded scenario
            active = !active; // swap active allocator
            for (auto it : ready) {
                ready[it.first] = false;
            }
        }
        alock.unlock();
    }

    inline std::vector<Clause*> collect() {
        alock.lock();
        std::vector<Clause*> clauses = allocator[int(active)].collect();
        alock.unlock();
        return clauses;
    }

private:
    std::array<ClauseAllocator, 2> allocator;
    bool active;

    std::mutex alock;
    std::unordered_map<std::thread::id, bool> ready;

    GlobalClauseAllocator(GlobalClauseAllocator const&) = delete;
    void operator=(GlobalClauseAllocator const&) = delete;

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSEALLOCATOR_H_ */