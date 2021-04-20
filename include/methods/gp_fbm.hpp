#pragma once
#include <config.hpp>

#include <bitset>

struct ParamDA
{
    double D, A, mu[3];
};

struct ParamCDA
{
    std::vector<ParamDA> particle;
    double DR, AR;
};

//////////////////////

class GP_FBM
{
    friend double transferSingle(const VectorXd &, void *);
    friend double transferCoupled(const VectorXd &, void *);

public:
    GP_FBM(MatrixXd &mat, Mailbox *mailbox);
    GP_FBM(std::vector<MatrixXd> &vecMat, Mailbox *mailbox);

    // CONTROL
    uint32_t getNumParticles(void) const { return route.size(); }
    bool hasSingle(void) const { return bControl[SINGLE]; }
    bool hasDistribSingle(void) const { return bControl[DTB_SINGLE]; }
    bool hasCoupled(void) const { return bControl[COUPLED]; }
    bool hasDistribCoupled(void) const { return bControl[DTB_COUPLED]; }
    bool hasSubstrate(void) const { return bControl[SUBSTRATE]; }

    // SETTERS
    void setSingleParameter(const ParamDA &par); // in case there is only one
    void setSingleParameters(const std::vector<ParamDA> &vec);
    void setCoupledParameters(const ParamCDA &par);
    void setParticleID(const std::vector<std::pair<uint32_t, uint32_t>> &partID);

    // GETTERS
    const ParamDA *getSingleParameters(void) const;
    const MatrixXd *getSingleDistribution(void) const;

    const ParamCDA &getCoupledParameters(void) const;
    const MatrixXd &getCoupledDistribution(void) const;

    const std::vector<std::pair<uint32_t, uint32_t>> &getParticleID(void) const;

    // FUNCTIONAL
    bool singleModel(void);
    bool coupledModel(void);

    void distrib_singleModel(void);
    void distrib_coupledModel(void);
    inline void setDistributionSize(uint32_t val) { maxMCS = val; }

    void calcAvgTrajectory(const VectorXd &vFrame, const VectorXd &vTime,
                           const uint32_t id = 0);

    const MatrixXd &getAvgTrajectory(const uint32_t id = 0) const { return vecAvgTraj[id]; }

    void estimateSubstrateMovement(void);
    const MatrixXd &getSubstrate(void) const { return substrate; }

private:
    Mailbox *mail = nullptr;

    double weightSingle(const VectorXd &DA);
    double weightCoupled(const VectorXd &DA);

private:
    enum // for control
    {
        SINGLE,
        DTB_SINGLE,
        COUPLED,
        DTB_COUPLED,
        SUBSTRATE,
        NBITS
    };

    std::bitset<NBITS> bControl;

    uint32_t runId = 0;          // which single trajectory is running
    MatrixXd cRoute;             // concatenates all routes in one
    std::vector<MatrixXd> route; // Holders all particles's routes

    std::vector<ParamDA> parSingle;
    ParamCDA parCoupled;

    std::vector<MatrixXd> distribSingle;
    MatrixXd distribCoupled;
    MatrixXd substrate;

    uint32_t nDims = 1;
    uint32_t maxMCS = 2000;
    double thresSimplex = 1e-4; // precision for simplex optimization

    std::vector<std::pair<uint32_t, uint32_t>> vPartID;
    std::vector<MatrixXd> vecAvgTraj;

}; // class-GP_FBM
