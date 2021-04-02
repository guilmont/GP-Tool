#pragma once

#include <glm/glm.hpp>

class Camera
{
private:
      float
          fNear = 0.001f,
          fFar = 1000.0f;

      float maxZoom = 0.05,
            minZoom = 1.0f;

      float speed = 1.0f,
            zoomSpeed = 1.0f,
            sensitivity = 0.2f;

      glm::vec3 position = {0.0f, 0.0f, 0.8f};

public:
      Camera(void) = default;
      ~Camera(void) = default;

      void reset(void) { position = {0.0f, 0.0f, 0.8f}; }

      glm::mat4 getViewMatrix(float aRatio);
      float getZoom(void) const { return position.z; }

      void moveFront(float elapsed);
      void moveBack(float elapsed);

      void moveUp(float elapsed);
      void moveDown(float elapsed);
      void moveLeft(float elapsed);
      void moveRight(float elapsed);
      void moveHorizontal(float value);
      void moveVertical(float value);
};
