#include <gtest/gtest.h>
#include <fstream>

#include "GPMethods.h"
#include "json/json.h"



// Saving helpers
static Json::Value jsonEigen(const VecXd& vec)
{
    Json::Value array;
    for (int64_t l = 0; l < vec.size(); l++)
        array.append(vec(l));

    return array;
}

static Json::Value jsonEigen(const MatXd& mat)
{
    Json::Value array;
    for (int64_t l = 0; l < mat.cols(); l++)
    {
        Json::Value col;
        for (int64_t k = 0; k < mat.rows(); k++)
            col.append(mat(k, l));

        array.append(std::move(col));
    }

    return array;
}


static void saveTrajCSV(const fs::path& path, const MatXd& data)
{
    std::ofstream arq(path);

    arq << "# particleID, frame, positionX, positionY\n";

    for (int64_t k = 0; k < data.rows(); k++)
        for (int64_t l = 0; l < data.cols(); l++)
            arq << data(k, l) << (l + 1 == data.cols() ? '\n' : ',');

    arq.close();
}



// GP-FBM functions
static MatXd genKernel(double D, double A, const VecXd& vt)
{
    VecXd vta = vt.array().pow(A);
    MatXd K(vt.size(), vt.size());

    for (int64_t k = 0; k < vt.size(); k++)
        for (int64_t l = k; l < vt.size(); l++)
        {
             double val = vta(k) + vta(l) - pow(abs(vt(k) - vt(l)), A);
            K(k, l) = K(l, k) = D * val;
        }

    return K;
}

static MatXd genTrajectory(const MatXd& kernel, const VecXd& vframe, const VecXd& vtime, double error)
{
    std::random_device dev;
    std::default_random_engine ran(dev());
    std::normal_distribution<double> normal(0.0, 1.0);

    int64_t N = vtime.size();

    MatXd unit(N, 2);
    for (int64_t k = 0; k < unit.size(); k++)
        unit.data()[k] = normal(ran);


    Eigen::LLT<MatXd> cholesky(kernel); // waste of energy, but whatever

    MatXd out(N, 6);
    out.col(0) = vframe;
    out.col(1) = vtime;
    out.block(0, 2, N, 2) = cholesky.matrixL() * unit;
    out.block(0, 4, N, 2).array() = error;

    return out;
}


// Image functions
static Mat3d transformMatrix(double width, double height)
{
    double
        transX = 0.2367, transY = 2.2376,
        sclX = 1.0109, sclY = 1.0101,
        rotX = 255.2404, rotY = 255.0994,
        ang = -0.0186;

    Mat3d A, B, C, D;

    A << sclX, 0.0f, (1.0f - sclX) * 0.5f * width,
        0.0f, sclY, (1.0f - sclY) * 0.5f * height,
        0.0f, 0.0f, 1.0f;

    B << 1.0, 0.0, transX + rotX,
        0.0, 1.0, transY + rotY,
        0.0, 0.0, 1.0;

    C << cos(ang), -sin(ang), 0.0,
        sin(ang), cos(ang), 0.0,
        0.0, 0.0, 1.0;

    D << 1.0, 0.0, -rotX,
        0.0, 1.0, -rotY,
        0.0, 0.0, 1.0;

    return A * B * C * D;
}

static MatXd generateImage(uint64_t height, uint64_t width, double signal, double bGround, double spotSize, const MatXd& particles)
{
    MatXd mat = bGround * MatXd::Ones(height, width);
    for (uint64_t y = 0; y < height; y++)
        for (uint64_t x = 0; x < width; x++)
            for (int64_t pt = 0; pt < particles.rows(); pt++)
            {
                double
                    dx = (double(x) + 0.5 - particles(pt, 0)) / spotSize,
                    dy = (double(y) + 0.5 - particles(pt, 1)) / spotSize;

                mat(y, x) += signal * exp(-0.5 * (dx * dx + dy * dy));
            }

    return mat;

}


static void poissonNoise(MatXd& mat)
{
    std::random_device device;
    std::default_random_engine ran(device());
    std::poisson_distribution<uint64_t> poisson(10.0);

    using param_t = std::poisson_distribution<uint64_t>::param_type;

    for (int64_t k = 0; k < mat.rows(); k++)
        for (int64_t l = 0; l < mat.cols(); l++)
        {
            poisson.param(param_t{ mat(k,l) });
            mat(k, l) = double(poisson(ran));
        }
}
