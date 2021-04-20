#include "renderer/camera.h"

#include <glm/gtc/matrix_transform.hpp>

glm::mat4 Camera::getViewMatrix(void)
{
    glm::vec3 front(0.0, 0.0, -1.0), up(0.0, 1.0, 0.0);
    glm::mat4 proj = glm::perspective(1.5707963f, aspect, fNear, fFar);
    glm::mat4 view = glm::lookAt(position, position + front, up);

    return proj * view;
} // getViewMatrix

void Camera::moveFront(float elapsed)
{
    position.z -= speed * elapsed;
    position.z = position.z < maxZoom ? maxZoom : position.z;
}

void Camera::moveBack(float elapsed)
{
    position.z += speed * elapsed;
    position.z = position.z > minZoom ? minZoom : position.z;
}

void Camera::moveUp(float elapsed)
{
    position.y += speed * position.z * elapsed;
}
void Camera::moveDown(float elapsed)
{
    position.y -= speed * position.z * elapsed;
}
void Camera::moveLeft(float elapsed)
{
    position.x -= speed * position.z * elapsed;
}
void Camera::moveRight(float elapsed)
{
    position.x += speed * position.z * elapsed;
}

void Camera::moveHorizontal(float value)
{
    // slower the closer to image
    position.x -= sensitivity * position.z * value;
}

void Camera::moveVertical(float value)
{
    // faster the further from image
    position.y -= sensitivity * position.z * value;
}