#include "header.h"


int main()
{
    std::random_device rd;
    std::default_random_engine rng(rd());
    std::uniform_real_distribution<double> unif(0.0, 1.0);

    const uint32_t
        nFrames = 256,
        nRoutes = 5000;


    constexpr double
        D1 = 0.89, A1 = 0.33,
        D2 = 1.11, A2 = 0.25,
        DR = 0.1, AR = 1.0;

    constexpr double
        DT = 0.25,
        PREC = 0.25,
        OCC = 0.1;
    
       
    MatXd resGP(nRoutes, 6);

    auto func_parallel = [&](uint32_t tid, uint32_t nThreads) -> void
    {
        for (uint32_t ct = tid; ct < nRoutes; ct += nThreads)
        {
            if (tid == 0)
            {
                std::cout << float(ct) / float(nRoutes) << std::endl;
                std::fflush(stdout);
            }

            MatXd route1 = GenTrajectories(nFrames, D1, A1, DT, PREC);
            MatXd route2 = GenTrajectories(nFrames, D2, A2, DT, PREC);
            MatXd routeR = GenTrajectories(nFrames, DR, AR, DT, 0.0001);

            // Inserting background movement
            route1.col(Track::POSX) += routeR.col(Track::POSX);
            route1.col(Track::POSY) += routeR.col(Track::POSY);

            route2.col(Track::POSX) += routeR.col(Track::POSX);
            route2.col(Track::POSY) += routeR.col(Track::POSY);

            // Resizing to normal size
            MatXd r1 = route1.block(0,0,256, route1.cols());
            MatXd r2 = route2.block(0,0,256, route2.cols());


          // Removing frames for occlusion
            addOcclusions(OCC, r1);
            addOcclusions(OCC, r2);


            GP_FBM gp(std::vector<MatXd>{r1, r2});
            const GP_FBM::CDA* cda = gp.coupledModel();
                        
            resGP.row(ct) << (cda->da[0].D - D1) / D1, (cda->da[0].A - A1) / A1, 
                             (cda->da[1].D - D2) / D2, (cda->da[1].A - A2) / A2,
                             (cda->DR - DR) / DR, (cda->AR - AR) / AR;

        } // loop-routes
    };


        uint32_t nThreads = std::thread::hardware_concurrency();
        std::vector<std::thread> vThr(nThreads);

        for (uint32_t k = 0; k < nThreads; k++)
            vThr[k] = std::thread(func_parallel, k, nThreads);

        for (std::thread& thr : vThr)
            thr.join();

        std::ofstream arq(std::string(OPATH) + "/relative_error_substrate.txt");
        arq << resGP;
        arq.close();

    return EXIT_SUCCESS;
}

