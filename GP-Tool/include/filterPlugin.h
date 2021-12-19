#pragma once

#include "gpch.h"
#include "plugin.h"
#include "filters.h"

#include "gptool.h"
class GPTool;

class FilterPlugin : public Plugin
{
public:
	FilterPlugin(GPT::Movie *mov, GPTool* ptr);
	~FilterPlugin(void);

	void showProperties(void) override;
	void update(float deltaTime) override;
	void showWindows(void) override;

	void loadImages(void); 
	void applyFilters(void);
	void saveImages(const fs::path& address);

private:
	// Funtions to display filter options in properties tab
	void displayContrast(GPT::Filter::Contrast* ptr);
	void displayMedian(GPT::Filter::Median* ptr);
	void displayCLAHE(GPT::Filter::CLAHE* ptr);
	void displaySVD(GPT::Filter::SVD* ptr);

private:
	GPT::Movie* mov = nullptr;
	GPTool* tool = nullptr;

	std::vector<MatXd> vImages;
	std::map<std::string, GPT::Filter::Filter*> vFilters;


private:
	GRender::Progress* prog = nullptr;
	
	bool
		cancel = false,
		updateTexture = true,
		viewWindow = false,
		viewHover = false,
		first = true;

	int32_t
		currentCH = 0,
		currentFR = 0,
		numThreads = 1,
		filterCounter = 0;

	std::unique_ptr<GRender::Quad> quad = nullptr;	
	std::unique_ptr<GRender::Framebuffer> fBuffer = nullptr;

	GRender::Camera2D camera;
};