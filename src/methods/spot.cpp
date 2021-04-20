#include "spot.h"

std::pair<double, double> getMeanDev(const VecXd &vec)
{
  double a = 0.0,
         a2 = 0.0;

  const uint32_t N = uint32_t(vec.size());
  for (uint32_t k = 0; k < N; k++)
  {
    a += vec(k);
    a2 += vec(k) * vec(k);
  }

  a /= double(vec.size());
  a2 /= double(vec.size());

  return {a, sqrt(a2 - a * a)};
} // getMeanDev

/////////////////////////////

Spot::Spot(const MatXd &mat)
{
  this->NX = int(mat.cols());
  this->NY = int(mat.rows());

  // Converting to integer for Poisson noise
  roi = mat.array().round();

  double BG = 0.25f * (roi.col(0).mean() + roi.row(0).mean() +
                       roi.col(NX - 1).mean() + roi.row(NY - 1).mean());

  double I0 = roi.maxCoeff() - BG;

  info.mu = {0.5 * NX, 0.5 * NY};
  info.size = {1.0, 1.0};
  info.error = {1.0, 1.0};
  info.signal = {BG, I0};
  info.rho = 0.0;

  // We maximize the likelihood to find centroid and size
  flag = findPositionAndSize();

  // To determine the error in centroid's position, we sample mX and mY
  if (flag)
    flag = refinePosition();

} // Constructor

double Spot::weightFunction(const VecXd &v)
{
  // The weight function is defined as the negative of the log-likelihood given
  // by a Poisson model

  // We use this trick to constrain nmsimplex output results
  double emx = exp(v(0)), emy = exp(v(1)),
         lx = exp(v(2)), ly = exp(v(3)),
         BG = exp(v(4)), I0 = exp(v(5)),
         erho = exp(v(6));

  double mx = NX * emx / (1.0 + emx),
         my = NY * emy / (1.0 + emy),
         rho = 2.0 * erho / (1.0 + erho) - 1.0;

  double logLike = 0.0f;
  for (uint32_t k = 0; k < NY; k++)
    for (uint32_t l = 0; l < NX; l++)
    {
      double dx = (l + 0.5 - mx) / lx,
             dy = (k + 0.5 - my) / ly,
             val = 1.0 - rho * rho;

      double mu = I0 * exp(-0.5 / val * (dx * dx + dy * dy - 2.0 * rho * dx * dy)) + BG;

      logLike += mu - roi(k, l) * log(mu);
    } // for-loop

  // Flat priors
  logLike -= v(0) + 2.0 * log(emx + 1.0);
  logLike -= v(1) + 2.0 * log(emy + 1.0);
  logLike -= v(6) + 2.0 * log(erho + 1.0);

  return logLike - v(4) - v(5);

} // weightFunction

bool Spot::findPositionAndSize(void)
{
  // The initial guess:
  //    mX and mY will be ICY location
  //    lX and lY will be 1/6 of roi's size

  VecXd vec(7);
  vec << 0.0, 0.0, log(NX / 6.0), log(NY / 6.0),
      log(info.signal[0]), log(info.signal[1]), 0.0;

  GOptimize::NMSimplex nms(vec, 1e-5);
  if (!nms.runSimplex(&Spot::weightFunction, this))
    return false;

  else
  {
    // Preparing the final values
    VecXd vx = nms.getResults().array().exp();
    info.mu = {NX * vx(0) / (1.0f + vx(0)), NY * vx(1) / (1.0f + vx(1))};
    info.size = {vx(2), vx(3)};
    info.signal = {vx(4), vx(5)};
    info.rho = 2.0 * vx(6) / (1.0 + vx(6)) - 1.0;

    return true;
  } // else

} // findPositionAndSize

bool Spot::refinePosition(void)
{
  // This functions will sample the mean position of the particle using MCMC.
  // By doing so we get an estimation of positional error

  VecXd MXY(7);
  MXY << -log(NX / info.mu.x - 1.0f), -log(NY / info.mu.y - 1.0f),
      log(info.size.x), log(info.size.y), log(info.signal[0]), log(info.signal[1]),
      -log(2.0 / (info.rho + 1.0) - 1.0f);

  uint32_t maxMCS = 5000;
  MatXd stats = GOptimize::sampleParameters(MXY, maxMCS, &Spot::weightFunction, this);

  MatXd pos(maxMCS, 2);
  for (uint32_t k = 0; k < maxMCS; k++)
  {
    double mx = exp(stats(k, 0)), my = exp(stats(k, 1));
    pos(k, 0) = NX * mx / (1.0f + mx);
    pos(k, 1) = NY * my / (1.0f + my);
  }

  double devx = getMeanDev(pos.col(0)).second;
  double devy = getMeanDev(pos.col(1)).second;

  // Doing some statistics and creating output
  if (std::isnan(devx) || std::isnan(devy))
  {
    return false;
  }
  else
  {
    info.error = {devx, devy};
    return true;
  }

} // refinePosition
