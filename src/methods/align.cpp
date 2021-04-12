#include "align.h"

TransformData::TransformData(uint32_t width, uint32_t height) : size(width, height)
{
    translate = {0.0f, 0.0f};
    scale = {1.0f, 1.0f};
    rotate = {0.5f, 0.5f, 0.0f};

    trf = glm::mat3(1.0f);
    itrf = glm::mat3(1.0f);
} // constructor

void TransformData::update(void)
{
    // Creating transform matrix

    glm::mat3 A(scale.x, 0.0f, (1.0f - scale.x) * 0.5f,
                0.0f, scale.y, (1.0f - scale.y) * 0.5f,
                0.0f, 0.0f, 1.0f);

    glm::mat3 B(1.0, 0.0, translate.x - rotate.x,
                0.0, 1.0, translate.y - rotate.y,
                0.0, 0.0, 1.0);

    glm::mat3 C(cos(rotate.z), sin(rotate.z), 0.0,
                -sin(rotate.z), cos(rotate.z), 0.0,
                0.0, 0.0, 1.0);

    glm::mat3 D(1.0, 0.0, rotate.x,
                0.0, 1.0, rotate.y,
                0.0, 0.0, 1.0);

    // Complete transformation
    trf = A * B * C * D;
    itrf = glm::inverse(trf); // just a facility

} // updateTransform

// static MatrixXd treatImage(MatrixXd img, int medianSize, double clipLimit,
//                            uint32_t tileSizeX, uint32_t tileSizeY, Mailbox *mail)
// {

//     if (medianSize % 2 == 0)
//     {
//         mail->createMessage<MSG_Error>("(treatImage): "
//                                        "Median size should be off number!!");
//         return MatrixXd(0, 0);
//     }

//     // Before anything else, let's correct contrast
//     double top = img.maxCoeff(),
//            bot = img.minCoeff();

//     img.array() -= bot;
//     img.array() *= 1.0 / (top - bot);

//     //////////////////////////////////////////////////////////////
//     // Creating look-up table

//     const uint32_t
//         nCols = img.cols(),
//         nRows = img.rows(),
//         tileArea = tileSizeX * tileSizeY,
//         TX = ceil(nCols / double(tileSizeX)),
//         TY = ceil(nRows / double(tileSizeY)),
//         NT = TX * TY;

//     clipLimit *= tileArea / 256.0;

//     std::vector<uint32_t> cdfmin(NT, 0);
//     MatrixXd lut = MatrixXd::Zero(256, NT);

//     for (uint32_t k = 0; k < nRows; k++)
//         for (uint32_t l = 0; l < nCols; l++)
//         {
//             uint32_t y = k / double(tileSizeX),
//                      x = l / double(tileSizeY);

//             uint32_t tid = y * TX + x;

//             uint32_t id = 255.0 * img(k, l);
//             lut(id, tid)++;
//         }

//     // Normalization
//     for (uint32_t tid = 0; tid < NT; tid++)
//     {
//         // To avoid contrast differences at borders, let's clip the histogram
//         while (true)
//         {
//             double extra = 0.0;
//             for (uint32_t r = 0; r < 256; r++)
//                 if (lut(r, tid) > clipLimit)
//                 {
//                     extra += lut(r, tid) - clipLimit;
//                     lut(r, tid) = clipLimit;
//                 }

//             if (extra < 0.00001)
//                 break;

//             lut.col(tid).array() += extra / 256.0;

//         } // while-true-clipLimit

//         double norm = lut.col(tid).sum();
//         lut.col(tid).array() /= norm;

//         for (uint32_t k = 1; k < 256; k++)
//             lut(k, tid) += lut(k - 1, tid);

//         double bot = lut.col(tid).minCoeff();
//         lut.col(tid).array() = (lut.col(tid).array() - bot) / (1.0 - bot);

//     } // normalization

//     /////////////////////////////////////////////////////////////////////
//     // Applying clahe algorithm

//     MatrixXd mat(nRows, nCols);
//     mat.fill(0.0);

//     for (uint32_t k = 0; k < nRows; k++)
//         for (uint32_t l = 0; l < nCols; l++)
//         {
//             // Determining tile
//             uint32_t x = l / tileSizeX, y = k / tileSizeY;

//             double px = double(l) / tileSizeX - x,
//                    py = double(k) / tileSizeY - y;

//             int dx = round(px) == 1.0 ? 1 : -1,
//                 dy = round(py) == 1.0 ? 1 : -1;

//             // boundary conditions
//             if (y == 0 && dy == -1)
//                 dy = 0;

//             if (y == (TY - 1) && dy == 1)
//                 dy = 0;

//             if (x == 0 && dx == -1)
//                 dx = 0;

//             if (x == (TX - 1) && dx == 1)
//                 dx = 0;

