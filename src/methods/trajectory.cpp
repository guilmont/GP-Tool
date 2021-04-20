#include "methods/trajectory.h"

static void removeRow(MatXd &matrix, uint32_t rowToRemove)
{
    uint32_t numRows = uint32_t(matrix.rows()) - 1;
    uint32_t numCols = uint32_t(matrix.cols());

    if (rowToRemove < numRows)
        matrix.block(rowToRemove, 0, numRows - rowToRemove, numCols) =
            matrix.block(rowToRemove + 1, 0, numRows - rowToRemove, numCols);

    matrix.conservativeResize(numRows, numCols);
}

static glm::dvec2 thresOutliers(VecXd vec)
{
    std::sort(vec.data(), vec.data() + vec.size());

    uint32_t N = uint32_t(0.5f * vec.size()),
             FQ = uint32_t(0.25f * vec.size()),
             TQ = uint32_t(0.75f * vec.size());

    double fifty = vec(TQ) - vec(FQ);
    return {vec(N) - 2.0 * fifty, vec(N) + 2.0 * fifty};
} // thresOutliers

static MatXd loadFromTextFile(const std::string &path, char delimiter,
                              uint32_t skip_rows, uint32_t skip_cols, Mailbox *mbox)
{

    std::ifstream arq(path, std::fstream::binary);
    if (arq.fail())
    {
        if (mbox)
            mbox->create<Message::Error>("(Trajectory::useCSV): Cannot open " + path);
        else
            std::cerr << "ERROR (Trajectory::useCSV) ==> Cannot open " << path << std::endl;

        return MatXd(0, 0);
    }

    std::string data;
    arq.seekg(0, arq.end);
    data.resize(size_t(arq.tellg()));

    arq.seekg(0, arq.beg);
    arq.read(data.data(), data.size());

    arq.close();

    // so we know where we are
    size_t pos = 0;

    // skipping rows demanded
    for (size_t k = 0; k < skip_rows; k++)
    {
        pos = data.find('\n', pos) + 1;
        if (pos == std::string::npos)
        {

            std::string msg = "(Trajectory::useCSV): More skip_rows than number of rows!! " + path;

            if (mbox)
                mbox->create<Message::Error>(msg);
            else
                std::cerr << "ERROR " << msg << std::endl;

            return MatXd(0, 0);
        }
    } // loop-skip-rows

    // Reading data to matrix
    std::vector<std::vector<double>> mat;

    while (true)
    {
        // loading row if existent
        size_t brk = std::min(data.find('\n', pos), data.size());

        // in case there are multiple empty lines at the bottom
        // in case there are no empty lines at bottom
        if (pos == brk || pos >= data.size())
            break;

        // Parsing row
        size_t ct = 0;
        std::vector<double> vec;
        while (true)
        {
            size_t loc = std::min(data.find(delimiter, pos), brk);

            if (++ct > skip_cols)
                vec.emplace_back(stof(data.substr(pos, loc - pos)));

            pos = loc + 1;

            if (pos >= brk)
                break;

        } // while-columns

        // appending row to matrix
        mat.emplace_back(std::move(vec));

    } // while-rows

    // Let's check is alls rows have same width
    size_t sum = 0;
    for (auto &vec : mat)
        sum += vec.size();

    if (sum == 0)
    {
        std::string msg = "(Trajectory::useCSV): Input matrix is empty!! " + path;

        if (mbox)
            mbox->create<Message::Error>(msg);
        else
            std::cerr << "ERROR " << msg << std::endl;

        return MatXd(0, 0);
    }

    const size_t
        NX = mat[0].size(),
        NY = mat.size();

    if (sum != mat.at(0).size() * mat.size())
    {
        std::string msg = "(Trajectory::useCSV): Input txt have rows of different widths!! " + path;

        if (mbox)
            mbox->create<Message::Error>(msg);
        else
            std::cerr << "ERROR " << msg << std::endl;

        return MatXd(0, 0);
    }

    MatXd obj(NY, NX);
    for (uint32_t k = 0; k < NY; k++)
        for (uint32_t l = 0; l < NX; l++)
            obj(k, l) = mat[k][l];

    return obj;

} // readTextFile

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS

