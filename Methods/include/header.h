#pragma once

#include <iostream>
#include <filesystem>
#include <fstream>
#include <thread>
#include <random>
#include <vector>
#include <memory>

#include <algorithm>
#include <unordered_map>

// vendor
#include "Eigen/Core"
#include "Eigen/LU"
#include "Eigen/Cholesky"

namespace fs = std::filesystem;

using MatXf = Eigen::Matrix<float, -1, -1, Eigen::RowMajor>; // Used for rendering

using Vec2u = Eigen::Matrix<uint32_t, 2, 1>;
using Vec2d = Eigen::Matrix<double, 2, 1>;
using Vec3d = Eigen::Matrix<double, 3, 1>;
using Mat3d = Eigen::Matrix<double, 3, 3>;

using VecXd = Eigen::VectorXd;
using MatXd = Eigen::MatrixXd;

template <typename T>
using Image = Eigen::Matrix<T, -1, -1, Eigen::RowMajor>;

/////////////////////////////
/////////////////////////////

#ifdef WIN32
#ifdef GBUILD_DLL
#define GP_API __declspec(dllexport)
#else
#define GP_API __declspec(dllimport)
#endif
#else
#define GP_API
#endif

/////////////////////////////
/////////////////////////////

namespace GPT
{

   // PRINTING UTILITY
   static void pout() { std::cout << std::endl; }

   template <typename TP, typename... Args>
   static void pout(TP data, Args &&...args)
   {
      std::cout << data << " ";
      pout(args...);
   }
}