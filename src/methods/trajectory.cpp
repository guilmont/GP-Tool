#include <trajectory.hpp>

using Data = std::tuple<uint32_t, const MatrixXd *, MatrixXd &>;
using Detection = std::tuple<double, double, double>;

static void removeRow(MatrixXd &matrix, uint32_t rowToRemove)
{
    unsigned int numRows = matrix.rows() - 1;
    unsigned int numCols = matrix.cols();

    if (rowToRemove < numRows)
        matrix.block(rowToRemove, 0, numRows - rowToRemove, numCols) =
            matrix.block(rowToRemove + 1, 0, numRows - rowToRemove, numCols);

    matrix.conservativeResize(numRows, numCols);
}

static std::pair<double, double> thresOutliers(VectorXd vec)
{
    std::sort(vec.data(), vec.data() + vec.size());

    uint32_t N = 0.5 * vec.size(),
             FQ = 0.25 * vec.size(), TQ = 0.75 * vec.size();

    double fifty = vec(TQ) - vec(FQ);

    return {vec(N) - 2.0 * fifty, vec(N) + 2.0 * fifty};
} // thresOutliers

MatrixXd loadFromTextFile(const std::string &path, char delimiter,
                          uint32_t skip_rows, uint32_t skip_cols, Mailbox *mail)
{

    std::ifstream arq(path, std::fstream::binary);
    if (arq.fail())
    {
        mail->createMessage<MSG_Warning>("(Trajectory::useCSV): Cannot open " + path);
        return MatrixXd(0, 0);
    }

    std::string data;
    arq.seekg(0, arq.end);
    data.resize(arq.tellg());

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
            mail->createMessage<MSG_Warning>("(Trajectory::useCSV): "
                                             "More skip_rows than number of rows!!");
            return MatrixXd(0, 0);
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
        mail->createMessage<MSG_Warning>("(Trajectory::useCSV): "
                                         "Input matrix is empty!!");

        return MatrixXd(0, 0);
    }

    const uint32_t
        NX = mat[0].size(),
        NY = mat.size();

    if (sum != mat.at(0).size() * mat.size())
    {
        mail->createMessage<MSG_Warning>("(Trajectory::useCSV): "
                                         " Input txt have rows of different widths!!");
        return MatrixXd(0, 0);
    }

    MatrixXd obj(NY, NX);
    for (uint32_t k = 0; k < NY; k++)
        for (uint32_t l = 0; l < NX; l++)
            obj(k, l) = mat[k][l];

    return obj;

} // readTextFile

///////////////////////////////////////////////////////////////////////////////

static void enhancePoint(const uint32_t pt, void *ptr)
{
    auto [radius, vImg, route] = *reinterpret_cast<Data *>(ptr);

    uint32_t sRoi = 2 * radius + 1;

    // Get frame and coordinates
    int frame = route(pt, TrajColumns::FRAME),
        px = route(pt, TrajColumns::POSX),
        py = route(pt, TrajColumns::POSY);

    // Let's check if all ROI pixels are within the image
    int px_o = px - radius, px_f = px_o + sRoi,
        py_o = py - radius, py_f = py_o + sRoi;

    bool check = true;
    check &= px_o >= 0;
    check &= px_f < vImg[frame].cols();
    check &= py_o >= 0;
    check &= py_f < vImg[frame].rows();

    if (!check)
    {
        // Cannot really work in this situation
        route(pt, 0) = -1;
        return;
    }

    MatrixXd roi = vImg[frame].block(py - radius, px - radius, sRoi, sRoi);

    // Correcting contrast
    double bot = roi.minCoeff();
    double top = roi.maxCoeff();

    roi.array() -= bot;
    roi.array() *= 255.0f / (top - bot);

    // Sending roi to Spot class for refinement
    Spot spot(roi);
    if (spot.successful())
    { // Everything went well
        const SpotInfo &info = spot.getSpotInfo();

        if (std::isnan(info.mX) || std::isnan(info.mY))
            route(pt, 0) = -1;
        else
        {

            // recentering posision
            route(pt, TrajColumns::POSX) = px + 0.5 + (info.mX - 0.5 * sRoi);
            route(pt, TrajColumns::POSY) = py + 0.5 + (info.mY - 0.5 * sRoi);
            route(pt, TrajColumns::ERRX) = info.eX;
            route(pt, TrajColumns::ERRY) = info.eY;
            route(pt, TrajColumns::SIZEX) = 3.0 * info.lX;
            route(pt, TrajColumns::SIZEY) = 3.0 * info.lY;
            route(pt, TrajColumns::INTENSITY) = info.I0;
            route(pt, TrajColumns::BACKGROUND) = info.BG;
        }
    }
    else
    {
        route(pt, 0) = -1;
    } // spot-successful

} // EnhanceRoute

