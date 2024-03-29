#include "align.h"

GPT::TransformData::TransformData(uint32_t width, uint32_t height) : size(width, height)
{
    translate = {0.0, 0.0};
    scale = {1.0, 1.0};
    rotate = {0.5 * width, 0.5 * height, 0.0f};

    trf = Mat3d::Identity();
    itrf = Mat3d::Identity();
} // constructor

void GPT::TransformData::update(void)
{
    // Creating transform matrix
    Mat3d A, B, C, D;

    A << scale(0), 0.0f, (1.0f - scale(0)) * 0.5f * size(0),
        0.0f, scale(1), (1.0f - scale(1)) * 0.5f * size(1),
        0.0f, 0.0f, 1.0f;

    B << 1.0, 0.0, translate(0) + rotate(0),
        0.0, 1.0, translate(1) + rotate(1),
        0.0, 0.0, 1.0;

    C << cos(rotate(2)), -sin(rotate(2)), 0.0,
        sin(rotate(2)), cos(rotate(2)), 0.0,
        0.0, 0.0, 1.0;

    D << 1.0, 0.0, -rotate(0),
        0.0, 1.0, -rotate(1),
        0.0, 0.0, 1.0;

    // Complete transformation
    trf = A * B * C * D;
    itrf = trf.inverse(); // just a facility

} // updateTransform

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static MatXd treatImage(MatXd img, int medianSize, double clipLimit, uint32_t tileSizeX, uint32_t tileSizeY)
{

    // Before anything else, let's correct contrast
    double top = img.maxCoeff(),
           bot = img.minCoeff();

    img.array() -= bot;
    img.array() *= 1.0 / (top - bot);

    //////////////////////////////////////////////////////////////
    // Creating look-up table

    const uint32_t
        nCols = uint32_t(img.cols()),
        nRows = uint32_t(img.rows()),
        tileArea = tileSizeX * tileSizeY,
        TX = uint32_t(std::ceil(nCols / double(tileSizeX))),
        TY = uint32_t(std::ceil(nRows / double(tileSizeY))),
        NT = TX * TY;

    clipLimit *= tileArea / 256.0;

    std::vector<uint32_t> cdfmin(NT, 0);
    MatXd lut = MatXd::Zero(256, NT);

    for (uint32_t k = 0; k < nRows; k++)
        for (uint32_t l = 0; l < nCols; l++)
        {
            uint32_t y = uint32_t(k / double(tileSizeX)),
                     x = uint32_t(l / double(tileSizeY));

            uint32_t tid = y * TX + x;

            uint32_t id = uint32_t(255.0 * img(k, l));
            lut(id, tid)++;
        }

    // Normalization
    for (uint32_t tid = 0; tid < NT; tid++)
    {
        // To avoid contrast differences at borders, let's clip the histogram
        while (true)
        {
            double extra = 0.0;
            for (uint32_t r = 0; r < 256; r++)
                if (lut(r, tid) > clipLimit)
                {
                    extra += lut(r, tid) - clipLimit;
                    lut(r, tid) = clipLimit;
                }

            if (extra < 0.00001)
                break;

            lut.col(tid).array() += extra / 256.0;

        } // while-true-clipLimit

        double norm = lut.col(tid).sum();
        lut.col(tid).array() /= norm;

        for (uint32_t k = 1; k < 256; k++)
            lut(k, tid) += lut(k - 1, tid);

        double bot = lut.col(tid).minCoeff();
        lut.col(tid).array() = (lut.col(tid).array() - bot) / (1.0 - bot);

    } // normalization

    /////////////////////////////////////////////////////////////////////
    // Applying clahe algorithm

    MatXd mat(nRows, nCols);
    mat.fill(0.0);

    for (uint32_t k = 0; k < nRows; k++)
        for (uint32_t l = 0; l < nCols; l++)
        {
            // Determining tile
            uint32_t x = l / tileSizeX, y = k / tileSizeY;

            double px = double(l) / tileSizeX - x,
                   py = double(k) / tileSizeY - y;

            int dx = round(px) == 1.0 ? 1 : -1,
                dy = round(py) == 1.0 ? 1 : -1;

            // boundary conditions
            if (y == 0 && dy == -1)
                dy = 0;

            if (y == (TY - 1) && dy == 1)
                dy = 0;

            if (x == 0 && dx == -1)
                dx = 0;

            if (x == (TX - 1) && dx == 1)
                dx = 0;

            // getting distance from tile's center
            px = px > 0.5 ? px - 0.5 : 0.5 - px;
            py = py > 0.5 ? py - 0.5 : 0.5 - py;

            uint32_t bin = uint32_t(255.0 * img(k, l)),
                     tid0 = (y + 0) * TX + (x + 0),
                     tid1 = (y + 0) * TX + (x + dx),
                     tid2 = (y + dy) * TX + (x + 0),
                     tid3 = (y + dy) * TX + (x + dx);

            double valx1 = (1.0 - px) * lut(bin, tid0) + px * (lut(bin, tid1));
            double valx2 = (1.0 - px) * lut(bin, tid2) + px * (lut(bin, tid3));

            mat(k, l) = (1.0 - py) * valx1 + py * valx2;
        }

    //////////////////////////////////////////
    // Let's apply some final treatment

    if (medianSize > 0)
    {
        int median_radius = int(0.5f * medianSize);
        for (uint32_t k = 0; k < nRows; k++)
            for (uint32_t l = 0; l < nCols; l++)
            {
                // Getting region
                uint32_t
                    xo = std::max<int>(l - median_radius, 0),
                    yo = std::max<int>(k - median_radius, 0),
                    xf = std::min<int>(l + median_radius + 1, nCols),
                    yf = std::min<int>(k + median_radius + 1, nRows);

                std::vector<double> vec;
                for (uint32_t y = yo; y < yf; y++)
                    for (uint32_t x = xo; x < xf; x++)
                        vec.push_back(mat(y, x));

                std::sort(vec.begin(), vec.end());

                uint32_t pos = uint32_t(0.5f * vec.size());
                img(k, l) = vec.at(pos);
            } // loop-median

        // And some autocontrast
        bot = img.minCoeff();
        top = img.maxCoeff();
        img.array() = (img.array() - bot) / (top - bot);

        return img;
    } // if-median-filter
    else
    {
        bot = mat.minCoeff();
        top = mat.maxCoeff();
        mat.array() = (mat.array() - bot) / (top - bot);

        return mat;
    }
}

