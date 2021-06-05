#pragma once

#include <iostream>
#include <thread>

#ifdef STATIC_API
#include "utils/mailbox.h"
#else
class Mailbox;
#endif
// vendor
#include "glm/glm.hpp"
#include "Eigen/Core"

using MatXf = Eigen::Matrix<float, -1, -1, Eigen::RowMajor>; // Used for rendering
using MatXd = Eigen::MatrixXd;
using VecXd = Eigen::VectorXd;

template <typename T>
using Image = Eigen::Matrix<T, -1, -1>;

#ifdef WIN32
#define API __declspec(dllexport)
#else
#define API
#endif
