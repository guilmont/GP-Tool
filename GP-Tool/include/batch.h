#pragma once

#include "gpch.h"

#include "movie.h"
#include "trajectory.h"
#include "align.h"
#include "gp_fbm.h"

class GPTool;


class Batching {
public:
	Batching(void) = default;
	Batching(GPTool* ptr);
	~Batching(void) = default;

	void setActive(void);
	void imguiLayer(void);
	void run(void);

private:
	enum ALIGN : int32_t
	{
		INDIVIDUAL = 0,
		BUNDLED = 1,
	};

	struct MovData
	{
		fs::path moviePath;
		std::vector<fs::path> trajPath;
	};

	std::vector<MovData> vecSamples; // Stores the informaton of all movies to analyze
	

private:
	Json::Value outputJson; // Storage for all the analyzed data to be saved
	
	std::vector<std::vector<MatXd>> vecImagesToAlign; // used in the case of bundled alignement

	void runSamples(const int32_t threadId);

private:
	GPTool* tool = nullptr;

	bool
		view_batching = false,
		cancelBatch = false,
		runTrajectories = false,
		runAlignment = false,
		runGPFBM = false;

	// Alignment
	bool
		checkCamera = true,
		checkAberration = true;

	// GP-FBM side
	bool
		checkSingle = false,
		checkInterpol = false,
		checkCoupled = false,
		checkSubstrate = false;

	int32_t
		alignID = 0,
		spotSize = 3,
		numChannels = 1,
		numThreads = 1;

	std::vector<std::array<char, 512>> suffices;


	char mainPath[1024] = { 0 };
	char outPath[1024] = { 0 };
};