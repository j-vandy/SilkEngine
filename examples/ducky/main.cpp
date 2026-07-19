#include "silk/Engine.h"

#include <iostream>
#include <fstream>
#include <format>
#include <set>
#include <optional>
#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

bool framebufferResized = false;
void framebufferResizeCallback([[maybe_unused]] GLFWwindow* window, [[maybe_unused]] int width, [[maybe_unused]] int height)
{
    framebufferResized = true;
}

glm::vec3 sphericalToCartesian(float radius, float theta, float phi)
{
    float thetaRadians = glm::radians(theta);
    float phiRadians = glm::radians(phi);
    return glm::vec3(radius * sin(thetaRadians) * sin(phiRadians), radius * cos(phiRadians), radius * cos(thetaRadians) * sin(phiRadians));
}

const float SCROLL_SPEED = 10.0f;
float camRadius = 500.0f;
float theta = 0.0f, phi = 90.0f;
glm::vec3 cameraPosition(0.0f, 0.0f, camRadius);
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    camRadius -= yoffset * SCROLL_SPEED;
    cameraPosition = sphericalToCartesian(camRadius, theta, phi);
}

bool isLeftMouseButtonDown = false;
bool isRightMouseButtonDown = false;
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_1)
    {
        if (action == GLFW_PRESS)
        {
            isLeftMouseButtonDown = true;
        }
        else if (action == GLFW_RELEASE)
        {
            isLeftMouseButtonDown = false;
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_2)
    {
        if (action == GLFW_PRESS)
        {
            isRightMouseButtonDown = true;
        }
        else if (action == GLFW_RELEASE)
        {
            isRightMouseButtonDown = false;
        }
    }
}

float cursorDeltaX = 0.0f, cursorDeltaY = 0.0f, prevCursorX = 0.0f, prevCursorY = 0.0f;
void updateCursorDelta(GLFWwindow* window)
{
    double currCursorX, currCursorY;
    glfwGetCursorPos(window, &currCursorX, &currCursorY);
    cursorDeltaX = static_cast<float>(currCursorX) - prevCursorX;
    cursorDeltaY = static_cast<float>(currCursorY) - prevCursorY;
    prevCursorX = static_cast<float>(currCursorX);
    prevCursorY = static_cast<float>(currCursorY);
}

