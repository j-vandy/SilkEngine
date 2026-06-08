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

// TODO
// Only create API functions or data structures whenever there's Vulkan obj reconstructions or code duplication across examples
// - Duck Model Viewer Example (6/7)
//      - Add texturing (6/3)
//      - Blinn-Phong shading (6/4)
//      - Adjustable specular value (6/4)
// - Check for depricated code on Vulkan website (6/10)
// - 2D paint (6/21)
//      - Get previous & current frame's mouse position (6/11)
//      - Draw capsule (6/14)
//      - Control diameter with +/- (6/16)
//      - Swap color with numbers (6/18)
//      - Improve API
//          - Creating different kinds of buffers
//          - Setting buffer/shader uniform values
// - flatland RC (7/13)
// - bilinear fix or holographic RC
// - Improve API
// - screen space RC, Input System, "Phox" Engine, ...

int main()
{
    // create glfw window
    const uint32_t WIDTH = 960;
    const uint32_t HEIGHT = 960;
    const char* APPLICATION_NAME = "Rubber Ducky";
    GLFWwindow* window;
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(WIDTH, HEIGHT, APPLICATION_NAME, nullptr, nullptr);
        // TODO
        // glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    // added callbacks
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    silk::DeviceContextCreateInfo deviceContextCreateInfo{};
    deviceContextCreateInfo.applicationName = APPLICATION_NAME;

    silk::DeviceContext deviceContext(window, deviceContextCreateInfo);

    // create VkRenderPass
    VkRenderPass renderPass;
    {
        VkSurfaceFormatKHR surfaceFormat = silk::getPhysicalDeviceSurfaceFormat(deviceContext.getPhysicalDevice(), deviceContext.getSurface());

        VkAttachmentDescription colorAttachmentDescription{};
        colorAttachmentDescription.format = surfaceFormat.format;
        colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentReference{};
        colorAttachmentReference.attachment = 0;
        colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription depthAttachmentDescription{};
        depthAttachmentDescription.flags = 0;
        depthAttachmentDescription.format = silk::getDepthFormat(deviceContext.getPhysicalDevice());
        depthAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentReference{};
        depthAttachmentReference.attachment = 1;
        depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentReference;
        subpass.pDepthStencilAttachment = &depthAttachmentReference;

        std::vector<VkAttachmentDescription> attachmentDescriptions{ colorAttachmentDescription, depthAttachmentDescription };

        VkSubpassDependency colorDependency{};
        colorDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        colorDependency.dstSubpass = 0;
        colorDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        colorDependency.srcAccessMask = 0;
        colorDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        colorDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkSubpassDependency depthDependency{};
        depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        depthDependency.dstSubpass = 0;
        depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        depthDependency.srcAccessMask = 0;
        depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::vector<VkSubpassDependency> subpassDependencies{ colorDependency, depthDependency };

        VkRenderPassCreateInfo renderPassCreateInfo{};
        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.attachmentCount = attachmentDescriptions.size();
        renderPassCreateInfo.pAttachments = attachmentDescriptions.data();
        renderPassCreateInfo.subpassCount = 1;
        renderPassCreateInfo.pSubpasses = &subpass;
        renderPassCreateInfo.dependencyCount = subpassDependencies.size();
        renderPassCreateInfo.pDependencies = subpassDependencies.data();

        VK_CHECK(vkCreateRenderPass(deviceContext.getDevice(), &renderPassCreateInfo, nullptr, &renderPass));
    }

    // create SwapchainContext
    silk::SwapchainContext swapchainContext(window, deviceContext, renderPass);

    // create VkDescriptorSetLayout
    VkDescriptorSetLayout descriptorSetLayout;
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
        descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCreateInfo.bindingCount = 1;
        descriptorSetLayoutCreateInfo.pBindings = &uboLayoutBinding;

        VK_CHECK(vkCreateDescriptorSetLayout(deviceContext.getDevice(), &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout));
    }

    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;

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
            };
        }
    };

    struct ModelPC
    {
        glm::mat4 model = glm::mat4(1.0f);;
        glm::mat4 normal = glm::mat4(1.0f);;

        static VkShaderStageFlags getStageFlags() { return VK_SHADER_STAGE_VERTEX_BIT; }
    };

    using VertexInputPack = std::tuple<Vertex>;
    using PushConstantPack = std::tuple<ModelPC>;
    auto pipelineContextCreateInfo = silk::PipelineContextCreateInfo::build<VertexInputPack, PushConstantPack>({ descriptorSetLayout });

    silk::PipelineContext pipelineContext(deviceContext.getDevice(), renderPass, pipelineContextCreateInfo);

    // create VkCommandPool
    VkCommandPool commandPool;
    {
        VkCommandPoolCreateInfo commandPoolCreateInfo{};
        commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolCreateInfo.queueFamilyIndex = deviceContext.getGraphicsQueueFamilyIndex();

        VK_CHECK(vkCreateCommandPool(deviceContext.getDevice(), &commandPoolCreateInfo, nullptr, &commandPool));
    }

    // load Rubber Ducky gltf model
    const std::string FILENAME = ".\\model\\Duck.gltf";
    const tinygltf::Model model = silk::loadGLTFModel(FILENAME);

    const std::vector<glm::vec3> positions = silk::getGLTFModelPositions(model);
    const std::vector<glm::vec3> normals = silk::getGLTFModelNormals(model);

    std::vector<Vertex> vertices(positions.size());
    for (size_t i = 0; i < positions.size(); i++)
    {
        vertices[i].position = positions[i];
        vertices[i].normal = normals[i];
    }

    // TODO
    // - make it easy to create different types of buffers
    // create (vertex) VkBuffer
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    {
        VkPhysicalDevice physicalDevice = deviceContext.getPhysicalDevice();
        VkDevice device = deviceContext.getDevice();
        VkDeviceSize vertexBufferSize = sizeof(Vertex) * vertices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        VK_CHECK(silk::createBuffer(physicalDevice, device, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory));

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, vertexBufferSize, 0, &data);
            memcpy(data, vertices.data(), static_cast<size_t>(vertexBufferSize));
        vkUnmapMemory(device, stagingBufferMemory);

        VK_CHECK(silk::createBuffer(physicalDevice, device, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory));

        VK_CHECK(silk::copyBuffer(device, deviceContext.getGraphicsQueue(), commandPool, stagingBuffer, vertexBuffer, vertexBufferSize));

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    const std::vector<uint16_t> indices = silk::getGLTFModelIndices(model);

    // create (index) VkBuffer
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    {
        VkPhysicalDevice physicalDevice = deviceContext.getPhysicalDevice();
        VkDevice device = deviceContext.getDevice();
        VkDeviceSize indexBufferSize = sizeof(uint16_t) * indices.size(); 

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        VK_CHECK(silk::createBuffer(physicalDevice, device, indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory));

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, indexBufferSize, 0, &data);
            memcpy(data, indices.data(), static_cast<size_t>(indexBufferSize));
        vkUnmapMemory(device, stagingBufferMemory);

        VK_CHECK(silk::createBuffer(physicalDevice, device, indexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory));
        VK_CHECK(silk::copyBuffer(device, deviceContext.getGraphicsQueue(), commandPool, stagingBuffer, indexBuffer, indexBufferSize));

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    // create (instance) VkBuffer
    const int MAX_FRAMES_IN_FLIGHT = 2;
    // uint32_t maxInstances = 100;
    // std::vector<uint32_t> instanceCounts;
    // std::vector<VkBuffer> instanceBuffers;
    // std::vector<VkDeviceMemory> instanceBuffersMemory;
    // std::vector<void*> instanceBuffersMapped;
    // {
    //     instanceCounts.resize(MAX_FRAMES_IN_FLIGHT);
    //     instanceBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    //     instanceBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    //     instanceBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    //     VkDeviceSize instanceBufferSize = sizeof(silk::InstanceData) * maxInstances;
    //     for (size_t i = 0; i < static_cast<size_t>(MAX_FRAMES_IN_FLIGHT); i++)
    //     {
    //         VK_CHECK(silk::createBuffer(physicalDevice, device, instanceBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, instanceBuffers[i], instanceBuffersMemory[i]));
    //         VK_CHECK(vkMapMemory(device, instanceBuffersMemory[i], 0, instanceBufferSize, 0, &instanceBuffersMapped[i]));
    //     }
    // }

    struct CameraUBO
    {
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    // create (uniform) VkBuffer
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;
    {
        uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        VkPhysicalDevice physicalDevice = deviceContext.getPhysicalDevice();
        VkDevice device = deviceContext.getDevice();
        VkDeviceSize bufferSize = sizeof(CameraUBO);
        for (size_t i = 0; i < static_cast<size_t>(MAX_FRAMES_IN_FLIGHT); i++)
        {
            VK_CHECK(silk::createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]));
            VK_CHECK(vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]));
        }
    }

    // create VkDescriptorPool
    VkDescriptorPool descriptorPool;
    {
        VkDescriptorPoolSize descriptorPoolSize{};
        descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorPoolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
        descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolCreateInfo.poolSizeCount = 1;
        descriptorPoolCreateInfo.pPoolSizes = &descriptorPoolSize;
        descriptorPoolCreateInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VK_CHECK(vkCreateDescriptorPool(deviceContext.getDevice(), &descriptorPoolCreateInfo, nullptr, &descriptorPool));
    }

    // create VkDescriptorSets
    std::vector<VkDescriptorSet> descriptorSets;
    {
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
        descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocateInfo.descriptorPool = descriptorPool;
        descriptorSetAllocateInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        descriptorSetAllocateInfo.pSetLayouts = descriptorSetLayouts.data();

        descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        VK_CHECK(vkAllocateDescriptorSets(deviceContext.getDevice(), &descriptorSetAllocateInfo, descriptorSets.data()));

        for (size_t i = 0; i < static_cast<size_t>(MAX_FRAMES_IN_FLIGHT); i++)
        {
            VkDescriptorBufferInfo descriptorBufferInfo{};
            descriptorBufferInfo.buffer = uniformBuffers[i];
            descriptorBufferInfo.offset = 0;
            descriptorBufferInfo.range = sizeof(CameraUBO);

            VkWriteDescriptorSet writeDescriptorSet{};
            writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSet.dstSet = descriptorSets[i];
            writeDescriptorSet.dstBinding = 0;
            writeDescriptorSet.dstArrayElement = 0;
            writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writeDescriptorSet.descriptorCount = 1;
            writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;

            vkUpdateDescriptorSets(deviceContext.getDevice(), 1, &writeDescriptorSet, 0, nullptr);
        }
    }

    // allocate VkCommandBuffer
    std::vector<VkCommandBuffer> commandBuffers;
    {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = commandPool;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

        VK_CHECK(vkAllocateCommandBuffers(deviceContext.getDevice(), &commandBufferAllocateInfo, commandBuffers.data()));
    }

    // create synchronization objects
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(swapchainContext.getSwapchainImageCount());

        VkSemaphoreCreateInfo semaphoreCreateInfo{};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceCreateInfo{};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VkDevice device = deviceContext.getDevice();
        for (size_t i = 0; i < static_cast<size_t>(MAX_FRAMES_IN_FLIGHT); i++)
        {
            VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphores[i]));
            VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &inFlightFences[i]));
        }

        for (size_t i = 0; i < renderFinishedSemaphores.size(); i++)
        {
            VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphores[i]));
        }
    }

    const float FOVY = 60.0f;
    const float Z_NEAR = 0.1f;
    const float Z_FAR = 10000.0f;
    const glm::vec3 CENTER(0.0f);
    const glm::vec3 UP(0.0f, 1.0f, 0.0f);
    const auto START_TIME = std::chrono::high_resolution_clock::now();
    const float ROT_SPEED = 0.5f;

    ModelPC modelPC{};
    float modelYaw = 0.0f, modelPitch = 0.0f;
    const glm::vec3 VEC3_UP(0.0f, 1.0f, 0.0f);
    const glm::vec3 VEC3_RIGHT(1.0f, 0.0f, 0.0f);

    // run
    {
        VkDevice device = deviceContext.getDevice();

        const auto startTime = std::chrono::high_resolution_clock::now();
        auto previousTime = startTime;
        uint32_t currentFrame = 0;
        while(!glfwWindowShouldClose(window))
        {
            glfwPollEvents();

            // update loop
            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float>(currentTime - previousTime).count();
            previousTime = currentTime;
            // for (std::function<void(float)> fn : updateCallbacks)
            // {
            //     fn(deltaTime);
            // }

            updateCursorDelta(window);

            // update camera UBO
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
                cameraUBO.view = glm::lookAt(cameraPosition, CENTER, UP);

                memcpy(uniformBuffersMapped[currentFrame], &cameraUBO, sizeof(CameraUBO));
            }

            // update ModelPC
            if (isRightMouseButtonDown)
            {
                modelYaw += cursorDeltaX * ROT_SPEED;
                modelPitch += cursorDeltaY * ROT_SPEED;
                modelPC.model = glm::mat4(1.0f);
                modelPC.model = glm::rotate(modelPC.model, glm::radians(modelYaw), VEC3_UP);
                modelPC.model = glm::rotate(modelPC.model, glm::radians(modelPitch), VEC3_RIGHT);
                modelPC.normal = glm::transpose(glm::inverse(modelPC.model));
            }

            // draw frame
            {
                vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

                uint32_t imageIndex;
                VkResult result = vkAcquireNextImageKHR(device, swapchainContext.getSwapchain(), UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

                if (result == VK_ERROR_OUT_OF_DATE_KHR)
                {
                    swapchainContext.recreate(window, deviceContext, renderPass);
                    continue;
                }
                else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
                {
                    throw std::runtime_error("Error: failed to aquire next swapchain image!");
                }

                vkResetFences(device, 1, &inFlightFences[currentFrame]);

                vkResetCommandBuffer(commandBuffers[currentFrame], 0);

                // record command buffer
                VkCommandBufferBeginInfo commandBufferBeginInfo{};
                commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

                VK_CHECK(vkBeginCommandBuffer(commandBuffers[currentFrame], &commandBufferBeginInfo));

                VkRenderPassBeginInfo renderPassBeginInfo{};
                renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassBeginInfo.renderPass = renderPass;
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

                    VkBuffer vertexBuffers[] = { vertexBuffer };
                    VkDeviceSize offsets[] = { 0 };
                    vkCmdBindVertexBuffers(commandBuffers[currentFrame], 0, 1, vertexBuffers, offsets);

                    vkCmdBindIndexBuffer(commandBuffers[currentFrame], indexBuffer, 0, VK_INDEX_TYPE_UINT16);

                    vkCmdBindDescriptorSets(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineContext.getPipelineLayout(), 0, 1, &descriptorSets[currentFrame], 0, nullptr);

                    vkCmdPushConstants(commandBuffers[currentFrame], pipelineContext.getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ModelPC), &modelPC);

                    vkCmdDrawIndexed(commandBuffers[currentFrame], indices.size(), 1, 0, 0, 0);

                vkCmdEndRenderPass(commandBuffers[currentFrame]);

                VK_CHECK(vkEndCommandBuffer(commandBuffers[currentFrame]));

                VkSubmitInfo submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

                VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
                VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
                submitInfo.waitSemaphoreCount = 1;
                submitInfo.pWaitSemaphores = waitSemaphores;
                submitInfo.pWaitDstStageMask = waitStages;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

                VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[imageIndex] };
                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = signalSemaphores;

                VK_CHECK(vkQueueSubmit(deviceContext.getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]));

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
                    swapchainContext.recreate(window, deviceContext, renderPass);
                }
                else if (result != VK_SUCCESS)
                {
                    throw std::runtime_error("Error: failed to present swapchain image!");
                }

                currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
            }
        }
    }

    VkDevice device = deviceContext.getDevice();
    vkDeviceWaitIdle(device);

    // destroy synchronization objects
    for (size_t i = 0; i < renderFinishedSemaphores.size(); i++)
    {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
    }
    for (size_t i = 0; i < static_cast<size_t>(MAX_FRAMES_IN_FLIGHT); i++)
    {
        vkDestroyFence(device, inFlightFences[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
    }

    // destroy VkDescriptorPool
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);

    // destroy (uniform) VkBuffer
    for (size_t i = 0; i < static_cast<size_t>(MAX_FRAMES_IN_FLIGHT); i++)
    {
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
    }

    // destroy (instance) VkBuffer
    // for (size_t i = 0; i < static_cast<size_t>(MAX_FRAMES_IN_FLIGHT); i++)
    // {
    //     vkFreeMemory(device, instanceBuffersMemory[i], nullptr);
    //     vkDestroyBuffer(device, instanceBuffers[i], nullptr);
    // }

    // destroy (index) VkBuffer
    vkFreeMemory(device, indexBufferMemory, nullptr);
    vkDestroyBuffer(device, indexBuffer, nullptr);

    // destroy (vertex) VkBuffer
    vkFreeMemory(device, vertexBufferMemory, nullptr);
    vkDestroyBuffer(device, vertexBuffer, nullptr);

    // destroy VkCommandPool
    vkDestroyCommandPool(device, commandPool, nullptr);

    // destroy VkDescriptorSetLayout
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

    // destroy renderPass
    vkDestroyRenderPass(device, renderPass, nullptr);

    // destroy glfw window
    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}