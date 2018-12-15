#ifndef SRC_TURNSTILE_H_
#define SRC_TURNSTILE_H_

#include "mutex_pool.h"

#include <memory>

class Mutex {
 private:
   uint64_t refs;
 public:
  Mutex();
  ~Mutex();
  Mutex(const Mutex&) = delete;

  void lock();    // NOLINT
  void unlock();  // NOLINT
};

#endif  // SRC_TURNSTILE_H_