//             // getting distance from tile's center
//             px = px > 0.5 ? px - 0.5 : 0.5 - px;
//             py = py > 0.5 ? py - 0.5 : 0.5 - py;

//             uint32_t bin = 255.0 * img(k, l),
//                      tid0 = (y + 0) * TX + (x + 0),
//                      tid1 = (y + 0) * TX + (x + dx),
//                      tid2 = (y + dy) * TX + (x + 0),
//                      tid3 = (y + dy) * TX + (x + dx);

//             double valx1 = (1.0 - px) * lut(bin, tid0) + px * (lut(bin, tid1));
//             double valx2 = (1.0 - px) * lut(bin, tid2) + px * (lut(bin, tid3));

//             mat(k, l) = (1.0 - py) * valx1 + py * valx2;
//         }

//     //////////////////////////////////////////
//     // Let's apply some final treatment

//     if (medianSize > 0)
//     {
//         int median_radius = int(0.5f * medianSize);
//         for (uint32_t k = 0; k < nRows; k++)
//             for (uint32_t l = 0; l < nCols; l++)
//             {
//                 // Getting region
//                 uint32_t
//                     xo = std::max<int>(l - median_radius, 0),
//                     yo = std::max<int>(k - median_radius, 0),
//                     xf = std::min<int>(l + median_radius + 1, nCols),
//                     yf = std::min<int>(k + median_radius + 1, nRows);

//                 std::vector<double> vec;
//                 for (uint32_t y = yo; y < yf; y++)
//                     for (uint32_t x = xo; x < xf; x++)
//                         vec.push_back(mat(y, x));

//                 std::sort(vec.begin(), vec.end());

//                 uint32_t pos = 0.5f * vec.size();
//                 img(k, l) = vec.at(pos);
//             } // loop-median

//         // And some autocontrast
//         bot = img.minCoeff();
//         top = img.maxCoeff();
//         img.array() = (img.array() - bot) / (top - bot);

//         return img;
//     } // if-median-filter
//     else
//     {
//         bot = mat.minCoeff();
//         top = mat.maxCoeff();
//         mat.array() = (mat.array() - bot) / (top - bot);

//         return mat;
//     }
// }

// /*******************************************************************/
// /*******************************************************************/

// Align::Align(Mailbox *mailbox, uint32_t nChannels) : mail(mailbox)
// {
//     // Initializing TranformData values
//     vecRT.resize(nChannels - 1);
//     for (uint32_t ch = 0; ch < nChannels - 1; ch++)
//     {
//         TransformData rt;

//         vecRT.emplace_back(rt);

//     } // loop-channels

// } // constructor

// void Align::setImageData(const uint32_t nFrames, MatrixXd *im1, MatrixXd *im2)
// {
//     using Frame = std::tuple<MatrixXd, MatrixXd, Mailbox *>;

//     // Initialize input data
//     numFrames = nFrames < 20 ? nFrames : 0.05f * nFrames;
//     width = im1[0].cols();
//     height = im1[0].rows();

//     // Setup transform properties
//     TransformData &RT = vecRT[chAligning];
//     RT.reset(width, height);

//     std::vector<Frame> vFrame;
//     for (uint32_t k = 0; k < numFrames; k++)
//         vFrame.emplace_back(im1[k], im2[k], mail);

//     auto parallel_image_treatment = [](const uint32_t id, void *ptr) -> void {
//         auto &[ch0, ch1, mail] = reinterpret_cast<Frame *>(ptr)[id];
//         ch0 = (255.0 * treatImage(ch0, 3, 5.0, 64, 64, mail)).array().round();
//         ch1 = (255.0 * treatImage(ch1, 3, 5.0, 64, 64, mail)).array().round();
//     };

//     MSG_Progress *msg = mail->createMessage<MSG_Progress>("Treating images");

//     Threadpool pool(msg);
//     pool.run(parallel_image_treatment, vFrame.size(), vFrame.data());

//     vImg0.clear();
//     vImg1.clear();
//     for (auto &[ch0, ch1, ptr] : vFrame)
//     {
//         vImg0.emplace_back(ch0.cast<uint8_t>());
//         vImg1.emplace_back(ch1.cast<uint8_t>());
//     } // loop-frames

// } // setImageData

// void Align::calcEnergy(const uint32_t id)
// {
//     global_energy.at(id) = 0.0;

//     Threadpool pool;
//     const uint32_t nThr = pool.getNumThreads();

//     for (uint32_t fr = id; fr < numFrames; fr += nThr)
//     {
//         // Evaluating transformed img2
//         for (uint32_t x = 0; x < width; x++)
//             for (uint32_t y = 0; y < height; y++)
//             {
//                 int i = itrf(0, 0) * (x + 0.5) + itrf(0, 1) * (y + 0.5) + itrf(0, 2),
//                     j = itrf(1, 0) * (x + 0.5) + itrf(1, 1) * (y + 0.5) + itrf(1, 2);

