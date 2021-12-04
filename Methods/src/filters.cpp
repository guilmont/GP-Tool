#include "filters.h"

#include <Eigen/SVD>

namespace GPT::Filter
{
    void Contrast::apply(MatXd& img)
    {
        if (low < 0)
            low = img.minCoeff();

        if (high < 0)
            high = img.maxCoeff();

        img.array() = (img.array() - low) / (high - low); // Images are always in between 0 and 1

        // To make sure
        for (int64_t k = 0; k < img.size(); k++)
        {
            double& val = img.data()[k];
            val = std::max<double>(0, val);
            val = std::min<double>(1, val);
        }
    }
    
    /**************************************************************************/
    /**************************************************************************/

	void Median::apply(MatXd& img)
	{
        MatXd mat(img);

        int64_t
            radiusX = static_cast<int64_t>(0.5 * double(sizeX)),
            radiusY = static_cast<int64_t>(0.5 * double(sizeY));

        for (int64_t k = 0; k < mat.rows(); k++)
            for (int64_t l = 0; l < mat.cols(); l++)
            {
                // Getting region
                int64_t
                    xo = std::max<int64_t>(l - radiusX, 0),
                    yo = std::max<int64_t>(k - radiusY, 0),
                    xf = std::min<int64_t>(l + radiusX + 1, mat.cols()),
                    yf = std::min<int64_t>(k + radiusY + 1, mat.rows());

                int64_t size = (yf - yo) * (xf - xo);

                std::vector<double> vec;
                vec.reserve(size);

                for (int64_t y = yo; y < yf; y++)
                    for (int64_t x = xo; x < xf; x++)
                        vec.push_back(mat(y, x));

                std::sort(vec.begin(), vec.end());

                int64_t pos = static_cast<int64_t>(0.5 * double(vec.size()));
                img(k, l) = vec.at(pos);
            }
	}

    /**************************************************************************/
    /**************************************************************************/

	void CLAHE::apply(MatXd& img)
	{
        const int64_t
            nCols = img.cols(),
            nRows = img.rows(),
            tileArea = tileSizeX * tileSizeY,
            TX = static_cast<int64_t>(std::ceil(double(nCols) / double(tileSizeX))),
            TY = static_cast<int64_t>(std::ceil(double(nRows) / double(tileSizeY))),
            NT = TX * TY;

        double clipValue = clipLimit * tileArea / 256.0;

        MatXd lut = MatXd::Zero(256, NT);

        for (int64_t k = 0; k < nRows; k++)
            for (int64_t l = 0; l < nCols; l++)
            {
                int64_t
                    y = static_cast<int64_t>(k / double(tileSizeX)),
                    x = static_cast<int64_t>(l / double(tileSizeY)),
                    tid = y * TX + x;

                int64_t id = static_cast<int64_t>(255.0 * img(k, l));
                lut(id, tid)++;
            }

        // Normalization all look up tables
        for (int64_t tid = 0; tid < NT; tid++)
        {
            // To avoid contrast differences at borders, let's clip the histogram
            while (true)
            {
                double extra = 0.0;
                for (int64_t r = 0; r < 256; r++)
                    if (lut(r, tid) > clipValue)
                    {
                        extra += lut(r, tid) - clipValue;
                        lut(r, tid) = clipValue;
                    }

                if (extra < 0.00001)
                    break;

                lut.col(tid).array() += extra / 256.0;
            } 

            double norm = lut.col(tid).sum();
            lut.col(tid).array() /= norm;

            for (int64_t k = 1; k < 256; k++)
                lut(k, tid) += lut(k - 1, tid);

            double bot = lut.col(tid).minCoeff();
            lut.col(tid).array() = (lut.col(tid).array() - bot) / (1.0 - bot);
        }

        // Applying clahe algorithm
        for (int64_t k = 0; k < nRows; k++)
            for (int64_t l = 0; l < nCols; l++)
            {
                // Determining tile
                int64_t x = l / tileSizeX, y = k / tileSizeY;

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

                int64_t
                    bin = static_cast<int64_t>(255.0 * img(k, l)),
                    tid0 = (y + 0) * TX + (x + 0),
                    tid1 = (y + 0) * TX + (x + dx),
                    tid2 = (y + dy) * TX + (x + 0),
                    tid3 = (y + dy) * TX + (x + dx);

                double valx1 = (1.0 - px) * lut(bin, tid0) + px * (lut(bin, tid1));
                double valx2 = (1.0 - px) * lut(bin, tid2) + px * (lut(bin, tid3));

                img(k, l) = (1.0 - py) * valx1 + py * valx2;
            }
	}

    /**************************************************************************/
    /**************************************************************************/

    void SVD::importImages(const std::vector<MatXd>& vec)
    {
        denoised.resize(vec.size());
        vImages.resize(vec.size());
        std::copy(vec.begin(), vec.end(), vImages.begin());
    }

	void SVD::run(bool &trigger)
	{
        int64_t
            maxFrames = vImages.size(),
            width = vImages[0].cols(),
            height = vImages[0].rows(),
            rows = width * height;

        // We will take the average over every rank approximated image for different slices
        std::vector<float> counter(maxFrames, 0);

        for (int64_t fr = 0; fr < maxFrames; fr++)
            denoised[fr] = MatXd::Zero(height, width);

        MatXd mat(rows, slice);
        for (int64_t fr = 0; fr <= maxFrames - slice; fr++)
        {
            for (int64_t k = 0; k < slice; k++)
                mat.col(k) = vImages[fr + k].reshaped();

            Eigen::BDCSVD<MatXd> svd(mat, Eigen::ComputeThinU | Eigen::ComputeThinV);
            MatXd U = svd.matrixU();
            MatXd V = svd.matrixV().transpose();
            const VecXd& S = svd.singularValues();


            mat = S(0) * U.col(0) * V.row(0);

            for (int64_t k = 1; k < rank; k++)
                mat += S(k) * U.col(k) * V.row(k);


            for (int64_t k = 0; k < slice; k++)
            {
                denoised[fr + k] += mat.col(k).reshaped();
                counter[fr + k]++;
            }

            // In case we want to stop this function from outside
            if (trigger)
                return;
        }

        for (int64_t fr = 0; fr < maxFrames; fr++)
            denoised[fr] /= counter[fr];

    }

    const MatXd& SVD::getImage(int64_t frame) { return denoised[frame]; }
    
    void SVD::updateImages(std::vector<MatXd>& vec)
    {
        assert(vec.size() == denoised.size());
        std::copy(denoised.begin(), denoised.end(), vec.begin());
    }
}