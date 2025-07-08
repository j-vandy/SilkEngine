#include "Engine.h"
#include "Transform.h"

#include <glm/gtc/matrix_transform.hpp>

const uint32_t WIDTH = 960;
const uint32_t HEIGHT = 580;
const char* APPLICATION_NAME = "Demo App";

const std::vector<silk::Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
};

silk::Engine engine(WIDTH, HEIGHT, APPLICATION_NAME);
silk::Scene scene;
silk::Entity cam = scene.createEntity(silk::Camera{2.0f}, silk::Transform{});
silk::Entity quad = scene.createEntity(silk::Mesh{vertices, indices}, silk::Transform{}, silk::Renderable{});
float duration = 0;

void onUpdate(float deltaTime)
{
    duration += deltaTime;

    silk::UniformBufferObject ubo{};

    silk::Transform quadTransform = scene.getComponent<silk::Transform>(quad);
    quadTransform.setRotation(duration * glm::radians(90.0f));
    ubo.model = quadTransform.getMatrix();

    ubo.view = scene.getComponent<silk::Transform>(cam).getMatrix();

    const VkExtent2D& swapchainExtent = engine.getSwapchainExtent();
    ubo.proj = scene.getComponent<silk::Camera>(cam).getOrthoMatrix(swapchainExtent.width, swapchainExtent.height);
    ubo.proj[1][1] *= -1;

    engine.memcpyUBO(ubo);
}

int main()
{
    engine.updateCallbacks.push_back(onUpdate);
    engine.loadScene(scene);
    engine.run();

    return EXIT_SUCCESS;
}