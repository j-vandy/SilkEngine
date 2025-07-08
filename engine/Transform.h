#pragma once
#include <glm/mat4x4.hpp>

namespace silk
{
    class Transform
    {
    public:
        glm::vec2 getPosition() const;
        float getRotation() const;
        glm::vec2 getScale() const;
        glm::mat4 getMatrix() const;
        void setPosition(float x, float y);
        void setPosition(const glm::vec2& position);
        void setRotation(float rotation);
        void setScale(float x, float y);
        void setScale(const glm::vec2& scale);
        Transform(glm::vec2 position = glm::vec2(), float rotation = 0.0f, glm::vec2 scale = glm::vec2(1,1)) : pos(position), rot(rotation), s(scale) { updateMat(); }
    private:
        glm::vec2 pos;
        float rot;
        glm::vec2 s;
        glm::mat4 mat;
        void updateMat();
    };
}