int main()
{
    const uint32_t WIDTH = 960;
    const uint32_t HEIGHT = 960;
    const char* APPLICATION_NAME = "Rubber Ducky";
    silk::WindowContext windowContext(WIDTH, HEIGHT, APPLICATION_NAME);

    // add callback
    glfwSetFramebufferSizeCallback(windowContext.getWindow(), framebufferResizeCallback);
    // glfwSetWindowUserPointer(windowContext.getWindow(), this);
    glfwSetScrollCallback(windowContext.getWindow(), scrollCallback);
    glfwSetMouseButtonCallback(windowContext.getWindow(), mouseButtonCallback);

    silk::DeviceContextCreateInfo deviceContextCreateInfo{};
    deviceContextCreateInfo.applicationName = APPLICATION_NAME;

    silk::DeviceContext deviceContext(windowContext.getWindow(), deviceContextCreateInfo);

    silk::RenderPassContext renderPassContext(deviceContext);

    silk::SwapchainContext swapchainContext(windowContext.getWindow(), deviceContext, renderPassContext.getRenderPass());

    // create DescriptorSetLayoutContext
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding samplerBinding{};
    samplerBinding.binding = 1;
    samplerBinding.descriptorCount = 1;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::vector<VkDescriptorSetLayoutBinding> bindings{ uboLayoutBinding, samplerBinding };

    silk::DescriptorSetLayoutContext descriptorSetLayoutContext(deviceContext.getDevice(), bindings);

    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv;

        static VkVertexInputBindingDescription getBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return bindingDescription;
        }

        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
        {
            return {
                { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) },
                { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) },
                { 2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv) },
            };
        }
    };

    struct ModelPC
    {
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 normal = glm::mat4(1.0f);
        
        static VkPushConstantRange getPushConstantRange()
        {
            return VkPushConstantRange
            {
                VK_SHADER_STAGE_VERTEX_BIT,
                0,
                static_cast<uint32_t>(sizeof(ModelPC))
            };
        }
    };

    silk::PipelineContextCreateInfo pipelineContextCreateInfo{};
    pipelineContextCreateInfo.descriptorSetLayouts = { descriptorSetLayoutContext.getDescriptorSetLayout() };
    pipelineContextCreateInfo.pushConstantRanges = { ModelPC::getPushConstantRange() };
    pipelineContextCreateInfo.vertexInputBindingDescriptions = { Vertex::getBindingDescription() };
    pipelineContextCreateInfo.vertexInputAttributeDescriptions = Vertex::getAttributeDescriptions();

    silk::PipelineContext pipelineContext(deviceContext.getDevice(), renderPassContext.getRenderPass(), pipelineContextCreateInfo);

    silk::CommandPoolContext commandPoolContext(deviceContext);

    // load Rubber Ducky gltf model
    const std::string FILENAME = ".\\model\\Duck.gltf";
    const tinygltf::Model model = silk::loadGLTFModel(FILENAME);

    // create vertex buffer
    const std::vector<glm::vec3> positions = silk::getGLTFModelPositions(model);
    const std::vector<glm::vec3> normals = silk::getGLTFModelNormals(model);
    const std::vector<glm::vec2> uvs = silk::getGLTFModelTexCoords(model);

    std::vector<Vertex> vertices(positions.size());
    for (size_t i = 0; i < positions.size(); i++)
    {
        vertices[i].position = positions[i];
        vertices[i].normal = normals[i];
        vertices[i].uv = uvs[i];
    }

    silk::DeviceLocalBufferContext<Vertex> vertexBufferContext(deviceContext, commandPoolContext.getCommandPool(), vertices, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    // create index buffer
    const std::vector<uint16_t> indices = silk::getGLTFModelIndices(model);
    silk::DeviceLocalBufferContext<uint16_t> indexBufferContext(deviceContext, commandPoolContext.getCommandPool(), indices, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    // create texture image
    tinygltf::Image tinyImage;
    {
        const auto material = model.materials[model.meshes[0].primitives[0].material];
        int textureIndex = material.pbrMetallicRoughness.baseColorTexture.index;
        const auto texture = model.textures[textureIndex];
        tinyImage = model.images[texture.source];
    }

    silk::DeviceLocalImageContext albedoTexContext(deviceContext, commandPoolContext.getCommandPool(), tinyImage);

    struct CameraUBO
    {
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    // create uniform buffer
    std::vector<silk::HostVisibleBufferContext<CameraUBO>> cameraUBOBufferContexts;
    const int MAX_FRAMES_IN_FLIGHT = 2;
    cameraUBOBufferContexts.reserve(MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < static_cast<size_t>(MAX_FRAMES_IN_FLIGHT); i++)
    {
        cameraUBOBufferContexts.emplace_back(deviceContext);
    }

    // create VkDescriptorPool
    VkDescriptorPoolSize uboDescriptorPoolSize{};
    uboDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboDescriptorPoolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolSize samplerDescriptorPoolSize{};
    samplerDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerDescriptorPoolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    std::vector<VkDescriptorPoolSize> poolSizes{ uboDescriptorPoolSize, samplerDescriptorPoolSize };

    silk::DescriptorPoolContext descriptorPoolContext(deviceContext.getDevice(), poolSizes, MAX_FRAMES_IN_FLIGHT);

    // create VkDescriptorSets
    std::vector<VkDescriptorSet> descriptorSets(MAX_FRAMES_IN_FLIGHT);
    {
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayoutContext.getDescriptorSetLayout());
        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
        descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocateInfo.descriptorPool = descriptorPoolContext.getDescriptorPool();
        descriptorSetAllocateInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        descriptorSetAllocateInfo.pSetLayouts = descriptorSetLayouts.data();

        VK_CHECK(vkAllocateDescriptorSets(deviceContext.getDevice(), &descriptorSetAllocateInfo, descriptorSets.data()));

        for (size_t i = 0; i < static_cast<size_t>(MAX_FRAMES_IN_FLIGHT); i++)
        {
            std::vector<VkDescriptorBufferInfo> descriptorBufferInfos{ cameraUBOBufferContexts[i].getVkDescriptorBufferInfos() };

            VkWriteDescriptorSet uboWriteDescriptorSet{};
            uboWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            uboWriteDescriptorSet.dstSet = descriptorSets[i];
            uboWriteDescriptorSet.dstBinding = 0;
            uboWriteDescriptorSet.dstArrayElement = 0;
            uboWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uboWriteDescriptorSet.descriptorCount = descriptorBufferInfos.size();
            uboWriteDescriptorSet.pBufferInfo = descriptorBufferInfos.data();

            VkDescriptorImageInfo imageInfo{};
            imageInfo.sampler = albedoTexContext.getSampler();
            imageInfo.imageView = albedoTexContext.getImageView();
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkWriteDescriptorSet imageWriteDescriptorSet{};
            imageWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            imageWriteDescriptorSet.dstSet = descriptorSets[i];
            imageWriteDescriptorSet.dstBinding = 1;
            imageWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            imageWriteDescriptorSet.descriptorCount = 1;
            imageWriteDescriptorSet.pImageInfo = &imageInfo;

            std::vector<VkWriteDescriptorSet> writeDescriptorSets{ uboWriteDescriptorSet, imageWriteDescriptorSet };

            vkUpdateDescriptorSets(deviceContext.getDevice(), writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
        }
    }

    // allocate VkCommandBuffer
    std::vector<VkCommandBuffer> commandBuffers(MAX_FRAMES_IN_FLIGHT);
    VK_CHECK(silk::allocateCommandBuffers(deviceContext.getDevice(), commandPoolContext.getCommandPool(), commandBuffers));

    // create synchronization objects
    std::vector<silk::SemaphoreContext> imageAvailableSemaphores;
    std::vector<silk::FenceContext> inFlightFences;
    imageAvailableSemaphores.reserve(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.reserve(MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        imageAvailableSemaphores.emplace_back(deviceContext.getDevice());
        inFlightFences.emplace_back(deviceContext.getDevice());
    }

    std::vector<silk::SemaphoreContext> renderFinishedSemaphores;
    const size_t swapchainImageCount = swapchainContext.getSwapchainImageCount();
    renderFinishedSemaphores.reserve(swapchainImageCount);
    for (size_t i = 0; i < swapchainImageCount; i++)
    {
        renderFinishedSemaphores.emplace_back(deviceContext.getDevice());
    }


    const float FOVY = 60.0f;
    const float Z_NEAR = 0.1f;
    const float Z_FAR = 10000.0f;
    const glm::vec3 VEC3_ORIGIN(0.0f);
    const glm::vec3 VEC3_UP(0.0f, 1.0f, 0.0f);
    const glm::vec3 VEC3_RIGHT(1.0f, 0.0f, 0.0f);
    const auto START_TIME = std::chrono::high_resolution_clock::now();
    const float ROT_SPEED = 0.5f;

    ModelPC modelPC{};
    float modelYaw = 0.0f, modelPitch = 0.0f;
    
    // run
    {
        VkDevice device = deviceContext.getDevice();

        const auto startTime = std::chrono::high_resolution_clock::now();
        auto previousTime = startTime;
        uint32_t currentFrame = 0;
        while(!glfwWindowShouldClose(windowContext.getWindow()))
        {
            glfwPollEvents();

            // update loop
            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float>(currentTime - previousTime).count();
            previousTime = currentTime;

            updateCursorDelta(windowContext.getWindow());

            // update UBO + push constant
            {
                CameraUBO cameraUBO{};

                float aspect = static_cast<float>(swapchainContext.getExtent().width) / static_cast<float>(swapchainContext.getExtent().height);
                cameraUBO.proj = glm::perspective(glm::radians(FOVY), aspect, Z_NEAR, Z_FAR);
                cameraUBO.proj[1][1] *= -1.0f; // vulkan clip space has inverted y-axis

                // rotate the camera
                if (isLeftMouseButtonDown)
                {
                    theta -= cursorDeltaX * ROT_SPEED;
                    phi = std::clamp(phi - cursorDeltaY * ROT_SPEED, 0.1f, 179.9f);

                    cameraPosition = sphericalToCartesian(camRadius, theta, phi);
                }
                cameraUBO.view = glm::lookAt(cameraPosition, VEC3_ORIGIN, VEC3_UP);

                cameraUBOBufferContexts[currentFrame].memcpy(&cameraUBO);

                // update ModelPC
                if (isRightMouseButtonDown)
                {
                    modelYaw += cursorDeltaX * ROT_SPEED;
                    modelPitch += cursorDeltaY * ROT_SPEED;
                    modelPC.model = glm::mat4(1.0f);
                    modelPC.model = glm::rotate(modelPC.model, glm::radians(modelYaw), VEC3_UP);
                    modelPC.model = glm::rotate(modelPC.model, glm::radians(modelPitch), VEC3_RIGHT);
                }
                modelPC.normal = glm::transpose(glm::inverse(cameraUBO.view * modelPC.model));
            }

            // draw frame
            {
                VkFence fence = inFlightFences[currentFrame].getFence();
                vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

                uint32_t imageIndex;
                VkResult result = vkAcquireNextImageKHR(device, swapchainContext.getSwapchain(), UINT64_MAX, imageAvailableSemaphores[currentFrame].getSemaphore(), VK_NULL_HANDLE, &imageIndex);

                if (result == VK_ERROR_OUT_OF_DATE_KHR)
                {
                    swapchainContext.recreate(windowContext.getWindow(), deviceContext, renderPassContext.getRenderPass());
                    continue;
                }
                else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
                {
                    throw std::runtime_error("Error: failed to aquire next swapchain image!");
                }

                vkResetFences(device, 1, &fence);

                vkResetCommandBuffer(commandBuffers[currentFrame], 0);

                // record command buffer
                VkCommandBufferBeginInfo commandBufferBeginInfo{};
                commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

                VK_CHECK(vkBeginCommandBuffer(commandBuffers[currentFrame], &commandBufferBeginInfo));

                VkRenderPassBeginInfo renderPassBeginInfo{};
                renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassBeginInfo.renderPass = renderPassContext.getRenderPass();
                renderPassBeginInfo.framebuffer = swapchainContext.getFramebuffers()[imageIndex];
                renderPassBeginInfo.renderArea.offset = {0, 0};
                renderPassBeginInfo.renderArea.extent = swapchainContext.getExtent();

                std::vector<VkClearValue> clearValues(2);
                clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
                clearValues[1].depthStencil = { 1.0f, 0 };

                renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
                renderPassBeginInfo.pClearValues = clearValues.data();

                vkCmdBeginRenderPass(commandBuffers[currentFrame], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

                    vkCmdBindPipeline(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineContext.getPipeline());

                    VkViewport viewport{};
                    viewport.x = 0.0f;
                    viewport.y = 0.0f;
                    viewport.width = static_cast<float>(swapchainContext.getExtent().width);
                    viewport.height = static_cast<float>(swapchainContext.getExtent().height);
                    viewport.minDepth = 0.0f;
                    viewport.maxDepth = 1.0f;
                    vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &viewport);

                    VkRect2D scissor{};
                    scissor.offset = {0, 0};
                    scissor.extent = swapchainContext.getExtent();
                    vkCmdSetScissor(commandBuffers[currentFrame], 0, 1, &scissor);

                    VkBuffer vertexBuffers[] = { vertexBufferContext.getBuffer() };
                    VkDeviceSize offsets[] = { 0 };
                    vkCmdBindVertexBuffers(commandBuffers[currentFrame], 0, 1, vertexBuffers, offsets);

                    vkCmdBindIndexBuffer(commandBuffers[currentFrame], indexBufferContext.getBuffer(), 0, VK_INDEX_TYPE_UINT16);

                    vkCmdBindDescriptorSets(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineContext.getPipelineLayout(), 0, 1, &descriptorSets[currentFrame], 0, nullptr);

                    vkCmdPushConstants(commandBuffers[currentFrame], pipelineContext.getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ModelPC), &modelPC);

                    vkCmdDrawIndexed(commandBuffers[currentFrame], indices.size(), 1, 0, 0, 0);

                vkCmdEndRenderPass(commandBuffers[currentFrame]);

                VK_CHECK(vkEndCommandBuffer(commandBuffers[currentFrame]));

                VkSubmitInfo submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

                VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame].getSemaphore() };
                VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
                submitInfo.waitSemaphoreCount = 1;
                submitInfo.pWaitSemaphores = waitSemaphores;
                submitInfo.pWaitDstStageMask = waitStages;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

                VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[imageIndex].getSemaphore() };
                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = signalSemaphores;

                VK_CHECK(vkQueueSubmit(deviceContext.getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame].getFence()));

                VkPresentInfoKHR presentInfo{};
                presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
                presentInfo.waitSemaphoreCount = 1;
                presentInfo.pWaitSemaphores = signalSemaphores;

                VkSwapchainKHR swapchains[] = { swapchainContext.getSwapchain() };
                presentInfo.swapchainCount = 1;
                presentInfo.pSwapchains = swapchains;
                presentInfo.pImageIndices = &imageIndex;

                result = vkQueuePresentKHR(deviceContext.getPresentQueue(), &presentInfo);
                if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
                {
                    framebufferResized = false;
                    swapchainContext.recreate(windowContext.getWindow(), deviceContext, renderPassContext.getRenderPass());
                }
                else if (result != VK_SUCCESS)
                {
                    throw std::runtime_error("Error: failed to present swapchain image!");
                }

                currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
            }
        }
    }

    return EXIT_SUCCESS;
}