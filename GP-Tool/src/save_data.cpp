// /*****************************************************************************/
// /*****************************************************************************/
// // INPUT-OUTPUT FUNCTIONS

// static void saveJSON(const String &path, MainStruct &data)
// {

//     if (!data.align && data.vecGP.size() == 0)
//     {
//         data.showInbox_flag = true;
//         data.mail.createMessage<MSG_Warning>("Nothing to be saved!");
//         return;
//     }

//     auto toArray = [](const double &a1, const double &a2) -> Json::Value {
//         Json::Value vec;
//         vec.append(a1);
//         vec.append(a2);
//         return vec;
//     };

//     auto convertEigenJSON = [](const MatrixXd &mat) -> Json::Value {
//         Json::Value array;
//         for (uint32_t k = 0; k < mat.rows(); k++)
//         {
//             Json::Value row;
//             for (uint32_t l = 0; l < mat.cols(); l++)
//                 row.append(mat(k, l));

//             array.append(row);
//         }
//         return array;
//     };

//     Json::Value output;

//     // First let's check if we have alignement matrix to store
//     if (data.align)
//     {
//         const TransformData &RT = data.align->getTransformData();

//         Json::Value align;
//         align["dimensions"] = toArray(RT.width, RT.height);
//         align["translation"] = toArray(RT.dx, RT.dy);
//         align["rotation"]["center"] = toArray(RT.cx, RT.cy);
//         align["rotation"]["angle"] = RT.angle;
//         align["scaling"] = toArray(RT.sx, RT.sy);
//         align["transform"] = convertEigenJSON(RT.trf);

//         output["Alignment"] = std::move(align);

//     } // if-align

//     // Storing cells
//     const Metadata &meta = data.movie->getMetadata();

//     const uint32_t nCells = data.vecGP.size();
//     const double Dcalib = meta.PhysicalSizeXY * meta.PhysicalSizeXY;

//     for (uint32_t id = 0; id < nCells; id++)
//     {
//         GP_FBM &gp = data.vecGP[id];

//         Json::Value cell;
//         const uint32_t nParticles = gp.getNumParticles();

//         // First let's save trajectories
//         uint32_t counter = 0;

//         Json::Value traj;
//         traj["PhysicalSizeXY"] = meta.PhysicalSizeXY;
//         traj["PhysicalSizeXYUnit"] = meta.PhysicalSizeXYUnit;
//         traj["TimeIncrementUnit"] = meta.TimeIncrementUnit;
//         traj["columns"] = "{frame, time, pos_x, error_x, pos_y, error_y,"
//                           "size_x, size_y, signal, background}";

//         for (auto &[ch, id] : gp.getParticleID())
//         {
//             const MatrixXd &mat = data.track->getTrack(ch).traj[id];
//             traj["trajectory_" + std::to_string(counter++)] = convertEigenJSON(mat);
//         } // loop-trajectories

//         cell["trajectories"] = std::move(traj);

//         // Now we save information about D and alpha
//         if (gp.hasSingle())
//         {
//             Json::Value single;
//             single["D_units"] = meta.PhysicalSizeXYUnit + "^2/" +
//                                 meta.TimeIncrementUnit + "^A";
//             single["point_at_zero_units"] = "pixels";
//             single["columns"] = "{D, A, zero_x, zero_y}";

//             const ParamDA *da = gp.getSingleParameters();
//             MatrixXd mat(nParticles, 4);
//             for (uint32_t k = 0; k < nParticles; k++)
//                 mat.row(k) << (Dcalib * da[k].D), da[k].A, da[k].mu[0], da[k].mu[1];

//             single["dynamics"] = convertEigenJSON(mat);

//             cell["without_substrate"] = std::move(single);
//         } // single-model

//         if (gp.hasCoupled())
//         {
//             Json::Value coupled;
//             coupled["D_units"] = meta.PhysicalSizeXYUnit + "^2/" +
//                                  meta.TimeIncrementUnit + "^A";
//             coupled["columns"] = "{D, A}";

//             MatrixXd mat(nParticles, 2);
//             const ParamCDA &cda = gp.getCoupledParameters();
//             for (uint32_t k = 0; k < nParticles; k++)
//                 mat.row(k) << (Dcalib * cda.particle[k].D), cda.particle[k].A;

//             coupled["dynamics"] = convertEigenJSON(mat);

//             // Substrate
//             coupled["substrate"]["dynamics"] = toArray(Dcalib * cda.DR, cda.AR);

//             if (!gp.hasSubstrate())
//                 gp.estimateSubstrateMovement();

//             coupled["substrate"]["trajectory"] = convertEigenJSON(gp.getSubstrate());

//             cell["with_substrate"] = std::move(coupled);

//         } // coupled-model

//         output["Cells"].append(std::move(cell));

//     } // loop-cells

//     std::fstream arq(path, std::fstream::out);
//     arq << output;
//     arq.close();

// } // saveJSON

// static void saveHDF5(const String &path, MainStruct &data)
// {
//     if (!data.align && data.vecGP.size() == 0)
//     {
//         data.showInbox_flag = true;
//         data.mail.createMessage<MSG_Warning>("Nothing to be saved!");
//         return;
//     }

//     GHDF5 output;

//     // First let's check if we have alignement matrix to store
//     if (data.align)
//     {
//         const TransformData &RT = data.align->getTransformData();

//         GHDF5 &align = output["Alignment"];

//         uint32_t dim[] = {RT.width, RT.height};
//         align["dimensions"].createFromPointer(dim, 2);

//         std::array<double, 2> arr = {RT.dx, RT.dy};
//         align["translation"].createFromPointer(arr.data(), 2);

