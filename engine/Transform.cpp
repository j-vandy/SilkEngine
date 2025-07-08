#include "Transform.h"

#include <glm/gtc/matrix_transform.hpp>
#include <math.h>

namespace silk
{
    glm::vec2 Transform::getPosition() const { return pos; }

    float Transform::getRotation() const { return rot; }

    glm::vec2 Transform::getScale() const { return s; }

    glm::mat4 Transform::getMatrix() const { return mat; }

    void Transform::setPosition(float x, float y)
    {
        pos.x = x;
        pos.y = y;
        updateMat();
    }

    void Transform::setPosition(const glm::vec2& position) { setPosition(position.x, position.y); }

    void Transform::setRotation(float rotation)
    {
        rot = rotation;
        updateMat();
    }

    void Transform::setScale(float x, float y)
    {
        s.x = x;
        s.y = y;
        updateMat();
    }

    void Transform::setScale(const glm::vec2& scale) { setScale(scale.x, scale.y); }

    void Transform::updateMat()
    {
        glm::mat4 rotMatrix(
            cos(rot), sin(rot), 0.0f, 0.0f,
            -sin(rot), cos(rot), 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        );
        mat = glm::scale(glm::mat4(1.0f), glm::vec3(s.x, s.y, 1.0f)) * rotMatrix * glm::translate(glm::mat4(1.0f), glm::vec3(pos.x, pos.y, 0.0f));
    }
}