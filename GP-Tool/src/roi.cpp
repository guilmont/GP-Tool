#include "roi.h"

void Roi::addPosition(const glm::vec2& pos)
{
	if (position.size() >= maxPoints)
		return;

	position.push_back(pos.x);
	position.push_back(pos.y);
}

void Roi::removePosition(const glm::vec2& pos)
{
	for (uint64_t k = 0; k < position.size(); k += 2)
		if (abs(pos.x - position[k]) < threshold && abs(pos.y - position[k+1]) < threshold)
		{
			position.erase(position.begin() + k + 1);
			position.erase(position.begin() + k);

			return;
		}
}

void Roi::movePosition(const glm::vec2& pos)
{
	for (uint64_t k = 0; k < position.size(); k += 2)
		if (abs(pos.x - position[k]) < threshold && abs(pos.y - position[k + 1]) < threshold)
		{
			position[k] = pos.x;
			position[k+1] = pos.y;

			return;
		}
}

bool Roi::contains(const glm::vec2& pos)
{
    auto crossLine = [](const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& pos) -> uint64_t {
        glm::vec2 dp = p2 - p1;
        glm::mat2 mat(pos.x, pos.y, -dp.x, -dp.y);

        if (glm::determinant(mat) == 0.0)
            return 0;

        glm::vec2 su = glm::inverse(mat) * p1;
        if (su.x >= 0 && su.x <= 1.0 && su.y >= 0.0 && su.y <= 1.0)
            return 1;
        else
            return 0;
    };


    uint64_t inner = 0;
    for (uint64_t k = 0; k < position.size(); k+=2)
    {
        glm::vec2 p1 = { position[k], position[k+1] };

        uint64_t l = (k + 2) % position.size();
        glm::vec2 p2 = { position[l], position[l + 1] };

        inner += crossLine(p1, p2, pos);
    }

    return !(inner % 2) == 0;
}