#include "trajectory.h"

namespace GPT
{

    static void removeRow(MatXd &mat, uint64_t row)
    {
        uint64_t numRows = mat.rows() - 1;
        uint64_t numCols = mat.cols();

        if (row < numRows)
            mat.block(row, 0, numRows - row, numCols) = mat.block(row + 1, 0, numRows - row, numCols);

        mat.conservativeResize(numRows, numCols);
    }

    static Vec2d thresOutliers(VecXd vec)
    {
        std::sort(vec.data(), vec.data() + vec.size());

        uint64_t
            N =  static_cast<uint64_t>(0.5 * vec.size()),
            FQ = static_cast<uint64_t>(0.25 * vec.size()),
            TQ = static_cast<uint64_t>(0.75 * vec.size());

        double fifty = vec(TQ) - vec(FQ);
        return {vec(N) - 3.0 * fifty, vec(N) + 3.0 * fifty};
    } 

    static MatXd loadFromTextFile(const fs::path &path)
    {

        std::ifstream arq(path.string().c_str(), std::fstream::binary);
        if (arq.fail())
        {
            pout("ERROR (Trajectory::useCSV) ==> Cannot open:", path);
            return MatXd(0, 0);
        }

        std::string data;
        arq.seekg(0, arq.end);
        data.resize(arq.tellg());

        arq.seekg(0, arq.beg);
        arq.read(data.data(), data.size());

        arq.close();

        // so we know where we are
        uint64_t pos = 0;

        // Check if files line has a comment
        if (data.find('#') != std::string::npos)
            pos = data.find('\n', pos) + 1;

        // Reading data to matrix
        std::vector<std::vector<double>> mat;

        while (true)
        {
            // loading row if existent
            uint64_t brk = std::min(data.find('\n', pos), data.size());

            // in case there are multiple empty lines at the bottom
            // in case there are no empty lines at bottom
            if (pos == brk || pos >= data.size())
                break;

            // Parsing row
            std::vector<double> vec;
            while (true)
            {
                uint64_t loc = std::min(data.find(',', pos), brk);
                vec.emplace_back(stof(data.substr(pos, loc - pos)));

                pos = loc + 1;

                if (pos >= brk)
                    break;

            } // while-columns

            // appending row to matrix
            mat.emplace_back(std::move(vec));

        } // while-rows

        // Let's check is alls rows have same width
        uint64_t sum = 0;
        for (auto &vec : mat)
            sum += vec.size();

        if (sum == 0)
        {
            pout("ERROR (Trajectory::useCSV) ==> Input matrix is empty!! ::", path);
            return MatXd(0, 0);
        }

        const uint64_t
            NX = mat[0].size(),
            NY = mat.size();

        if (sum != mat.at(0).size() * mat.size())
        {
            pout("ERROR (Trajectory::useCSV) ==> Input txt have rows of different widths!! ::", path); 
            return MatXd(0, 0);
        }

        MatXd obj(NY, NX);
        for (uint64_t k = 0; k < NY; k++)
            for (uint64_t l = 0; l < NX; l++)
                obj(k, l) = mat[k][l];

        return obj;

    }

    ///////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////
    // PUBLIC FUNCTIONS

    Trajectory::Trajectory(Movie *mov) : movie(mov)
    {
        uint64_t SC = movie->getMetadata().SizeC;
        m_vTrack.resize(SC); // one track per channel
    }


