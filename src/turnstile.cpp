#include "turnstile.h"

#include <utility>

thread_local std::unique_ptr<ts_pool::Semaphore> semaphore(nullptr);
thread_local bool semaphore_initialized(false);

Mutex::Mutex()
{
  refs = 0;
}
Mutex::~Mutex()
{
}
void Mutex::lock()
{
  if (!semaphore_initialized) {
    semaphore_initialized = true;
    semaphore = std::unique_ptr<ts_pool::Semaphore> {new ts_pool::Semaphore{}};
  }
  semaphore = ts_pool::ts_lock((void *) this, refs, std::move(semaphore));
}
void Mutex::unlock()
{
  semaphore = ts_pool::ts_unlock((void *) this, refs, std::move(semaphore));
}
