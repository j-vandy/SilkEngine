#include "Engine.h"
#include "Transform.h"

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

const uint32_t WIDTH = 960;
const uint32_t HEIGHT = 960;
const char* APPLICATION_NAME = "Quad App";
const float FOV_Y = 2.0f;
const float CAMERA_SPEED = 1.0f;

silk::Engine engine(WIDTH, HEIGHT, APPLICATION_NAME);
silk::Scene scene;
silk::Entity cam = scene.createEntity(silk::Camera{FOV_Y}, silk::Transform{});
glm::vec2 dirInput(0.0f);

const glm::vec4 BLACK(0.0f);
const glm::vec4 RED(1.0f, 0.0f, 0.0f, 1.0f);
const glm::vec4 GREEN(0.0f, 1.0f, 0.0f, 1.0f);
const glm::vec4 BLUE(0.0f, 0.0f, 1.0f, 1.0f);
const glm::vec4 WHITE(1.0f, 1.0f, 1.0f, 1.0f);


glm::vec4 tint = WHITE;
void keyCallback(GLFWwindow* window __attribute__((unused)), int key, int scancode __attribute__((unused)), int action, int mods __attribute__((unused)))
{
    if (action == GLFW_REPEAT)
    {
        return;
    }

    // reverse input if action is released
    float isPress = action == GLFW_PRESS ? 1.0f : -1.0f;

    switch (key)
    {
        case GLFW_KEY_W:
            dirInput.y += 1 * isPress;
            break;
        case GLFW_KEY_A:
            dirInput.x -= 1 * isPress;
            break;
        case GLFW_KEY_S:
            dirInput.y -= 1 * isPress;
            break;
        case GLFW_KEY_D:
            dirInput.x += 1 * isPress;
            break;
        case GLFW_KEY_0:
            tint = BLACK;
            break;
        case GLFW_KEY_1:
            tint = RED;
            break;
        case GLFW_KEY_2:
            tint = GREEN;
            break;
        case GLFW_KEY_3:
            tint = BLUE;
            break;
        case GLFW_KEY_4:
            tint = WHITE;
            break;
    }
}

float mouseButton1IsDown = false;
float press_x, press_y;
silk::Entity currQuad;
void mouseButtonCallback(GLFWwindow* window __attribute__((unused)), int button, int action, int mods __attribute__((unused)))
{
    if (button == GLFW_MOUSE_BUTTON_1)
    {
        if (action == GLFW_PRESS)
        {
            mouseButton1IsDown = true;
            engine.getCursorWorldSpace(scene, cam, &press_x, &press_y);
            currQuad = scene.createEntity(silk::Quad{}, silk::Transform{}, silk::Tint{tint});
        }
        else if (action == GLFW_RELEASE)
        {
            mouseButton1IsDown = false;
        }
    }
}

void onUpdate(float deltaTime)
{
    // update quad position
    if (mouseButton1IsDown)
    {
        silk::Transform& quadTransform = scene.getComponent<silk::Transform>(currQuad);

        float x,y;
        engine.getCursorWorldSpace(scene, cam, &x, &y);

        glm::vec2 pressToCurr = glm::vec2((x - press_x), (y - press_y));
        quadTransform.setScale(pressToCurr);
        quadTransform.setPosition((x + press_x)/2.0f, (y + press_y)/2.0f);
    }

    // update camera pos + ubo
    silk::Transform& camTransform = scene.getComponent<silk::Transform>(cam);
    glm::vec2 dist(0.0f);
    if (glm::length(dirInput) != 0.0f)
    {
        dist = glm::normalize(dirInput) * CAMERA_SPEED * deltaTime;
    }
    camTransform.setPosition(camTransform.getPosition() + dist);

    silk::CameraUBO ubo{};
    ubo.view = glm::inverse(camTransform.getMatrix());

    const VkExtent2D& swapchainExtent = engine.getSwapchainExtent();
    ubo.proj = scene.getComponent<silk::Camera>(cam).getOrthoMatrix(swapchainExtent.width, swapchainExtent.height);
    ubo.proj[1][1] *= -1;

    engine.memcpyCameraUBO(ubo);

    // load instance info
    std::vector<silk::InstanceData> instanceDatas;
    for(silk::Entity e : scene.query<silk::Quad, silk::Transform>())
    {
        silk::InstanceData i;
        i.model = scene.getComponent<silk::Transform>(e).getMatrix();
        i.tint = scene.getComponent<silk::Tint>(e).color;
        instanceDatas.push_back(i);
    }
    engine.updateInstanceBuffer(instanceDatas);
}

int main()
{
    engine.setKeyCallback(keyCallback);
    engine.setMouseButtonCallback(mouseButtonCallback);
    engine.updateCallbacks.push_back(onUpdate);
    engine.run();

    // TODO:
    // - Radiance cascade basics
    // - Textured quads
    // - Separate light map

    return EXIT_SUCCESS;
}