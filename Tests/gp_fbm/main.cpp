#include "header.h"


int main()
{
    std::random_device rd;
    std::default_random_engine rng(rd());
    std::uniform_real_distribution<double> unif(0.0, 1.0);

    const uint32_t
        nFrames = 512,
        nRoutes = 5000;

    MatXd params(nRoutes, 5);
    for (uint32_t k = 0; k < nRoutes; k++)
    {
        params(k, Pmtr::D) = 0.01 + 1.5 * unif(rng);
        params(k, Pmtr::A) = 0.01 + 1.89 * unif(rng);
        params(k, Pmtr::DT) = 0.1 + 0.9 * unif(rng);
        params(k, Pmtr::PREC) = 0.001 + 0.95 * unif(rng);
        params(k, Pmtr::OCC) = 0.8 * unif(rng);
    }

   
    MatXd resGP(nRoutes, 2);

    auto func_parallel = [&](uint32_t tid, uint32_t nThreads) -> void
    {
        for (uint32_t ct = tid; ct < nRoutes; ct += nThreads)
        {
            if (tid == 0)
            {
                std::cout << float(ct) / float(nRoutes) << std::endl;
                std::fflush(stdout);
            }

            MatXd route = GenTrajectories(nFrames, params.row(ct));

            double D = params(ct, Pmtr::D),
                A = params(ct, Pmtr::A);

            GP_FBM gp(route);
            const GP_FBM::DA* gp_da = gp.singleModel();
            resGP.row(ct) << (gp_da->D - D) / D, (gp_da->A - A) / A;

        } // loop-routes
    };

    uint32_t nThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> vThr(nThreads);

    for (uint32_t k = 0; k < nThreads; k++)
        vThr[k] = std::thread(func_parallel, k, nThreads);

    for (std::thread& thr : vThr)
        thr.join();

    std::ofstream arq(std::string(OPATH) + "/relative_error.txt");
    arq << resGP;
    arq.close();

    return EXIT_SUCCESS;
}

