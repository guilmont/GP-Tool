#pragma once

#include "header.h"

namespace GPT::Filter
{
	// Base class from which all the other filters will be derived
	struct Filter
	{
		Filter(void) = default;
		virtual ~Filter(void) = default;

		virtual void apply(MatXd& img) {}
	};

	struct Contrast : public Filter
	{
		double low = -1.0, high = -1.0; // Negative values mean auto-contrast, otherwise, betweem 0 and 1

		GP_API Contrast(double lowValue, double highValue) : low(lowValue), high(highValue) {}
		GP_API Contrast(void) = default;
		GP_API ~Contrast(void) = default;

		GP_API void apply(MatXd& img) override;
	};


	struct Median : public Filter
	{
		// For now, we are going to shrink the tile around the boundary

		int64_t sizeX = 5, sizeY = 5;

		GP_API Median(int64_t tileSizeX, int64_t tileSizeY) : sizeX(tileSizeX), sizeY(tileSizeY) {}
		GP_API Median(void) = default;
		GP_API ~Median(void) = default;

		GP_API void apply(MatXd& img) override;
	};

	struct CLAHE : public Filter
	{
		double clipLimit = 2.0;
		int64_t 
			tileSizeX = 32,
			tileSizeY = 32;

		GP_API CLAHE(double clipLimit, int64_t tileSizeX, int64_t tileSizeY) : clipLimit(clipLimit), tileSizeX(tileSizeX), tileSizeY(tileSizeY) {}
		GP_API CLAHE(void) = default;
		GP_API ~CLAHE(void) = default;

		GP_API void apply(MatXd& img) override;
	};


	class SVD : public Filter
	{
		// I'm not sure I like all the copies happening, but it should do for now
	public:
		GP_API SVD(void) = default;
		GP_API ~SVD(void) = default;

		int64_t slice = 1, rank = 1;

		GP_API void importImages(const std::vector<MatXd>& vec);
		GP_API void updateImages(std::vector<MatXd>& vec);  // Copies denoised images into vec
		GP_API const MatXd& getImage(int64_t frame);

		GP_API void run(bool &trigger);

	private:
		std::vector<MatXd> vImages, denoised;
	};
}