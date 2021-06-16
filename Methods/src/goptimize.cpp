#include "goptimize.h"

GOptimize::Vertex::Vertex(const VecXd &pos, double weight) : pos(pos), weight(weight) {}
GOptimize::Vertex::Vertex(const Vertex &vt) : pos(vt.pos), weight(vt.weight) {}

GOptimize::Vertex::Vertex(Vertex &&vt) : pos(std::move(vt.pos)), weight(std::move(vt.weight)) {}

GOptimize::Vertex &GOptimize::Vertex::operator=(const Vertex &vt)
{
    if (&vt != this)
    {
        pos = vt.pos;
        weight = vt.weight;
    }

    return *this;
}

GOptimize::Vertex &GOptimize::Vertex::operator=(Vertex &&vt)
{
    if (&vt != this)
    {
        pos = std::move(vt.pos);
        weight = std::move(vt.weight);
    }

    return *this;
}

///////////////////////

GOptimize::NMSimplex::NMSimplex(const VecXd &vec, double thres, double step)
{
    this->numParams = uint32_t(vec.size());
    this->sizeThres = thres;
    this->step = step;
    this->params = vec;

} // Constructor

GOptimize::NMSimplex::NMSimplex(VecXd &&vec, double thres, double step)
{
    this->numParams = uint32_t(vec.size());
    this->sizeThres = thres;
    this->step = step;
    this->params = std::move(vec);

} // Constructor

double GOptimize::NMSimplex::GetSimplexSize()
{
    // Returns average longest distance between simplex vertices
    VecXd big(numParams);
    big.fill(-INFINITY);

    VecXd small(numParams);
    small.fill(INFINITY);

    for (uint32_t l = 0; l < numParams; l++)
        for (auto &sp : simplex)
        {
            double val = sp.pos(l);
            big(l) = (val > big(l) ? val : big(l));
            small(l) = (val < small(l) ? val : small(l));
        }

    double size = 0;
    for (uint32_t k = 0; k < numParams; k++)
        size += (big(k) - small(k));

    return size / double(numParams + 1.0);
} // GetSimplexSize
