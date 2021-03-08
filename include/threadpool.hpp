#pragma once

// To run member function declare as friend
// template <class var>
// float forward_obj(void *ptr)
// {
//     return reinterpret_cast<var *>(ptr)->member();
// }

#include <iostream>
#include <vector>

#include <thread>
#include <atomic>
#include <mutex>

#include <mailbox.hpp>

class Threadpool
{
public:
    Threadpool(MSG_Progress *message = nullptr);
    Threadpool(const uint32_t nThreads, MSG_Progress *message = nullptr);

    Threadpool &operator=(Threadpool &&pool);
    Threadpool &operator=(const Threadpool &pool);

    void resize(const uint32_t nThreads);
    void resetProgress(MSG_Progress *prog) { mail = prog; }

    uint32_t getNumThreads(void) const { return nThr; }

    // non-member functions
    void run(void (*function)(const uint32_t, void *), const uint32_t N,
             void *ptr = nullptr);

    std::mutex mtx;

private:
    uint32_t nThr;
    MSG_Progress *mail = nullptr;

    std::atomic<int> thrCounter;
    std::vector<std::thread> vThr;
};