/*******************************************************************/
/*******************************************************************/

GPT::Align::Align(uint32_t nFrames, const MatXd *im1, const MatXd *im2)
{

    vIm0.resize(nFrames);
    vIm1.resize(nFrames);
    RT = TransformData(uint32_t(im1[0].cols()), uint32_t(im1[0].rows()));

    // Setup transform properties
    auto parallel_image_treatment = [&](uint32_t tid, uint32_t nThr) -> void
    {
        for (uint32_t k = tid; k < nFrames; k += nThr)
        {
            vIm0[k] = (255.0 * treatImage(im1[k], 3, 5.0, 32, 32)).array().round().cast<uint8_t>();
            vIm1[k] = (255.0 * treatImage(im2[k], 3, 5.0, 32, 32)).array().round().cast<uint8_t>();
        }
    };

    std::vector<std::thread> vThr(std::thread::hardware_concurrency());
    for (uint32_t tid = 0; tid < vThr.size(); tid++)
        vThr[tid] = std::thread(parallel_image_treatment, tid, uint32_t(vThr.size()));

    for (auto &thr : vThr)
        thr.join();

} // constructor

void GPT::Align::calcEnergy(const uint32_t id, const uint32_t nThr)
{
    global_energy[id] = 0.0;

    const uint32_t
        width = uint32_t(vIm0[0].cols()),
        height = uint32_t(vIm0[0].rows());

    for (uint32_t fr = id; fr < vIm0.size(); fr += nThr)
    {
        // Evaluating transformed img2
        for (uint32_t y = 0; y < height; y++)
            for (uint32_t x = 0; x < width; x++)
            {
                int32_t i = static_cast<int32_t>(itrf(0, 0) * (x + 0.5) + itrf(0, 1) * (y + 0.5) + itrf(0, 2) + 0.5);
                int32_t j = static_cast<int32_t>(itrf(1, 0) * (x + 0.5) + itrf(1, 1) * (y + 0.5) + itrf(1, 2) + 0.5);

                double dr = double(vIm0[fr](y, x));
                if (i >= 0 && i < int(width) && j >= 0 && j < int(height))
                    dr -= double(vIm1[fr](j, i));

                global_energy[id] += dr * dr;
            }

    } // thread-loop

} // calcEnergy

