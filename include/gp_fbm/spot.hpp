#pragma once
// #include <config.hpp>
#include <iostream>
#include <goptimize.hpp>
#include <Eigen/Core>

using VectorXd = Eigen::VectorXd;
using MatrixXd = Eigen::MatrixXd;

struct SpotInfo
{
  double mX, mY, // mode of 2d gaussian fitted
      lX, lY,    // deviation gaussian / spot size
      eX, eY,    // error in mode's position due noise and shape
      I0, BG,    // signal intensity
      rho;       // if spot is rotated
};

class Spot
{

  friend double transferWeight(const VectorXd &, void *);

public:
  Spot(const MatrixXd &mat);

  const SpotInfo &getSpotInfo(void) const { return info; }
  bool successful(void) const { return flag; }

private:
  uint32_t NX, NY;   // roi size
  bool flag = false; // Tells if everything was calculated properly

  SpotInfo info;
  MatrixXd roi; // region of interest

  double weightFunction(const VectorXd &v);
  bool findPositionAndSize(void);
  void refinePosition(void);

}; // class