    bool Trajectory::useICY(const fs::path &xmlTrack, uint64_t ch)
    {
        const Metadata &meta = movie->getMetadata();

        pugi::xml_document doc;
        if (!doc.load_file(xmlTrack.c_str()))
        {
            pout("ERROR (Trajectory::useICY_XML) ==> Couldn't open", xmlTrack.string());
            return false;
        }

        pugi::xml_node group = doc.child("root").child("trackgroup");

        m_vTrack[ch].path = xmlTrack;
        m_vTrack[ch].description = group.attribute("description").as_string();

        for (auto xtr : group.children("track"))
        {
            std::vector<Vec3d> vec;
            for (auto dtc : xtr.children("detection"))
                vec.emplace_back(dtc.attribute("t").as_double(),
                                 dtc.attribute("x").as_double(),
                                 dtc.attribute("y").as_double());

            uint64_t counter = 0;
            MatXd mat = MatXd::Zero(vec.size(), Track::NCOLS);
            for (Vec3d &txy : vec)
            {
                mat(counter, Track::FRAME) = txy[0];
                mat(counter, Track::POSX) = txy[1];
                mat(counter, Track::POSY) = txy[2];

                if (meta.hasPlanes())
                    mat(counter, Track::TIME) = meta.getPlane(ch, 0, int(txy[0])).DeltaT;
                else
                    mat(counter, Track::TIME) = txy[0] * meta.TimeIncrement;

                counter++;
            }

            m_vTrack[ch].traj.push_back(mat);
        } // loop-tracks

        return true;
    }

    bool Trajectory::useCSV(const fs::path &csvTrack, uint64_t ch)
    {

        // mov is not const because we might want to update metadata later
        const Metadata &meta = movie->getMetadata();

        // Importing all particles for channel
        MatXd particles = loadFromTextFile(csvTrack);

        if (particles.size() == 0)
            return false;

        Track_API track;
        track.path = csvTrack;

        // Splitting particles

        uint64_t 
            row = 0,
            partID = static_cast<uint64_t>(particles(0, 0)),
            nRows = particles.rows();

        for (uint64_t k = 0; k < nRows; k++)
            if (particles(k, 0) != partID || k == nRows - 1)
            {
                uint64_t N = k - row;
                if (k == nRows - 1)
                    N++;

                MatXd loc(N, uint64_t(Track::NCOLS));
                loc.fill(0.0);

                for (uint64_t r = 0; r < N; r++)
                {
                    uint64_t fr = static_cast<uint64_t>(particles(row + r, 1));

                    loc(r, Track::FRAME) = particles(row + r, 1);
                    loc(r, Track::POSX) = particles(row + r, 2);
                    loc(r, Track::POSY) = particles(row + r, 3);

                    if (meta.hasPlanes())
                        loc(r, Track::TIME) = meta.getPlane(ch, 0, fr).DeltaT;
                    else
                        loc(r, Track::TIME) = fr * meta.TimeIncrement;
                }

                track.traj.emplace_back(loc);

                partID = static_cast<uint64_t>(particles(k, 0));
                row = k;
            }

        m_vTrack[ch] = std::move(track);

        return true;
    } 

    void Trajectory::enhanceTracks(void)
    {
        running = true;
        progress = 0.0f;

        // calculating total number of tracks to enhance
        float dp = 0.0f;
        for (uint64_t ch = 0; ch < m_vTrack.size(); ch++)
            dp += m_vTrack[ch].traj.size();


        dp = 1.0f / dp;

        for (uint64_t ch = 0; ch < m_vTrack.size(); ch++)
        {
            Track_API &track = m_vTrack[ch];
            const uint64_t N = track.traj.size();

            if (N == 0)
                continue;

            for (uint64_t k = 0; k < N; k++)
            {
                enhanceTrajectory(ch, k);
                progress += dp;
            }

        }

        progress = 1.0f;
    } 



    ///////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////
    // PRIVATE FUNCTIONS

