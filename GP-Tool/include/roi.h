#pragma once

#include "gpch.h"

class Roi
{
public:
	Roi(void) = default;
	~Roi(void) = default;

	bool contains(const glm::vec2& pos);
	void clear(void) { position.clear(); }
		 
	void addPosition(const glm::vec2& pos);
	void removePosition(const glm::vec2& pos);
	void movePosition(const glm::vec2& pos);

	glm::vec4& getColor(void) { return color; }

	uint64_t getNumPoints(void) { return position.size() >> 1; }
	const float* getData(void) { return position.data(); }

private:
	const uint64_t maxPoints = 40;   // 2 times the number of points for XY

	int64_t findClosest(const glm::vec2& pos);


	glm::vec4 color = { 1.0f, 0.5f, 0.1f, 0.1f };
	std::vector<float> position;
};

