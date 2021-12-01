#pragma once

#include "header.h"

namespace GPT::Filter
{
	// Base class from which all the other filters will be derived
	struct Filter
	{
		Filter(void) = default;
		virtual ~Filter(void) = default;

		virtual void apply(MatXd& img) = 0;
	};

	struct Contrast : public Filter
	{
		double low = -1.0, high = -1.0;

		// Negative values mean auto-contrast
		Contrast(double low = -1, double high = -1) : low(low), high(high) {}
		~Contrast(void) = default;

		void apply(MatXd& img) override;
	};


	struct Median : public Filter
	{
		// For now, we are going to shrink the tile around the boundary

		uint64_t sizeX, sizeY;
		Median(uint64_t sizeX, uint64_t sizeY) : sizeX(sizeX), sizeY(sizeY) {}
		~Median(void) = default;

		void apply(MatXd& img) override;
	};

	struct CLAHE : public Filter
	{
		double clipLimit;
		uint64_t tileSizeX, tileSizeY;

		CLAHE(double clipLimit, uint64_t tileSizeX, uint64_t tileSizeY) : clipLimit(clipLimit), tileSizeX(tileSizeX), tileSizeY(tileSizeY) {}
		~CLAHE(void) = default;

		void apply(MatXd& img) override;
	};


	struct SVD : public Filter
	{
		// We will need to know about all the images in the stack to perform this filter
		// Because of that, we will need to perform per filter treatment in all images, not one image at the time

		uint64_t slice, rank, frame = 0;
		std::vector<MatXd> vImages; 

		SVD(std::vector<MatXd> vImages, uint64_t slice, uint64_t rank) : vImages(vImages), slice(slice), rank(rank) {}
		~SVD(void) = default;

		void setFrame(uint64_t frame);
		void apply(MatXd& img) override;
	};
}