#pragma once

#include <iostream>
#include <fstream>
#include <thread>

#include "utils/goptimize.h"
#include "renderer/mailbox.h"

#include <glm/glm.hpp>
#include <Eigen/Core>

using MatXf = Eigen::Matrix<float, -1, -1, Eigen::RowMajor>; // Used for rendering
using MatXd = Eigen::MatrixXd;
using VecXd = Eigen::VectorXd;

template <typename T>
using Image = Eigen::Matrix<T, -1, -1>;