double GPT::Align::weightTransRot(const VecXd &p)
{
    double 
        dx = p[0], dy = p[1],
        cx = p[2], cy = p[3], angle = p[4];

    // Transformation matrices
    Mat3d A, B, C;

    A << 1.0, 0.0, dx + cx,
         0.0, 1.0, dy + cy,
         0.0, 0.0, 1.0;

    B << cos(angle), -sin(angle), 0.0,
         sin(angle), cos(angle), 0.0,
         0.0, 0.0, 1.0;

    C << 1.0, 0.0, -cx,
         0.0, 1.0, -cy,
         0.0, 0.0, 1.0;

    // Complete transformation
    Mat3d trf = A * B * C;
    itrf = trf.inverse();

    // Splitting log-likelihood calculationg to threads
    const uint32_t nThr = std::thread::hardware_concurrency();

    global_energy.resize(nThr);
    std::vector<std::thread> vThr(nThr);

    for (uint32_t tid = 0; tid < nThr; tid++)
        vThr[tid] = std::thread(&Align::calcEnergy, this, tid, nThr);

    for (auto &thr : vThr)
        thr.join();

    double energy = 0.0;
    for (double val : global_energy)
        energy += val;

    // We want to minimize, therefore the negative of the maximize
    return 0.5 * RT.size(0) * RT.size(1) * log(energy);

} // weightFunc

double GPT::Align::weightScale(const VecXd &p)
{
    double 
        sx = p[0], sy = p[1],
        width = static_cast<double>(vIm0[0].cols()), 
        height = static_cast<double>(vIm0[0].rows());

    // Transformation matrices
    Mat3d A;
    A << sx, 0.0, 0.5 * width * (1.0 - sx),
         0.0, sy, 0.5 * height * (1.0 - sy),
         0.0, 0.0, 1.0;

    // Inverse of transfomation for mapping
    Mat3d trf = A * RT.trf;
    itrf = trf.inverse();

    // Splitting log-likelihood calculationg to threads
    const uint32_t nThr = std::thread::hardware_concurrency();

    global_energy.resize(nThr);
    std::vector<std::thread> vThr(nThr);

    for (uint32_t tid = 0; tid < nThr; tid++)
        vThr[tid] = std::thread(&Align::calcEnergy, this, tid, nThr);

    for (auto &thr : vThr)
        thr.join();

    double energy = 0.0;
    for (double val : global_energy)
        energy += val;

    // We want to minimize, therefore the negative of the maximize
    return 0.5 * width * height * log(energy);
}; // weightFunc

bool GPT::Align::alignCameras(void)
{
    VecXd vec(5);
    vec << RT.translate(0), RT.translate(1), RT.rotate(0), RT.rotate(1), RT.rotate(2);

    nms = std::make_unique<GOptimize::NMSimplex>(vec, 1e-8, 15.0);
    if (!nms->runSimplex(&Align::weightTransRot, this))
        return false;

    // Update TransformData
    vec = nms->getResults();
    RT.translate = {vec(0), vec(1)};
    RT.rotate = {vec(2), vec(3), vec(4)};
    RT.update();

    return true;
}

bool GPT::Align::correctAberrations(void)
{
    VecXd vec(2);
    vec << RT.scale(0), RT.scale(1);

    nms = std::make_unique<GOptimize::NMSimplex>(vec, 1e-8, 0.1);
    if (!nms->runSimplex(&Align::weightScale, this))
        return false;

    // Saving to main vector
    vec = nms->getResults();
    RT.scale = {vec(0), vec(1)};
    RT.update();

    return true;
}