//                 double dr = double(vImg0[fr](y, x));
//                 if (i >= 0 && i < int(width) && j >= 0 && j < int(height))
//                     dr -= double(vImg1[fr](j, i));

//                 global_energy.at(id) += dr * dr;
//             }

//     } // thread-loop

// } // calcEnergy

// double Align::weightTransRot(const VectorXd &p)
// {
//     double dx = p[0], dy = p[1],
//            cx = p[2], cy = p[3], angle = p[4];

//     // Transformation matrices
//     MatrixXd A(3, 3), B(3, 3), C(3, 3);
//     A << 1.0f, 0.0f, dx + cx,
//         0.0f, 1.0f, dy + cy,
//         0.0f, 0.0f, 1.0f;

//     B << cos(angle), sin(angle), 0.0f,
//         -sin(angle), cos(angle), 0.0f,
//         0.0f, 0.0f, 1.0f;

//     C << 1.0f, 0.0f, -cx,
//         0.0f, 1.0f, -cy,
//         0.0f, 0.0f, 1.0f;

//     // Complete transformation
//     MatrixXd trf = A * B * C;
//     itrf = inverseMatrix3x3(trf);

//     if (itrf.size() == 0)
//         mail->createMessage<MSG_Warning>("(Align::weightTransRot): "
//                                          "Transformation cannot be inverted!");

//     // Splitting log-likelihood calculationg to threads
//     Threadpool pool;
//     const uint32_t nThr = pool.getNumThreads();
//     global_energy.resize(nThr);
//     pool.run(&transferEnergy, nThr, this);

//     double energy = 0.0;
//     for (double val : global_energy)
//         energy += val;

//     // We want to minimize, therefore the negative of the maximize
//     return 0.5 * width * height * log(energy);
// }; // weightFunc

// double Align::weightScale(const VectorXd &p)
// {
//     double sx = p[0], sy = p[1];

//     // Transformation matrices
//     MatrixXd A(3, 3);
//     A << sx, 0.0f, 0.5f * width * (1.0f - sx),
//         0.0f, sy, 0.5f * height * (1.0f - sy),
//         0.0f, 0.0f, 1.0f;

//     // Inverse of transfomation for mapping
//     MatrixXd trf = A * vecRT[chAligning].trf;
//     itrf = inverseMatrix3x3(trf);

//     // Splitting log-likelihood calculationg to threads
//     Threadpool pool;
//     const uint32_t nThr = pool.getNumThreads();
//     global_energy.resize(nThr);
//     pool.run(&transferEnergy, nThr, this);

//     double energy = 0.0f;
//     for (double val : global_energy)
//         energy += val;

//     // We want to minimize, therefore the negative of the maximize
//     return 0.5f * width * height * log(energy);
// }; // weightFunc

// bool Align::alignCameras(void)
// {
//     if (vImg0.empty() || vImg1.empty())
//     {
//         mail->createMessage<MSG_Error>("(Align::alignCameras): "
//                                        "Image data not set properly!");
//         return false;
//     }

//     // Optimizing translation and rotation
//     TransformData &RT = vecRT[chAligning];

//     VectorXd vec(5);
//     vec << RT.dx, RT.dy, RT.cx, RT.cy, RT.angle;

//     GOptimize::NMSimplex<double, VectorXd> spx(vec, 1e-8, 15.0);

//     if (!spx.runSimplex(&transferWeight, this))
//     {
//         mail->createMessage<MSG_Warning>("(Align::alignCameras): "
//                                          "Simplex didn't converge!!");

//         return false;
//     }

//     // Update TransformData
//     vec = spx.getResults();
//     RT.dx = vec(0);
//     RT.dy = vec(1);
//     RT.cx = vec(2);
//     RT.cy = vec(3);
//     RT.angle = vec(4);

//     // Updating final matrix
//     RT.updateTransform();

//     return true;

// } // alignCameras

// bool Align::correctAberrations(void)
// {
//     if (vImg0.empty() || vImg1.empty())
//     {
//         mail->createMessage<MSG_Error>("(Align::correctAberrations): "
//                                        "Image data not set properly!");
//         return false;
//     }

//     TransformData &RT = vecRT[chAligning];
//     VectorXd vec(2);
//     vec << RT.sx, RT.sy;

//     GOptimize::NMSimplex<double, VectorXd> spx(vec, 1e-8, 0.1);
//     if (!spx.runSimplex(&transferScale, this))
//     {
//         mail->createMessage<MSG_Warning>("(Align::correctAberrations): "
//                                          "Simplex didn't converge!!");

//         return false;
//     }

//     // Saving to main vector
//     vec = spx.getResults();
//     RT.sx = vec(0);
//     RT.sy = vec(1);

//     // Updating final transform
//     RT.updateTransform();

//     return true;

// } // correctAberrations
