#pragma once

#include <glm/glm.hpp>

class Camera
{
private:
      float
          fNear = 0.001f,
          fFar = 1000.0f,
          maxZoom = 0.05,
          minZoom = 1.0f,
          speed = 1.0f,
          sensitivity = 0.2f,
          aspect = 1.0f;

public:
      Camera(void) = default;
      ~Camera(void) = default;
      glm::vec3 position = {0.0f, 0.0f, 0.8f};

      void reset(void) { position = {0.0f, 0.0f, 0.8f}; }

      glm::mat4 getViewMatrix(void);
      float getZoom(void) const { return position.z; }

      void setAspectRatio(float value) { aspect = value; }

      void moveFront(float elapsed);
      void moveBack(float elapsed);

      void moveUp(float elapsed);
      void moveDown(float elapsed);
      void moveLeft(float elapsed);
      void moveRight(float elapsed);
      void moveHorizontal(float value);
      void moveVertical(float value);
};
