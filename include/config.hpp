#pragma once

#include <iostream>
#include <fstream>

#include "goptimize.hpp"
#include "threadpool.hpp"
#include "mailbox.hpp"

#include <Eigen/Core>

using MatrixXf = Eigen::Matrix<float, -1, -1, Eigen::RowMajor>;
using MatrixXd = Eigen::MatrixXd;
using VectorXd = Eigen::VectorXd;

template <typename T>
using Image = Eigen::Matrix<T, -1, -1>;

namespace TrajColumns
{
    enum : uint32_t // for route matrices
    {
        FRAME,
        TIME,
        POSX,
        ERRX,
        POSY,
        ERRY,
        SIZEX,
        SIZEY,
        INTENSITY,
        BACKGROUND,
        NCOLS
    };
} // namespace TrajColumns
