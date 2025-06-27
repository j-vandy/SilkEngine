#include "Engine.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

const uint32_t WIDTH = 960;
const uint32_t HEIGHT = 580;
const char* APPLICATION_NAME = "Demo App";

const std::vector<silk::Vertex> vertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
};

silk::Engine engine(WIDTH, HEIGHT, APPLICATION_NAME);
silk::Scene scene;
silk::Entity camera = scene.createEntity(silk::Camera{glm::radians(45.0f), 0.1f, 10.0f}, silk::Transform{});
silk::Entity quad = scene.createEntity(silk::Mesh{vertices, indices}, silk::Transform{}, silk::Renderable{});
float duration = 0;

void onUpdate(float deltaTime)
{
    duration += deltaTime;

    const VkExtent2D& swapchainExtent = engine.getSwapchainExtent();
    silk::UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), duration * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), swapchainExtent.width / static_cast<float>(swapchainExtent.height), 0.1f, 10.0f);
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