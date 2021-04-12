#pragma once

#include <iostream>
#include <algorithm>
#include <random>

// VERSION: 3.0
// Provides Nelder-Mead minimization algorithm
// https://en.wikipedia.org/wiki/Nelder%E2%80%93Mead_method

// It also provides a method to determine the probability density function for
// optimized parameters


namespace GOptimize
{
    template <typename T, typename VAR>
    struct Vertex
    {
        T weight;
        VAR pos;
        Vertex(void) = default;
        Vertex(const VAR &pos, T weight);

        Vertex(const Vertex &vt);
        Vertex(Vertex &&vt);

        Vertex &operator=(const Vertex &vt);
        Vertex &operator=(Vertex &&vt);
    }; // struct-vertex

    /////////////////////////////////////////////

    template <typename TYPE, typename VAR>
    class NMSimplex
    {
    public:
        NMSimplex(const VAR &vec, TYPE thres, TYPE step = 1);
        NMSimplex(VAR &&vec, TYPE thres, TYPE step = 1);

        void setMaxIterations(uint32_t num) { maxIterations = num; }
        VAR getResults(void) const { return params; }

        bool runSimplex(TYPE (*weight)(const VAR &, void *),
                        void *ptr = nullptr);

    private:
        uint32_t numParams;
        uint32_t maxIterations = 10000;

        TYPE
            sizeThres, // Accepted average longest distance between simplex vertices
            step;      // for simplex initialization

        VAR params;                             // To store parameters
        std::vector<Vertex<TYPE, VAR>> simplex; // Vector containing simplex vertices

        TYPE GetSimplexSize();

    }; // class NMSimplex

    /////////////////////////////////////////////

    // This function will use markov chain monte carlo to estimate the probability
    // distribution of each parameter
    template <typename T, typename VEC, typename VAR>
    VAR sampleParameters(const VEC &params, const uint32_t sample_size,
                         T (*func)(const VEC &, void *), void *ptr = nullptr);

} // namespace GOptimize

//////////////////
// LET'S IMPLMENT ALL THOSE FUNCTIONS HERE

template <typename T, typename VAR>
GOptimize::Vertex<T, VAR>::Vertex(const VAR &pos, T weight)
    : weight(weight), pos(pos) {}

template <typename T, typename VAR>
GOptimize::Vertex<T, VAR>::Vertex(const Vertex &vt) : weight(vt.weight), pos(vt.pos) {}

template <typename T, typename VAR>
GOptimize::Vertex<T, VAR>::Vertex(Vertex &&vt)
    : weight(std::move(vt.weight)), pos(std::move(vt.pos)) {}

template <typename T, typename VAR>
GOptimize::Vertex<T, VAR> &GOptimize::Vertex<T, VAR>::operator=(const Vertex &vt)
{
    if (&vt != this)
    {
        pos = vt.pos;
        weight = vt.weight;
    }

    return *this;
}

template <typename T, typename VAR>
GOptimize::Vertex<T, VAR> &GOptimize::Vertex<T, VAR>::operator=(Vertex &&vt)
{
    if (&vt != this)
    {
        pos = std::move(vt.pos);
        weight = std::move(vt.weight);
    }

    return *this;
}

///////////////////////

template <typename TYPE, typename VAR>
GOptimize::NMSimplex<TYPE, VAR>::NMSimplex(const VAR &vec, TYPE thres, TYPE step)
{
    this->numParams = vec.size();
    this->sizeThres = thres;
    this->step = step;
    this->params = vec;

} // Constructor

template <typename TYPE, typename VAR>
GOptimize::NMSimplex<TYPE, VAR>::NMSimplex(VAR &&vec, TYPE thres, TYPE step)
{
    this->numParams = vec.size();
    this->sizeThres = thres;
    this->step = step;
    this->params = std::move(vec);

} // Constructor