void Trajectory::enhancePoint(uint32_t trackID, uint32_t trajID, uint32_t tid)
{

    const uint32_t sRoi = 2 * spotSize + 1,
                   nThreads = std::thread::hardware_concurrency();

    MatXd &route = m_vTrack[trackID].traj[trajID];

    for (uint32_t pt = tid; pt < uint32_t(route.rows()); pt += nThreads)
    {

        // Get frame and coordinates
        int frame = int(route(pt, Track::FRAME)),
            px = int(route(pt, Track::POSX)),
            py = int(route(pt, Track::POSY));

        // Loading a pointer to image
        const MatXd &img = movie->getImage(trackID, frame);

        // Let's check if all ROI pixels are within the image
        int px_o = px - spotSize, px_f = px_o + sRoi,
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

        MatXd roi = img.block(py - spotSize, px - spotSize, sRoi, sRoi);

        // Correcting contrast
        double bot = roi.minCoeff();
        double top = roi.maxCoeff();

        roi.array() -= bot;
        roi.array() *= 255.0f / (top - bot);

        // Sending roi to Spot class for refinement
        Spot spot(roi);
        if (spot.successful()) // Everything went well
        {
            const SpotInfo &info = spot.getSpotInfo();

            // recentering posision
            route(pt, Track::POSX) = px + 0.5 + (info.mu.x - 0.5 * sRoi);
            route(pt, Track::POSY) = py + 0.5 + (info.mu.y - 0.5 * sRoi);

            route(pt, Track::ERRX) = info.error.x;
            route(pt, Track::ERRY) = info.error.y;
            route(pt, Track::SIZEX) = 3.0 * info.size.x;
            route(pt, Track::SIZEY) = 3.0 * info.size.y;
            route(pt, Track::BG) = info.signal.x;
            route(pt, Track::SIGNAL) = info.signal.y;
        }
        else
            route(pt, 0) = -1;

    } // loop-rows

} // EnhanceRoute

void Trajectory::enhanceTrajectory(uint32_t trackID, uint32_t trajID)
{

    auto condPush = [](std::vector<uint32_t> &vec, uint32_t val) -> void {
        for (uint32_t k : vec)
            if (val == k)
                return;

        vec.push_back(val);
    };

    // Updating localization and estimating error
    const uint32_t nThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> vThr(nThreads);

    for (uint32_t tid = 0; tid < nThreads; tid++)
        vThr[tid] = std::thread(&Trajectory::enhancePoint, this, trackID, trajID, tid);

    for (std::thread &thr : vThr)
        thr.join();

    // Removing rows that didn't converge during enhancement
    MatXd &route = m_vTrack[trackID].traj[trajID];

    uint32_t nRows = uint32_t(route.rows());
    for (int32_t k = nRows - 1; k >= 0; k--)
        if (route(k, 0) < 0)
            removeRow(route, k);

    if (route.rows() < 10) // is going to be removed anyway
        return;

    // Finally, we check if there are outliers in detection and remove them
    nRows = uint32_t(route.rows());
    VecXd dx = route.block(1, Track::POSX, nRows - 1, 1) -
               route.block(0, Track::POSX, nRows - 1, 1);

    VecXd dy = route.block(1, Track::POSY, nRows - 1, 1) -
               route.block(0, Track::POSY, nRows - 1, 1);

    glm::dvec2
        thresX = thresOutliers(dx.array().abs()),
        thresY = thresOutliers(dy.array().abs()),
        thresSX = thresOutliers(route.col(Track::SIZEX)),
        thresSY = thresOutliers(route.col(Track::SIZEY)),
        thresEX = thresOutliers(route.col(Track::ERRX)),
        thresEY = thresOutliers(route.col(Track::ERRY)),
        thresSig = thresOutliers(route.col(Track::SIGNAL)),
        thresBG = thresOutliers(route.col(Track::BG));

    std::vector<uint32_t> toRemove;
    for (uint32_t k = 0; k < nRows; k++)
    {
        bool check = true;

        if (k < nRows - 1)
        {
            check &= dx(k) > thresX.x;
            check &= dx(k) < thresX.y;

            check &= dy(k) > thresY.x;
            check &= dy(k) < thresY.y;

            if (!check)
                condPush(toRemove, k + 1);
        }

        check = true;
        check &= route(k, Track::SIZEX) < thresSX.y;
        check &= route(k, Track::SIZEX) > thresSX.x;

        check &= route(k, Track::SIZEY) < thresSY.y;
        check &= route(k, Track::SIZEY) > thresSY.x;

        check &= route(k, Track::ERRX) < thresEX.y;
        check &= route(k, Track::ERRX) > thresEX.x;

        check &= route(k, Track::ERRY) < thresEY.y;
        check &= route(k, Track::ERRY) > thresEY.x;

        check &= route(k, Track::BG) < thresBG.y;
        check &= route(k, Track::BG) > thresBG.x;

        check &= route(k, Track::SIGNAL) < thresSig.y;
        check &= route(k, Track::SIGNAL) > thresSig.x;

        if (!check)
            condPush(toRemove, k);
    }

    std::sort(toRemove.begin(), toRemove.end());
    std::reverse(toRemove.begin(), toRemove.end());
    for (uint32_t k : toRemove)
        removeRow(route, k);

} // enhanceRoutes

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS

