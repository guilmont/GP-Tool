#include "spot.hpp"

std::pair<double, double> getMeanDev(const VectorXd &vec)
{
  double a = 0.0,
         a2 = 0.0;

  for (uint32_t k = 0; k < vec.size(); k++)
  {
    a += vec(k);
    a2 += vec(k) * vec(k);
  }

  a /= double(vec.size());
  a2 /= double(vec.size());

  return {a, sqrt(a2 - a * a)};
} // getMeanDev

/////////////////////////////

double transferWeight(const VectorXd &v, void *ptr)
{
  return reinterpret_cast<Spot *>(ptr)->weightFunction(v);
}

Spot::Spot(const MatrixXd &mat) : NX(mat.cols()), NY(mat.rows())
{
  roi.resize(NY, NX);

  // Converting to integer for Poisson noise
  roi = mat.array().round();

  double BG = 0.25f * (roi.col(0).mean() + roi.row(0).mean() +
                       roi.col(NX - 1).mean() + roi.row(NY - 1).mean());

  double I0 = roi.maxCoeff() - BG;

  info.mX = 0.5 * NX;
  info.mY = 0.5 * NY;
  info.lX = 1.0;
  info.lY = 1.0;
  info.eX = 0.1;
  info.eY = 0.1;
  info.I0 = I0;
  info.BG = BG;
  info.rho = 0.0;

  // We maximize the likelihood to find centroid and size
  if (findPositionAndSize())
  {
    // To determine the error in centroid's position, we sample mX and mY
    refinePosition();
    flag = true;
  }

} // Constructor

double Spot::weightFunction(const VectorXd &v)
{
  // The weight function is defined as the negative of the log-likelihood given
  // by a Poisson model

  // We use this trick to constrain nmsimplex output results
  double emx = exp(v(0)), emy = exp(v(1)),
         lx = exp(v(2)), ly = exp(v(3)),
         I0 = exp(v(4)), BG = exp(v(5)),
         erho = exp(v(6));

  double mx = NX * emx / (1.0 + emx),
         my = NY * emy / (1.0 + emy),
         rho = 2.0 * erho / (1.0 + erho) - 1.0;

  double logLike = 0.0f;
  for (uint32_t k = 0; k < NY; k++)
    for (uint32_t l = 0; l < NX; l++)
    {
      double dx = (l + 0.5f - mx) / lx,
             dy = (k + 0.5f - my) / ly,
             val = 1.0 - rho * rho;

      double mu = I0 * exp(-0.5f / val * (dx * dx + dy * dy - 2.0 * rho * dx * dy)) + BG;

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
  VectorXd vec(7);
  vec(0) = 0.0;
  vec(1) = 0.0;
  vec(2) = log(NX / 6.0);
  vec(3) = log(NY / 6.0);
  vec(4) = log(info.I0);
  vec(5) = log(info.BG);
  vec(6) = 0.0;

  GOptimize::NMSimplex<double, VectorXd> nms(vec, 1e-5);
  if (!nms.runSimplex(&transferWeight, this))
    return false;

  else
  {
    // Preparing the final values
    VectorXd vx = nms.getResults().array().exp();
    info.mX = NX * vx(0) / (1.0f + vx(0));
    info.mY = NY * vx(1) / (1.0f + vx(1));
    info.lX = vx(2);
    info.lY = vx(3);
    info.I0 = vx(4);
    info.BG = vx(5);
    info.rho = 2.0 * vx(6) / (1.0 + vx(6)) - 1.0;

    return true;
  } // else

} // findPositionAndSize

void Spot::refinePosition(void)
{
  // This functions will sample the mean position of the particle using MCMC.
  // By doing so we get an estimation of positional error

  VectorXd MXY(7);
  MXY(0) = -log(NX / info.mX - 1.0f);
  MXY(1) = -log(NY / info.mY - 1.0f);
  MXY(2) = log(info.lX);
  MXY(3) = log(info.lY);
  MXY(4) = log(info.I0);
  MXY(5) = log(info.BG);
  MXY(6) = -log(2.0 / (info.rho + 1.0) - 1.0f);

  uint32_t maxMCS = 5000;
  MatrixXd stats = GOptimize::sampleParameters<double, VectorXd, MatrixXd>(
      MXY, maxMCS, &transferWeight, this);

  MatrixXd pos(maxMCS, 2);
  for (uint32_t k = 0; k < maxMCS; k++)
  {
    double mx = exp(stats(k, 0)), my = exp(stats(k, 1));

    pos(k, 0) = NX * mx / (1.0f + mx);
    pos(k, 1) = NY * my / (1.0f + my);
  }

  double devx = getMeanDev(pos.col(0)).second;
  double devy = getMeanDev(pos.col(1)).second;

  // Doing some statistics and creating output
  info.eX = devx;
  info.eY = devy;

  if (std::isnan(info.eX))
    info.eX = double(this->NX);
  if (std::isnan(info.eY))
    info.eY = double(this->NY);

} // refinePosition