template <typename TYPE, typename VAR>
bool GOptimize::NMSimplex<TYPE, VAR>::runSimplex(
    TYPE (*weight)(const VAR &, void *), void *ptr)
{
    // Some parameters needed by the method.Suggested values used
    TYPE alpha = 1.0, gamma = 2.0, rho = 0.5, sigma = 0.5;

    // Initializing Simplex
    VAR vec(this->params);
    simplex.emplace_back(vec, (*weight)(vec, ptr));

    for (uint32_t k = 0; k < numParams; k++)
    {
        vec(k) += step;
        if (k > 0)
            vec(k - 1) -= step;
        simplex.emplace_back(vec, (*weight)(vec, ptr));
    } // loop-vertex

    // Starting on the simplex method
    uint32_t counter = 0;
    while (counter++ < maxIterations)
    {
        // Determine simplex size
        TYPE size = GetSimplexSize();
        if (size < sizeThres)
        { // Saving final values to be returned
            params = simplex.at(0).pos;
            return true;
        }

        // Sorting from smallest to biggest vertex-weight
        sort(simplex.begin(), simplex.end(),
             [](const Vertex<TYPE, VAR> &a, const Vertex<TYPE, VAR> &b) {
                 return a.weight < b.weight;
             });

        // Calculate centroid
        VAR CM(numParams);
        CM.fill(TYPE(0));

        for (uint32_t k = 0; k < numParams; k++)
            CM += simplex.at(k).pos;

        CM /= TYPE(numParams); // Norma

        // Calculate the refletion point
        vec = CM + alpha * (CM - simplex.at(numParams).pos);
        Vertex<TYPE, VAR> RF(vec, (*weight)(vec, ptr));
        if (RF.weight < simplex.at(numParams - 1).weight &&
            RF.weight > simplex.at(0).weight)
        {
            simplex.at(numParams) = std::move(RF);
        }
        else if (RF.weight < simplex.at(0).weight)
        {

            // Calculate expansion point
            vec = CM + gamma * (RF.pos - CM);
            Vertex<TYPE, VAR> EX(vec, (*weight)(vec, ptr));
            if (EX.weight < RF.weight)
                simplex.at(numParams) = std::move(EX);
            else
                simplex.at(numParams) = std::move(RF);
        }
        else
        {

            // Calculate contraction point
            vec = CM + rho * (simplex.at(numParams).pos - CM);
            Vertex<TYPE, VAR> CT(vec, (*weight)(vec, ptr));
            if (CT.weight < simplex.at(numParams).weight)
                simplex.at(numParams) = std::move(CT);
            else
            {

                // Shrink
                Vertex<TYPE, VAR> vx0 = simplex.at(0);
                for (uint32_t k = 1; k <= numParams; k++)
                    simplex.at(k).pos = vx0.pos +
                                        sigma * (simplex.at(k).pos - vx0.pos);

            } // else-contraction
        }     // main else
    }         // while loop

    return false;
}

template <typename TYPE, typename VAR>
TYPE GOptimize::NMSimplex<TYPE, VAR>::GetSimplexSize()
{
    // Returns average longest distance between simplex vertices
    VAR big(numParams);
    big.fill(-TYPE(INFINITY));

    VAR small(numParams);
    small.fill(TYPE(INFINITY));

    for (uint32_t l = 0; l < numParams; l++)
        for (auto &sp : simplex)
        {
            TYPE val = sp.pos(l);
            big(l) = (val > big(l) ? val : big(l));
            small(l) = (val < small(l) ? val : small(l));
        }

    TYPE size = 0;
    for (uint32_t k = 0; k < numParams; k++)
        size += (big(k) - small(k));

    return size / TYPE(numParams + 1.0);
} // GetSimplexSize

///////////////// DISTRIBUTION ///////////////////////
template <typename T, typename VEC, typename VAR>
VAR GOptimize::sampleParameters(const VEC &params, const uint32_t sample_size,
                                T (*func)(const VEC &, void *), void *ptr)
{
    // This functions will sample the used parameters via MCMC.

    const uint32_t N = params.size(); // number of parameters

    VEC DA = params,
        da = params;

    VAR mcmc(sample_size, N);

    // Maximum likelihood
    T LIKE = -(*func)(DA, ptr);
    T S = 0.02; // proposal deviation

    std::mt19937 rng(time(NULL));
    std::uniform_real_distribution<T> unif(0.0, 1.0);
    std::normal_distribution<T> normal(0.0, S);

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
            T like = -(*func)(da, ptr);
            T ratio = exp(like - LIKE);

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
            S = std::max<T>(S, 0.001);
            normal = std::normal_distribution<T>(0.0, S);
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
        T like = -(*func)(da, ptr);
        T ratio = exp(like - LIKE);

        // Are the proposed values accepted?
        if (ratio > unif(rng))
        {
            DA = da;
            LIKE = like;
        }

        // Saving correct values to mcmc matrix
        for (uint32_t k = 0; k < N; k++)
            mcmc(mcs, k) = DA(k);

    } // loop-mcs

    return mcmc;

} // paramDistribution