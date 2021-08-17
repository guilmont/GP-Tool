#pragma once

#include "header.h"
#include"goptimize.h"

namespace GPT
{

    struct SpotInfo
    {
        Vec2d
            mu,     // mode of 2d gaussian fitted
            size,   // deviation gaussian / spot size
            error,  // error in mode's position due noise and shape
            signal; // signal intensity // bg and top

        double rho = 0; // if spot is rotated
    };

    class Spot
    {

    public:
        Spot(const MatXd &mat);

        const SpotInfo &getSpotInfo(void) const { return info; }
        bool successful(void) const { return flag; }

    private:
        bool flag = false; // Tells if everything was calculated properly

        uint32_t NX, NY;
        MatXd roi; // region of interest
        SpotInfo info;

        double weightFunction(const VecXd &v);
        bool findPositionAndSize(void);
        bool refinePosition(void);

    }; // class

}
