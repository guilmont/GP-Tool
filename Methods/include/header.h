#pragma once

#include <iostream>
#include <fstream>
#include <thread>
#include <random>
#include <vector>
#include <memory>

#include <algorithm>
#include <unordered_map>

// vendor
#include "glm/glm.hpp"

#include "Eigen/Core"
#include "Eigen/LU"
#include "Eigen/Cholesky"

using MatXf = Eigen::Matrix<float, -1, -1, Eigen::RowMajor>; // Used for rendering
using MatXd = Eigen::MatrixXd;
using VecXd = Eigen::VectorXd;

template <typename T>
using Image = Eigen::Matrix<T, -1, -1>;

/////////////////////////////
/////////////////////////////

#ifdef WIN32
#define GP_API __declspec(dllexport)
#else
#define GP_API
#endif

/////////////////////////////
/////////////////////////////

static void pout() { std::cout << std::endl; }

template <typename TP, typename ...Args>
static void pout(TP data, Args &&...args)
{
    std::cout << data << " ";
    pout(args...);
}