//         arr = {RT.cx, RT.cy};
//         align["rotation"]["center"].createFromPointer(arr.data(), 2);
//         align["rotation"]["angle"] = RT.angle;

//         arr = {RT.sx, RT.sy};
//         align["scaling"].createFromPointer(arr.data(), 2);

//         MatrixXd trans = RT.trf.transpose();
//         align["transform"].createFromPointer(trans.data(), trans.rows(), trans.cols());

//     } // if-align

//     // Storing cells
//     const Metadata &meta = data.movie->getMetadata();

//     const uint32_t nCells = data.vecGP.size();
//     const double Dcalib = meta.PhysicalSizeXY * meta.PhysicalSizeXY;

//     for (uint32_t id = 0; id < nCells; id++)
//     {
//         GP_FBM &gp = data.vecGP[id];

//         GHDF5 &cell = output["Cells"]["cell_" + std::to_string(id)];

//         // First let's save trajectories
//         const uint32_t nParticles = gp.getNumParticles();
//         uint32_t counter = 0;

//         GHDF5 &traj = cell["trajectories"];
//         traj.setAttribute("PhysicalSizeXY", meta.PhysicalSizeXY);
//         traj.setAttribute("PhysicalSizeXYUnit", meta.PhysicalSizeXYUnit);
//         traj.setAttribute("TimeIncrementUnit", meta.TimeIncrementUnit);
//         traj.setAttribute("columns", "{frame, time, pos_x, error_x, pos_y, error_y,"
//                                      "size_x, size_y, signal, background}");

//         for (auto &[ch, id] : gp.getParticleID())
//         {
//             String txt = "trajectory_" + std::to_string(counter++);
//             MatrixXd mat = data.track->getTrack(ch).traj[id].transpose();
//             traj[txt].createFromPointer(mat.data(), mat.rows(), mat.cols());
//         } // loop-trajectories

//         // Now we save information about D and alpha
//         if (gp.hasSingle())
//         {
//             GHDF5 &single = cell["without_substrate"];

//             single.setAttribute("point_at_zero_units", "pixels");
//             single.setAttribute("columns", "{D, A, zero_x, zero_y}");
//             single.setAttribute("D_units", meta.PhysicalSizeXYUnit + "^2/" +
//                                                meta.TimeIncrementUnit + "^A");

//             const ParamDA *da = gp.getSingleParameters();
//             MatrixXd mat(nParticles, 4);
//             for (uint32_t k = 0; k < nParticles; k++)
//                 mat.row(k) << (Dcalib * da[k].D), da[k].A, da[k].mu[0], da[k].mu[1];

//             MatrixXd trans = mat.transpose();
//             single["dynamics"].createFromPointer(trans.data(),
//                                                  trans.rows(), trans.cols());

//         } // single-model

//         if (gp.hasCoupled())
//         {
//             GHDF5 &coupled = cell["with_substrate"];

//             coupled.setAttribute("columns", "{D, A}");
//             coupled.setAttribute("D_units", meta.PhysicalSizeXYUnit + "^2/" +
//                                                 meta.TimeIncrementUnit + "^A");

//             MatrixXd mat(nParticles, 2);
//             const ParamCDA &cda = gp.getCoupledParameters();
//             for (uint32_t k = 0; k < nParticles; k++)
//                 mat.row(k) << (Dcalib * cda.particle[k].D), cda.particle[k].A;

//             MatrixXd trans = mat.transpose();
//             coupled["dynamics"].createFromPointer(trans.data(),
//                                                   trans.rows(), trans.cols());

//             // Substrate
//             double arr[] = {Dcalib * cda.DR, cda.AR};
//             coupled["substrate"]["dynamics"].createFromPointer(arr, 2);

//             if (!gp.hasSubstrate())
//                 gp.estimateSubstrateMovement();

//             trans = gp.getSubstrate().transpose();
//             coupled["substrate"]["trajectory"].createFromPointer(trans.data(),
//                                                                  trans.rows(),
//                                                                  trans.cols());

//         } // coupled-model

//     } // loop-cells

//     // Saving to file
//     output.save(path);
// } // saveHDF5

// static void saveData(MainStruct *data)
// {
//     // Generic function to save data based on extension

//     const String &path = data->dialog.getPath2File();

//     if (path.find(".hdf") != String::npos)
//         saveHDF5(path, *data);

//     else if (path.find(".json") != String::npos)
//         saveJSON(path, *data);

//     else
//     {
//         data->mail.createMessage<MSG_Warning>("Cannot save data to '" + path + "'");
//         data->showInbox_flag = true;
//         return;
//     }

//     data->mail.createMessage<MSG_Info>("Data saved to " + path);
//     data->showInbox_flag = true;

// } // saveDataDialog

// static void saveCSV(std::pair<MainStruct *, const MatrixXd *> info)
// {

//     auto [data, mat] = info;

//     const String &path = data->dialog.getPath2File();

//     const char *names[] = {"Frame", "Time", "Position X", "Error X",
//                            "Position Y", "Error Y", "Size X", "Size Y",
//                            "signal", "Background"};

//     const uint32_t nCols = mat->cols(),
//                    nRows = mat->rows();

//     std::ofstream arq(path);
//     // Header
//     for (uint32_t l = 0; l < nCols; l++)
//         arq << names[l] << (l == nCols - 1 ? "\n" : ", ");

//     // Body
//     for (uint32_t k = 0; k < nRows; k++)
//         for (uint32_t l = 0; l < nCols; l++)
//             arq << (*mat)(k, l) << (l == nCols - 1 ? "\n" : ", ");

//     arq.close();

//     data->mail.createMessage<MSG_Info>("Data saved to " + path);
//     data->showInbox_flag = true;

// } // saveCSV