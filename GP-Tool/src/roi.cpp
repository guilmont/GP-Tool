#include "roi.h"

void Roi::addPosition(const glm::vec2& pos)
{
	if (position.size() >= maxPoints)
		return;

	position.push_back(pos.x);
	position.push_back(pos.y);
}

int64_t Roi::findClosest(const glm::vec2& pos)
{
    int64_t id = 0;
    float dist = INFINITY;
	for (uint64_t k = 0; k < position.size(); k += 2)
    {
        float dx = pos.x - position[k], dy = pos.y - position[k+1];
        float loc = dx*dx + dy*dy;
        if (loc < dist)
        {
            id = k;
            dist = loc;
        }
    }

    if (dist < 0.001)
        return id;
    else
        return -1;
}

void Roi::removePosition(const glm::vec2& pos)
{
    int64_t id = findClosest(pos);
    if (id >= 0)
    {
        position.erase(position.begin() + id + 1);
        position.erase(position.begin() + id);
    }
}

void Roi::movePosition(const glm::vec2& pos)
{
   int64_t id = findClosest(pos);
    if (id >= 0)
    {
        position[id] = pos.x;
		position[id+1] = pos.y;
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