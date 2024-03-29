#ifndef _MUTEX_POOL_H_
#define _MUTEX_POOL_H_

#include <cassert>
#include <condition_variable>
#include <mutex>
#include <type_traits>
#include <utility>
#include <vector>

namespace ts_pool {

class Semaphore {
 private:
  std::mutex mut;
  std::condition_variable con;
  unsigned long count;

 public:
  Semaphore() : count{0} {};
  Semaphore(unsigned long count) : count{count} {};
  void notify() {
    std::lock_guard<decltype(mut)> lock(mut);
    ++count;
    con.notify_one();
  }

  void wait() {
    std::unique_lock<decltype(mut)> lock(mut);
    while (!count)  // Handle spurious wake-ups.
      con.wait(lock);
    --count;
  }
};

struct Turnstile {
  /* queue of semaphores. every thread sleep on the first one */
  std::vector<std::unique_ptr<Semaphore> > mutex_pool;
  /* we need to know if this is proper turstile */
  const void* obj = nullptr;
};

struct TurnstileChain {
  /* every chains has few turnstiles because hashes might collide */
  std::vector<std::shared_ptr<Turnstile> > ts;
  Semaphore sec{1};
};

std::unique_ptr<Semaphore> ts_lock(const void* obj, uint64_t& refs,
                                   std::unique_ptr<Semaphore>&& sem);

void ts_unlock(const void* obj, uint64_t& refs);

}  // namespace ts_pool

#endif /* _MUTEX_POOL_H_ */