static MatrixXd enhanceRoute(const uint32_t spotRadius, const MatrixXd *vImg,
                             MatrixXd route)
{

    auto condPush = [](std::vector<uint32_t> &vec, uint32_t val) -> void {
        for (uint32_t k : vec)
            if (val == k)
                return;

        vec.push_back(val);
    };

    uint32_t nRows = route.rows();

    Data data = {spotRadius, vImg, route};

    // Updating localization and estimating error
    Threadpool pool;
    pool.run(&enhancePoint, nRows, &data);

    // Removing rows that didn't converge during enhancement
    for (int32_t k = nRows - 1; k >= 0; k--)
        if (route(k, 0) < 0)
            removeRow(route, k);

    // Updating route length
    nRows = route.rows();

    if (nRows < 10) // is going to be removed anyway
        return route;

    // Finally, we check if there are outliers in detection and remove them
    VectorXd dx = route.block(1, TrajColumns::POSX, nRows - 1, 1) -
                  route.block(0, TrajColumns::POSX, nRows - 1, 1);

    VectorXd dy = route.block(1, TrajColumns::POSY, nRows - 1, 1) -
                  route.block(0, TrajColumns::POSY, nRows - 1, 1);

    auto [thresX_low, thresX_high] = thresOutliers(dx.array().abs());
    auto [thresY_low, thresY_high] = thresOutliers(dy.array().abs());
    auto [thresSX_low, thresSX_high] = thresOutliers(route.col(TrajColumns::SIZEX));
    auto [thresSY_low, thresSY_high] = thresOutliers(route.col(TrajColumns::SIZEY));
    auto [thresEX_low, thresEX_high] = thresOutliers(route.col(TrajColumns::ERRX));
    auto [thresEY_low, thresEY_high] = thresOutliers(route.col(TrajColumns::ERRY));
    auto [thresSig_low, thresSig_high] = thresOutliers(route.col(TrajColumns::INTENSITY));
    auto [thresBG_low, thresBG_high] = thresOutliers(route.col(TrajColumns::BACKGROUND));

    std::vector<uint32_t> toRemove;
    for (uint32_t k = 0; k < nRows; k++)
    {
        bool check = true;

        if (k < nRows - 1)
        {
            check &= dx(k) > thresX_low;
            check &= dx(k) < thresX_high;

            check &= dy(k) > thresY_low;
            check &= dy(k) < thresY_high;

            if (!check)
                condPush(toRemove, k + 1);
        }

        check = true;
        check &= route(k, TrajColumns::SIZEX) < thresSX_high;
        check &= route(k, TrajColumns::SIZEX) > thresSX_low;

        check &= route(k, TrajColumns::SIZEY) < thresSY_high;
        check &= route(k, TrajColumns::SIZEY) > thresSY_low;

        check &= route(k, TrajColumns::ERRX) < thresEX_high;
        check &= route(k, TrajColumns::ERRX) > thresEX_low;

        check &= route(k, TrajColumns::ERRY) < thresEY_high;
        check &= route(k, TrajColumns::ERRY) > thresEY_low;

        check &= route(k, TrajColumns::INTENSITY) < thresSig_high;
        check &= route(k, TrajColumns::INTENSITY) > thresSig_low;

        check &= route(k, TrajColumns::BACKGROUND) < thresBG_high;
        check &= route(k, TrajColumns::BACKGROUND) > thresBG_low;

        if (!check)
            condPush(toRemove, k);
    }

    std::sort(toRemove.begin(), toRemove.end());
    std::reverse(toRemove.begin(), toRemove.end());
    for (uint32_t k : toRemove)
        removeRow(route, k);

    return route;

} // Constructor

///////////////////////////////////////////////////////////////////////////////

Trajectory::Trajectory(Trajectory &&obj)
{
    this->movie = obj.movie;
    this->m_spotRadius = obj.m_spotRadius;
    this->mail = obj.mail;

    this->m_vTrack.insert(std::make_move_iterator(obj.m_vTrack.begin()),
                          std::make_move_iterator(obj.m_vTrack.end()));

} // move-constructor

Trajectory::~Trajectory(void)
{
    movie = nullptr;
    m_vTrack.clear();
} // destructor