    void Trajectory::enhancePoint(uint64_t trackID, uint64_t trajID, uint64_t tid, uint64_t nThreads)
    {

        const uint64_t sRoi = 2 * spotSize + 1;

        MatXd& route = m_vTrack[trackID].traj[trajID];

        for (int64_t pt = tid; pt < route.rows(); pt += nThreads)
        {
            if (!running)
                return;

            // Get frame and coordinates
            int64_t 
                frame = static_cast<int64_t>(route(pt, Track::FRAME)),
                px = static_cast<int64_t>(route(pt, Track::POSX)),
                py = static_cast<int64_t>(route(pt, Track::POSY));

            // Loading a pointer to image
            const MatXd& img = movie->getImage(trackID, frame);

            // Let's check if all ROI pixels are within the image
            int64_t 
                px_o = px - spotSize, px_f = px_o + sRoi,
                py_o = py - spotSize, py_f = py_o + sRoi;

            bool check = true;
            check &= px_o >= 0;
            check &= px_f < img.cols();
            check &= py_o >= 0;
            check &= py_f < img.rows();

            if (!check)
            {
                // Cannot really work in this situation
                route(pt, 0) = -1;
                continue;
            }

            MatXd roi = img.block(py_o, px_o, sRoi, sRoi);

            // Correcting contrast
            double bot = roi.minCoeff();
            double top = roi.maxCoeff();

            roi.array() -= bot;
            roi.array() *= 255.0 / (top - bot);

            // Sending roi to Spot class for refinement
            Spot spot(roi);
            if (spot.successful()) // Everything went well
            {
                const SpotInfo& info = spot.getSpotInfo();

                // recentering posision
                route(pt, Track::POSX) = px + 0.5 + (info.mu(0) - 0.5 * sRoi);
                route(pt, Track::POSY) = py + 0.5 + (info.mu(1) - 0.5 * sRoi);

                route(pt, Track::ERRX) = info.error(0);
                route(pt, Track::ERRY) = info.error(1);
                route(pt, Track::SIZEX) = 3.0 * info.size(0);
                route(pt, Track::SIZEY) = 3.0 * info.size(1);
                route(pt, Track::BG) = info.signal(0);
                route(pt, Track::SIGNAL) = info.signal(1);
            }
            else
                route(pt, 0) = -1;

        } // loop-rows

    }

    void Trajectory::enhanceTrajectory(uint64_t trackID, uint64_t trajID)
    {

        // Updating localization and estimating error
        const uint64_t nThreads = std::thread::hardware_concurrency();
        std::vector<std::thread> vThr(nThreads);

        for (uint64_t tid = 0; tid < nThreads; tid++)
            vThr[tid] = std::thread(&Trajectory::enhancePoint, this, trackID, trajID, tid, nThreads);

        for (std::thread& thr : vThr)
            thr.join();

        // Removing rows that didn't converge during enhancement
        MatXd& route = m_vTrack[trackID].traj[trajID];

        int64_t nRows = route.rows();
        for (int64_t k = nRows - 1; k >= 0; k--)
            if (route(k, 0) < 0)
                removeRow(route, k);

        nRows = route.rows();
        if (nRows < 10)
            return;

        Vec2d
          thresSX = thresOutliers(route.col(Track::SIZEX)),   thresSY = thresOutliers(route.col(Track::SIZEY)),
          thresEX = thresOutliers(route.col(Track::ERRX)),    thresEY = thresOutliers(route.col(Track::ERRY)),
          thresSig = thresOutliers(route.col(Track::SIGNAL)), thresBG = thresOutliers(route.col(Track::BG));

        for (int64_t k = nRows - 1; k >= 0; k--)
        {
            bool check = true;
            check &= route(k, Track::SIZEX) > thresSX(0) && route(k, Track::SIZEX) < thresSX(1);
            check &= route(k, Track::SIZEY) > thresSY(0) && route(k, Track::SIZEY) < thresSY(1);

            check &= route(k, Track::ERRX) > thresEX(0) && route(k, Track::ERRX) < thresEX(1);
            check &= route(k, Track::ERRY) < thresEY(1) && route(k, Track::ERRY) > thresEY(0);

            check &= route(k, Track::BG) > thresBG(0) && route(k, Track::BG) < thresBG(1);
            check &= route(k, Track::SIGNAL) > thresSig(0) && route(k, Track::SIGNAL) < thresSig(1);

            if (!check)
                removeRow(route, k);
        }
    }

}