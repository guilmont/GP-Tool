// VERSION: 2.0
// Provides Nelder-Mead minimization algorithm
// https://en.wikipedia.org/wiki/Nelder%E2%80%93Mead_method

// It also provides a method to determine the probability density function for
// optimized paramters

#pragma once

#include <iostream>
#include <algorithm>
#include <random>

#include <Eigen/Core>

using VecXd = Eigen::VectorXd;
using MatXd = Eigen::MatrixXd;

namespace GOptimize
{
    struct Vertex
    {
        VecXd pos;
        double weight;
        Vertex(void) = default;
        Vertex(const VecXd &pos, double weight);

        Vertex(const Vertex &vt);
        Vertex(Vertex &&vt);

        Vertex &operator=(const Vertex &vt);
        Vertex &operator=(Vertex &&vt);
    }; // struct-vertex

    /////////////////////////////////////////////

    class NMSimplex
    {
    public:
        NMSimplex(const VecXd &vec, double thres, double step = 1);
        NMSimplex(VecXd &&vec, double thres, double step = 1);

        void setMaxIterations(uint32_t num) { maxIterations = num; }
        VecXd getResults(void) const { return params; }

        template <class CL>
        bool runSimplex(double (CL::*weight)(const VecXd &), CL *ptr);

    private:
        uint32_t
            numParams,
            maxIterations = 10000;

        double
            sizeThres, // Accepted average longest distance between simplex vertices
            step;      // for simplex initialization

        VecXd params;                // To store parameters
        std::vector<Vertex> simplex; // Vector containing simplex vertices

        double GetSimplexSize();

    }; // class NMSimplex

    // /////////////////////////////////////////////

    // This function will use markov chain monte carlo to estimate the probability
    // distribution of each parameter
    template <class CL>
    MatXd sampleParameters(const VecXd &params, const uint32_t sample_size,
                           double (CL::*func)(const VecXd &), CL *ptr);

} // namespace GOptimize

///////////////////////////////////////////////////////////////////////////////

template <class CL>
bool GOptimize::NMSimplex::runSimplex(double (CL::*weight)(const VecXd &), CL *ptr)
{
    // Some parameters needed by the method.Suggested values used
    double alpha = 1.0, gamma = 2.0, rho = 0.5, sigma = 0.5;

    // Initializing Simplex
    VecXd vec(this->params);
    simplex.emplace_back(vec, (ptr->*weight)(vec));

    for (uint32_t k = 0; k < numParams; k++)
    {
        vec(k) += step;
        if (k > 0)
            vec(k - 1) -= step;
        simplex.emplace_back(vec, (ptr->*weight)(vec));
    } // loop-vertex

    // Starting on the simplex method
    uint32_t counter = 0;
    while (counter++ < maxIterations)
    {
        // Determine simplex size
        double size = GetSimplexSize();
        if (size < sizeThres)
        { // Saving final values to be returned
            params = simplex.at(0).pos;
            return true;
        }

        // Sorting from smallest to biggest vertex-weight
        sort(simplex.begin(), simplex.end(),
             [](const Vertex &a, const Vertex &b) { return a.weight < b.weight; });

        // Calculate centroid
        VecXd CM(numParams);
        CM.fill(0);

        for (uint32_t k = 0; k < numParams; k++)
            CM += simplex.at(k).pos;

        CM /= double(numParams); // Norma

        // Calculate the refletion point
        vec = CM + alpha * (CM - simplex.at(numParams).pos);
        Vertex RF(vec, (ptr->*weight)(vec));
        if (RF.weight < simplex.at(numParams - 1).weight && RF.weight > simplex.at(0).weight)
        {
            simplex.at(numParams) = std::move(RF);
        }
        else if (RF.weight < simplex.at(0).weight)
        {
            // Calculate expansion point
            vec = CM + gamma * (RF.pos - CM);
            Vertex EX(vec, (ptr->*weight)(vec));
            if (EX.weight < RF.weight)
                simplex.at(numParams) = std::move(EX);
            else
                simplex.at(numParams) = std::move(RF);
        }
        else
        {
            // Calculate contraction point
            vec = CM + rho * (simplex.at(numParams).pos - CM);
            Vertex CT(vec, (ptr->*weight)(vec));
            if (CT.weight < simplex.at(numParams).weight)
                simplex.at(numParams) = std::move(CT);
            else
            {
                // Shrink
                Vertex vx0 = simplex.at(0);
                for (uint32_t k = 1; k <= numParams; k++)
                    simplex.at(k).pos = vx0.pos + sigma * (simplex.at(k).pos - vx0.pos);
            }

        } // main else

    } // while loop

    return false;
}

///////////////// DISTRIBUTION ///////////////////////
template <class CL>
MatXd GOptimize::sampleParameters(const VecXd &params, const uint32_t sample_size,
                                  double (CL::*func)(const VecXd &), CL *ptr)
{
    // This functions will sample the used parameters via MCMC.

    const uint32_t N = params.size(); // number of parameters

    VecXd DA(params), da(params);
    MatXd mcmc(sample_size, N);

    // Maximum likelihood
    double LIKE = -(ptr->*func)(DA);
    double S = 0.02; // proposal deviation

    std::mt19937 rng(time(NULL));
    std::uniform_real_distribution<double> unif(0.0, 1.0);
    std::normal_distribution<double> normal(0.0, S);

    ///////////////////////////////////////////////////
    // calibrating variance for "good" acceptance ratio ~0.25
    const uint32_t nStep = 1000;
    uint32_t ct = 0;
    while (ct++ < 100)
    {
        float accepted = 0.0f;
        for (uint32_t mcs = 0; mcs < nStep; mcs++)
        {
            // Proposing new position
            for (uint32_t k = 0; k < N; k++)
                da(k) = DA(k) + normal(rng);

            // Estimating posterior and transition probabilities
            double like = -(ptr->*func)(da);
            double ratio = exp(like - LIKE);

            // Are the proposed values accepted?
            if (ratio > unif(rng))
            {
                DA = da;
                LIKE = like;
                accepted++;
            }

        } // loop-mcs

        float aux = (accepted + 1.0f) / float(nStep);
        if (abs(0.25f - aux) < 0.1)
            break;
        else
        {
            S *= aux / 0.2f;
            S = std::max<double>(S, 0.001);
            normal = std::normal_distribution<double>(0.0, S);
        }

    } // while true

    //////////////////////////////////////////////
    // Running distribution
    for (uint32_t mcs = 0; mcs < sample_size; mcs++)
    {
        // Proposing new position
        for (uint32_t k = 0; k < N; k++)
            da(k) = DA(k) + normal(rng);

        // Estimating posterior and transition probabilities
        double like = -(ptr->*func)(da);
        double ratio = exp(like - LIKE);

        // Are the proposed values accepted?
        if (ratio > unif(rng))
        {
            DA = da;
            LIKE = like;
        }

        // Saving correct values to mcmc matrix
        mcmc.row(mcs) = DA;

    } // loop-mcs

    return mcmc;

} // paramDistribution