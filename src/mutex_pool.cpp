#include "mutex_pool.h"

#include <stack>

namespace ts_pool
{
  /**
   * Hash table for turnstile chains.
   * Hash is just mutex address shifted and collected most interesting bits.
   * TAB_SIZE needs to be power of 2 for HASH_MASK to be only 1s.
   */
  const int TAB_SIZE = 128;
  const int HASH_MASK = TAB_SIZE - 1;
  const int SHIFT = 8;
  TurnstileChain chains[TAB_SIZE];
  /* security for free list */
  std::mutex sec;
  /* stack of free turnstiles */
  std::stack <std::shared_ptr<Turnstile> > ts_free;

  /**
   * Gets turnstile chain from hash table using address hashing
   */
  inline
  TurnstileChain * ts_get_chain(const void * obj) {
    return &chains[((uint64_t) obj >> SHIFT) & HASH_MASK];
  }

  /**
   * Automatically sets lock on a chain
   */
  std::shared_ptr<Turnstile> ts_lookup(const void * obj) {
    TurnstileChain * tc = ts_get_chain(obj);
    tc->sec.lock();
    for (auto i : tc->ts) {
      if (i->obj == obj) {
        return i;
      }
    }
    return nullptr;
  }

  /**
   * Find, erase from vector and put on the free_list
   */
  void ts_erase(const void * obj) {
    TurnstileChain * tc = ts_get_chain(obj);
    for (std::vector<std::shared_ptr<Turnstile> >::iterator i = tc->ts.begin(); i != tc->ts.end(); ++i) {
      if ((*i)->obj == obj) {
        sec.lock();
        ts_free.push(std::move(*i));
        sec.unlock();
        tc->ts.erase(i);
        return;
      }
    }
  }

  /**
   * If this is the second thread to call ts_lock it will lends its semaphore to the turnstile
   * and go to sleep. If it is the first one, it will only change number of references.
   */
  std::unique_ptr<Semaphore> ts_lock(const void * obj, uint64_t & refs, std::unique_ptr<Semaphore>&& sem) {
    std::shared_ptr<Turnstile> ts = ts_lookup(obj);
    TurnstileChain * tc = ts_get_chain(obj);

    assert(obj != nullptr);
    assert(sem != nullptr);

    refs++;
    /* if this is the first mutex on this turnstile do nothing */
    if (refs != 1) {
      if (ts == nullptr) {
        assert(refs == 2);
        /* this is the second reference, so we need alloc and go to sleep */
        sec.lock();
        if (!ts_free.empty()) {
          ts = ts_free.top();
          ts_free.pop();
          assert(ts != nullptr);
        }
        sec.unlock();
        /* if there is no available turnstiles on the free_list */
        if (ts == nullptr) {
          ts = std::shared_ptr<Turnstile>{new Turnstile{}};
        }
        ts->obj = obj;
        assert(ts != nullptr);
        tc->ts.push_back(ts);
      }
      /* lend your semaphore to the turnstile */
      ts->mutex_pool.push_back(std::move(sem));
      tc->sec.unlock();

      /* start sleeping */
      ts->mutex_pool.at(0)->wait();

      sem = std::move(ts->mutex_pool.back());
      ts->mutex_pool.pop_back();

      if (ts->mutex_pool.empty()) {
        ts_erase(obj);
      }

    }
    tc->sec.unlock();
    return std::move(sem);
  }

  void ts_unlock(const void * obj, uint64_t & refs) {
    std::shared_ptr<Turnstile> ts = ts_lookup(obj);
    TurnstileChain * tc = ts_get_chain(obj);

    assert(obj != nullptr);

    refs--;

    if (ts != nullptr) assert(!ts->mutex_pool.empty());

    /* If there is turnstile for this mutex, we might need to release semaphore */
    if (ts != nullptr) {
      ts->mutex_pool.at(0)->notify();
    } else {
      tc->sec.unlock();
    }
  }
}