Trajectory::Trajectory(const Movie *mov, Mailbox *mail) : movie(mov), mbox(mail)
{
    uint32_t SC = movie->getMetadata().SizeC;
    m_vTrack.resize(SC); // one track per channel
}

Trajectory::~Trajectory(void) {}

bool Trajectory::useICY(const std::string &xmlTrack, uint32_t ch)
{
    const Metadata &meta = movie->getMetadata();

    pugi::xml_document doc;
    if (!doc.load_file(xmlTrack.c_str()))
    {
        if (mbox)
            mbox->create<Message::Error>("(Trajectory::useICY_XML): Couldn't open " + xmlTrack);
        else
            std::cerr << "ERROR (Trajectory::useICY_XML) ==> Couldn't open " << xmlTrack
                      << std::endl;

        return false;
    }

    pugi::xml_node group = doc.child("root").child("trackgroup");

    m_vTrack[ch].path = std::move(xmlTrack);
    m_vTrack[ch].channel = ch;
    m_vTrack[ch].description = group.attribute("description").as_string();

    for (auto xtr : group.children("track"))
    {
        std::vector<glm::dvec3> vec;
        for (auto dtc : xtr.children("detection"))
            vec.emplace_back(dtc.attribute("t").as_double(),
                             dtc.attribute("x").as_double(),
                             dtc.attribute("y").as_double());

        uint32_t counter = 0;
        MatXd mat = MatXd::Zero(vec.size(), Track::NCOLS);
        for (glm::dvec3 &txy : vec)
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

} // constructor

bool Trajectory::useCSV(const std::string &csvTrack, uint32_t ch)
{

    // mov is not const because we might want to update metadata later
    const Metadata &meta = movie->getMetadata();

    // Importing all particles for channel
    MatXd particles = loadFromTextFile(csvTrack, ',', 1, 0, mbox);

    if (particles.size() == 0)
        return false;

    Track track;
    track.channel = ch;
    track.path = std::move(csvTrack);

    // Splitting particles

    uint32_t row = 0,
             partID = uint32_t(particles(0, 0)),
             nRows = uint32_t(particles.rows());

    for (uint32_t k = 0; k < nRows; k++)
        if (particles(k, 0) != partID || k == nRows - 1)
        {
            uint32_t N = k - row;
            if (k == nRows - 1)
                N++;

            MatXd loc(N, uint32_t(Track::NCOLS));
            loc.fill(0.0);

            for (uint32_t r = 0; r < N; r++)
            {
                uint32_t fr = uint32_t(particles(row + r, 1));

                loc(r, Track::FRAME) = fr;
                loc(r, Track::POSX) = particles(row + r, 2);
                loc(r, Track::POSY) = particles(row + r, 3);

                if (meta.hasPlanes())
                    loc(r, Track::TIME) = meta.getPlane(ch, 0, fr).DeltaT;
                else
                    loc(r, Track::TIME) = fr * meta.TimeIncrement;
            }

            track.traj.emplace_back(loc);

            partID = uint32_t(particles(k, 0));
            row = k;
        }

    return true;

} // useCSV

void Trajectory::enhanceTracks(void)
{

    std::vector<uint32_t> empty;
    for (uint32_t ch = 0; ch < m_vTrack.size(); ch++)
    {
        Track &track = m_vTrack[ch];
        const uint32_t N = uint32_t(track.traj.size());

        Message::Progress *ptr = nullptr;

        if (mbox)
            ptr = mbox->create<Message::Progress>("Enhancing " + std::to_string(N) +
                                                  " trajectories for channel " +
                                                  std::to_string(ch));

        std::vector<uint32_t> toRemove;
        for (uint32_t k = 0; k < N; k++)
        {
            enhanceTrajectory(ch, k);

            if (track.traj[k].rows() < 10)
            {
                mbox->create<Message::Warn>("Trajectory smaller than 10 frames removed!!");
                toRemove.push_back(k);
            }

            if (ptr)
                ptr->progress = float(k + 1) / float(track.traj.size());
        } // loop-tracks

        std::reverse(toRemove.begin(), toRemove.end());
        for (uint32_t k : toRemove)
            track.traj.erase(track.traj.begin() + k);

        if (track.traj.size() == 0)
            empty.push_back(ch);

    } // loop-trajectories

    // Removing empty tracks
    for (uint32_t ch : empty)
        m_vTrack.erase(m_vTrack.begin() + ch);

} // enhanceTracks
