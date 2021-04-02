#include <threadpool.hpp>

Threadpool::Threadpool(MSG_Progress *message) : mail(message)
{
    this->nThr = std::thread::hardware_concurrency();
    vThr.resize(nThr);
}

Threadpool::Threadpool(const uint32_t nThreads, MSG_Progress *message)
    : nThr(nThreads), mail(message)
{
    vThr.resize(nThreads);
}

void Threadpool::resize(const uint32_t nThreads)
{
    nThr = nThreads;
    vThr.resize(nThreads);
} // resize

Threadpool &Threadpool::operator=(Threadpool &&pool)
{
    this->nThr = pool.nThr;
    this->mail = pool.mail;
    this->vThr = std::move(pool.vThr);

    return *this;
}

Threadpool &Threadpool::operator=(const Threadpool &pool)
{
    this->nThr = pool.nThr;
    this->mail = pool.mail;
    this->vThr.resize(this->nThr);

    return *this;
}

void Threadpool::run(void (*function)(const uint32_t, void *), const uint32_t N,
                     void *ptr)
{
    thrCounter = N;

    auto runStuff = [&](void) -> void {
        while (true)
        {
            int k = --thrCounter;
            if (k < 0)
                return;

            function(k, ptr);

            if (mail)
            {
                mtx.lock();
                mail->update(float(N - k) / float(N));
                mtx.unlock();
            }

        } // while-true
    };    // runstuff

    for (std::thread &thr : vThr)
        thr = std::thread(runStuff);

    for (std::thread &thr : vThr)
        thr.join();

    if (mail)
    {
        mail->update(1.0f);
        mail->markAsRead();
    }
} // run -> num-members with void pointer
