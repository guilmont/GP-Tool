#include "goptimize.h"

GPT::GOptimize::Vertex::Vertex(const VecXd &pos, double weight) : pos(pos), weight(weight) {}
GPT::GOptimize::Vertex::Vertex(const Vertex &vt) : pos(vt.pos), weight(vt.weight) {}

GPT::GOptimize::Vertex::Vertex(Vertex &&vt) noexcept : pos(std::move(vt.pos)), weight(std::move(vt.weight)) {}

GPT::GOptimize::Vertex &GPT::GOptimize::Vertex::operator=(const Vertex &vt)
{
    if (&vt != this)
    {
        pos = vt.pos;
        weight = vt.weight;
    }

    return *this;
}

GPT::GOptimize::Vertex &GPT::GOptimize::Vertex::operator=(Vertex &&vt) noexcept
{
    if (&vt != this)
    {
        pos = std::move(vt.pos);
        weight = std::move(vt.weight);
    }

    return *this;
}

///////////////////////

GPT::GOptimize::NMSimplex::NMSimplex(const VecXd &vec, double thres, double step)
{
    this->numParams = vec.size();
    this->sizeThres = thres;
    this->step = step;
    this->params = vec;

} // Constructor

GPT::GOptimize::NMSimplex::NMSimplex(VecXd &&vec, double thres, double step)
{
    this->numParams = vec.size();
    this->sizeThres = thres;
    this->step = step;
    this->params = std::move(vec);

} // Constructor

double GPT::GOptimize::NMSimplex::GetSimplexSize()
{
    // Returns average longest distance between simplex vertices
    VecXd big(numParams);
    big.fill(-INFINITY);

    VecXd small(numParams);
    small.fill(INFINITY);

    for (uint64_t l = 0; l < numParams; l++)
        for (auto &sp : simplex)
        {
            double val = sp.pos(l);
            big(l) = (val > big(l) ? val : big(l));
            small(l) = (val < small(l) ? val : small(l));
        }

    double size = 0;
    for (uint64_t k = 0; k < numParams; k++)
        size += (big(k) - small(k));

    return size / double(numParams + 1.0);
} // GetSimplexSize