bool Trajectory::useICY(const std::string &xmlTrack, uint32_t ch)
{
    if (!movie)
    {
        mail->createMessage<MSG_Error>(
            "(Trajectory::useICY_XML): Movie not loaded!!");
        return false;
    }

    // mov is not const because we might want to update metadata later
    const Metadata &meta = movie->getMetadata();

    pugi::xml_document doc;
    if (!doc.load_file(xmlTrack.c_str()))
    {
        mail->createMessage<MSG_Warning>(
            "(Trajectory::useICY_XML): Couldn't open " + xmlTrack);

        return false;
    }

    pugi::xml_node group = doc.child("root").child("trackgroup");

    Track track;

    track.path = std::move(xmlTrack);
    track.channel = ch;
    track.description = group.attribute("description").as_string();
    track.vImage = movie->getChannel(ch);

    for (auto xtr : group.children("track"))
    {
        std::vector<Detection> vec;
        for (auto dtc : xtr.children("detection"))
            vec.emplace_back(dtc.attribute("t").as_double(),
                             dtc.attribute("x").as_double(),
                             dtc.attribute("y").as_double());

        uint32_t counter = 0;
        MatrixXd mat = MatrixXd::Zero(vec.size(), TrajColumns::NCOLS);
        for (auto [t, x, y] : vec)
        {
            mat(counter, TrajColumns::FRAME) = t;
            mat(counter, TrajColumns::POSX) = x;
            mat(counter, TrajColumns::POSY) = y;

            if (meta.hasPlanes())
                mat(counter, TrajColumns::TIME) = meta.getPlane(ch, 0, t).DeltaT;
            else
                mat(counter, TrajColumns::TIME) = t * meta.TimeIncrement;

            counter++;
        }

        track.traj.push_back(mat);
    } // loop-tracks

#ifdef MESSAGE_IMGUI
    for (uint32_t k = 0; k < track.traj.size(); k++)
    {
        // Preparing to display in gui
        std::array<float, 3> tag = {
            float(rand()) / RAND_MAX,
            float(rand()) / RAND_MAX,
            float(rand()) / RAND_MAX};

        track.view.push_back({true, std::move(tag)});
    }
#endif

    m_vTrack[ch] = std::move(track);

    return true;

} // constructor

bool Trajectory::useCSV(const std::string &csvTrack, uint32_t ch)
{
    if (!movie)
    {
        mail->createMessage<MSG_Error>("ERROR (Trajectory::useCSV): Movie not loaded!!");
        return false;
    }

    // mov is not const because we might want to update metadata later
    const Metadata &meta = movie->getMetadata();

    // Importing all particles for channel
    MatrixXd particles = loadFromTextFile(csvTrack, ',', 1, 0, mail);

    if (particles.size() == 0)
        return false;

    Track track;
    track.channel = ch;
    track.path = std::move(csvTrack);
    track.vImage = movie->getChannel(ch);

    // Splitting particles

    uint32_t row = 0,
             partID = particles(0, 0),
             nRows = particles.rows();

    for (uint32_t k = 0; k < nRows; k++)
        if (particles(k, 0) != partID || k == nRows - 1)
        {
            uint32_t N = k - row;
            if (k == nRows - 1)
                N++;

            MatrixXd loc(N, uint32_t(TrajColumns::NCOLS));
            loc.fill(0.0);

            for (uint32_t r = 0; r < N; r++)
            {
                uint32_t fr = particles(row + r, 1);

                loc(r, TrajColumns::FRAME) = fr;
                loc(r, TrajColumns::POSX) = particles(row + r, 2);
                loc(r, TrajColumns::POSY) = particles(row + r, 3);

                if (meta.hasPlanes())
                    loc(r, TrajColumns::TIME) = meta.getPlane(ch, 0, fr).DeltaT;
                else
                    loc(r, TrajColumns::TIME) = fr * meta.TimeIncrement;
            }

            track.traj.emplace_back(loc);

            partID = particles(k, 0);
            row = k;
        }

#ifdef MESSAGE_IMGUI
    for (uint32_t k = 0; k < track.traj.size(); k++)
    {
        // Preparing to display in gui
        std::array<float, 3> tag = {
            float(rand()) / RAND_MAX,
            float(rand()) / RAND_MAX,
            float(rand()) / RAND_MAX};

        track.view.push_back({true, std::move(tag)});
    }
#endif

    // Moving dato into main memory
    m_vTrack[ch] = track;

    return true;

} // useCSV

void Trajectory::enhanceTracks(void)
{

    std::vector<uint32_t> empty;
    for (auto &[ch, track] : m_vTrack)
    {
        const uint32_t N = track.traj.size();

        MSG_Progress *msg = mail->createMessage<MSG_Progress>(
            "Enhancing " + std::to_string(N) +
            " trajectories for channel " + std::to_string(ch));

        std::vector<uint32_t> toRemove;
        for (uint32_t k = 0; k < N; k++)
        {
            track.traj[k] = enhanceRoute(m_spotRadius, track.vImage, track.traj[k]);

            if (track.traj[k].rows() < 10)
            {
                mail->createMessage<MSG_Warning>(
                    "Trajectory smaller than 10 frames removed!!");

                toRemove.push_back(k);
            }

            msg->update((k + 1) / float(track.traj.size()));
        } // loop-tracks

        std::reverse(toRemove.begin(), toRemove.end());
        for (uint32_t k : toRemove)
            track.traj.erase(track.traj.begin() + k);

        if (track.traj.size() == 0)
            empty.push_back(ch);

        msg->markAsRead();

    } // loop-trajectories

    // Removing empty tracks
    for (uint32_t ch : empty)
        m_vTrack.erase(ch);

} // enhanceTracks