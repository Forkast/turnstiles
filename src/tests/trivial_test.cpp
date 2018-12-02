#include "turnstile.h"

#include <iostream>
#include <map>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

static_assert(std::is_destructible<Mutex>::value,
              "Mutex should be destriuctible.");
static_assert(!std::is_copy_constructible<Mutex>::value,
              "Mutex should not be copy-constructible.");
static_assert(!std::is_move_constructible<Mutex>::value,
              "Mutex should not be move-constructible.");
static_assert(std::is_default_constructible<Mutex>::value,
              "Mutex should be default constructible.");
static_assert(std::is_same<void, decltype(std::declval<Mutex>().lock())>::value,
              "Mutex should have a \"void lock()\" member function.");
static_assert(
    std::is_same<void, decltype(std::declval<Mutex>().unlock())>::value,
    "Mutex should have a \"void unlock()\" member function.");
static_assert(sizeof(Mutex) <= 8, "Mutex is too large");

void DummyTest() {
  int shared_cntr = 0;
  int const kNumRounds = 100;
  Mutex mu;

  std::vector<std::thread> v;
  for (int i = 0; i < 2; ++i) {
    v.emplace_back([&]() {
      for (int i = 0; i < kNumRounds; ++i) {
        std::lock_guard<Mutex> lk(mu);
        ++shared_cntr;
      }
    });
  }

  for (auto &t : v) {
    t.join();
  }

  if (shared_cntr != kNumRounds * 2) {
    throw std::logic_error("Counter==" + std::to_string(shared_cntr) +
                           " expected==" + std::to_string(kNumRounds * 2));
  }
}

int main() {
  try {
    Mutex m;
    m.lock();
    m.unlock();
    m.lock();
    m.unlock();
    m.lock();
    m.unlock();
    DummyTest();
  } catch (std::exception &e) {
    std::cout << "Exception: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
