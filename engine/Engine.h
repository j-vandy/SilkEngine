#pragma once

#include "ECS.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>

#include <vector>
#include <array>
#include <functional>
#include <iostream>

namespace silk
{
    void validateVkResult(VkResult result, const char* msg);

    VkSurfaceFormatKHR getPhysicalDeviceSurfaceFormat(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface);

    std::vector<char> readFile(const std::string& filename);

    VkResult createVkShaderModule(const VkDevice& device, VkShaderModule& shaderModule, const std::vector<char>& code);

    struct DeviceContextCreateInfo
    {
        const char* applicationName;
        bool enableValidationLayers = true;
        std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
        const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    };

    class DeviceContext
    {
    public:
        DeviceContext(GLFWwindow* window, const DeviceContextCreateInfo& createInfo);
        ~DeviceContext();
        VkSurfaceKHR getSurface() const;
        VkPhysicalDevice getPhysicalDevice() const;
        VkDevice getDevice() const;
        VkQueue getGraphicsQueue() const;
        uint32_t getGraphicsQueueFamilyIndex() const;
        VkQueue getPresentQueue() const;
        uint32_t getPresentQueueFamilyIndex() const;
    private:
        bool enableValidationLayers;
        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkSurfaceKHR surface;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device;
        VkQueue graphicsQueue;
        uint32_t graphicsQueueFamilyIndex;
        VkQueue presentQueue;
        uint32_t presentQueueFamilyIndex;
    };

    class SwapchainContext
    {
    public:
        SwapchainContext(GLFWwindow* window, const DeviceContext& deviceContext, VkRenderPass renderPass);
        ~SwapchainContext();
        void recreate(GLFWwindow* window, const silk::DeviceContext& deviceContext, VkRenderPass renderPass);
        const VkExtent2D& getExtent() const;
        VkSwapchainKHR getSwapchain() const;
        const std::vector<VkFramebuffer>& getFramebuffers() const;
    private:
        VkDevice device;
        VkExtent2D extent;
        VkSwapchainKHR swapchain;
        std::vector<VkImageView> imageViews;
        std::vector<VkFramebuffer> framebuffers;
        void create(GLFWwindow* window, const DeviceContext& deviceContext, VkRenderPass renderPass);
        void destroy();
    };

    template <typename T>
    concept VertexInput = requires {
        { T::getBindingDescription() } -> std::same_as<VkVertexInputBindingDescription>;
        { T::getAttributeDescriptions() } -> std::same_as<std::vector<VkVertexInputAttributeDescription>>;
    };

    class PipelineContext
    {
    public:
        template <VertexInput... Ts>
            requires (sizeof...(Ts) > 0)
        static PipelineContext create(VkDevice device, VkRenderPass renderPass)
        {
            // relative to binary dir
            std::vector<char> vertShaderCode = readFile("shaders/vert.spv");
            std::vector<char> fragShaderCode = readFile("shaders/frag.spv");

            VkShaderModule vertShaderModule;
            validateVkResult(createVkShaderModule(device, vertShaderModule, vertShaderCode), "Error: failed to create fragment VkShaderModule!");

            VkShaderModule fragShaderModule;
            validateVkResult(createVkShaderModule(device, fragShaderModule, fragShaderCode), "Error: failed to create fragment VkShaderModule!");

            VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo{};
            vertShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vertShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
            vertShaderStageCreateInfo.module = vertShaderModule;
            vertShaderStageCreateInfo.pName = "main";

            VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo{};
            fragShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            fragShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            fragShaderStageCreateInfo.module = fragShaderModule;
            fragShaderStageCreateInfo.pName = "main";

            VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageCreateInfo, fragShaderStageCreateInfo};

            VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
            vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

            std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions;
            std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;
            (
                [&vertexBindingDescriptions, &vertexAttributeDescriptions]() {
                    vertexBindingDescriptions.push_back(Ts::getBindingDescription());
                    auto attributeDescriptions = Ts::getAttributeDescriptions();
                    vertexAttributeDescriptions.insert(vertexAttributeDescriptions.end(), attributeDescriptions.begin(), attributeDescriptions.end());
                }(),
                ...
            );
            if (vertexBindingDescriptions.size() <= 0 || vertexAttributeDescriptions.size() <= 0)
            {
                throw std::runtime_error("Error: failed to get VkVertexInputBindingDescriptions or VkVertexInputAttributeDescription!");
            }

            vertexInputCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindingDescriptions.size());
            vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescriptions.size());
            vertexInputCreateInfo.pVertexBindingDescriptions = vertexBindingDescriptions.data();
            vertexInputCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();

            VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
            inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

            VkPipelineViewportStateCreateInfo viewportCreateInfo{};
            viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportCreateInfo.viewportCount = 1;
            viewportCreateInfo.scissorCount = 1;

            // TODO
            // - Cull mode support: rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;

            VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo{};
            rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizationCreateInfo.depthClampEnable = VK_FALSE;
            rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
            rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
            rasterizationCreateInfo.lineWidth = 1.0f;
            rasterizationCreateInfo.cullMode = VK_CULL_MODE_NONE;
            rasterizationCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            rasterizationCreateInfo.depthBiasEnable = VK_FALSE;

            VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo{};
            multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;
            multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

            VkPipelineColorBlendAttachmentState colorBlendAttachment{};
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = VK_FALSE;

            VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo{};
            colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlendCreateInfo.logicOpEnable = VK_FALSE;
            colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
            colorBlendCreateInfo.attachmentCount = 1;
            colorBlendCreateInfo.pAttachments = &colorBlendAttachment;
            colorBlendCreateInfo.blendConstants[0] = 0.0f;
            colorBlendCreateInfo.blendConstants[1] = 0.0f;
            colorBlendCreateInfo.blendConstants[2] = 0.0f;
            colorBlendCreateInfo.blendConstants[3] = 0.0f;

            std::vector<VkDynamicState> dynamicStates = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR};
            VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
            dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
            dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

            VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
            pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutCreateInfo.setLayoutCount = 0;
            // TODO
            // pipelineLayoutCreateInfo.setLayoutCount = 1;
            // pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
            pipelineLayoutCreateInfo.pushConstantRangeCount = 0;

            PipelineContext pipelineContext;
            pipelineContext.device = device;

            validateVkResult(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineContext.pipelineLayout), "Error: failed to create VkPipelineLayout!");

            VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
            graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            graphicsPipelineCreateInfo.stageCount = 2;
            graphicsPipelineCreateInfo.pStages = shaderStages;
            graphicsPipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
            graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
            graphicsPipelineCreateInfo.pViewportState = &viewportCreateInfo;
            graphicsPipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
            graphicsPipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
            graphicsPipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
            graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
            graphicsPipelineCreateInfo.layout = pipelineContext.pipelineLayout;
            graphicsPipelineCreateInfo.renderPass = renderPass;
            graphicsPipelineCreateInfo.subpass = 0;
            graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

            validateVkResult(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &pipelineContext.pipeline), "Error: failed to create VkPipeline!");

            vkDestroyShaderModule(device, vertShaderModule, nullptr);
            vkDestroyShaderModule(device, fragShaderModule, nullptr);

            std::cout << "Create PipelineContext\n";
            return pipelineContext;
        }
        ~PipelineContext();
        VkPipelineLayout getPipelineLayout() const;
        VkPipeline getPipeline() const;
    private:
        PipelineContext() = default;
        VkDevice device;
        VkPipelineLayout pipelineLayout;
        VkPipeline pipeline;
    };

    // struct Tint
    // {
    //     glm::vec4 color;
    // };

    // struct Quad{};

    // struct CameraUBO
    // {
    //     alignas(16) glm::mat4 view;
    //     alignas(16) glm::mat4 proj;
    // };

    // struct Camera
    // {
    //     float fovYAxis;
    //     glm::mat4 getOrthoMatrix(uint32_t screenWidth, uint32_t screenHeight) const;
    //     Camera(float fovy = 1.0f) : fovYAxis(fovy) {}
    // };

    // struct InstanceData
    // {
    //     glm::mat4 model;
    //     glm::vec4 tint;
    //     static VkVertexInputBindingDescription getBindingDescription(uint32_t binding = 0);
    //     static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions(uint32_t binding = 0, uint32_t location = 0);
    // };

    // class Engine
    // {
    // public:
    //     std::vector<std::function<void(float)>> updateCallbacks;
    //     Engine(int width, int height, const char* applicationName);
    //     ~Engine();
    //     void run();
    //     void shouldResizeFramebuffer();
    //     void memcpyCameraUBO(const CameraUBO& ubo);
    //     void updateInstanceBuffer(const std::vector<InstanceData>& instances);
    //     const VkExtent2D& getSwapchainExtent() const;
    //     void getCursorWorldSpace(Scene& scene, const Entity& cam, float* x, float* y) const;
    //     GLFWkeyfun setKeyCallback(GLFWkeyfun callback);
    //     GLFWmousebuttonfun setMouseButtonCallback(GLFWmousebuttonfun callback);
    // private:
    //     const bool ENABLE_VALIDATION_LAYERS = true;
    //     const int MAX_FRAMES_IN_FLIGHT = 2;
    //     const char* ENGINE_NAME = "Silk Engine";
    //     const std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };
    //     const std::vector<const char*> DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    //     bool framebufferResized = true;
    //     uint32_t currentFrame = 0;
    //     uint32_t maxInstances = 100;
    //     GLFWwindow* window;
    //     VkInstance instance;
    //     VkDebugUtilsMessengerEXT debugMessenger;
    //     VkSurfaceKHR surface;
    //     VkPhysicalDevice physicalDevice;
    //     VkDevice device;
    //     VkQueue graphicsQueue;
    //     VkQueue presentQueue;
    //     uint32_t graphicsQueueFamilyIndex, presentQueueFamilyIndex;
    //     VkSwapchainKHR swapchain;
    //     VkExtent2D swapchainExtent;
    //     std::vector<VkImageView> swapchainImageViews;
    //     VkRenderPass renderPass;
    //     VkDescriptorSetLayout descriptorSetLayout;
    //     VkPipelineLayout pipelineLayout;
    //     VkPipeline graphicsPipeline;
    //     std::vector<VkFramebuffer> swapchainFramebuffers;
    //     VkCommandPool commandPool;
    //     VkBuffer vertexBuffer;
    //     VkDeviceMemory vertexBufferMemory;
    //     std::vector<uint32_t> instanceCounts;
    //     std::vector<VkBuffer> instanceBuffers;
    //     std::vector<VkDeviceMemory> instanceBuffersMemory;
    //     std::vector<void*> instanceBuffersMapped;
    //     uint32_t indexCount;
    //     VkBuffer indexBuffer;
    //     VkDeviceMemory indexBufferMemory;
    //     std::vector<VkBuffer> uniformBuffers;
    //     std::vector<VkDeviceMemory> uniformBuffersMemory;
    //     std::vector<void*> uniformBuffersMapped;
    //     VkDescriptorPool descriptorPool;
    //     std::vector<VkDescriptorSet> descriptorSets;
    //     std::vector<VkCommandBuffer> commandBuffers;
    //     std::vector<VkSemaphore> imageAvailableSemaphores;
    //     std::vector<VkSemaphore> renderFinishedSemaphores;
    //     std::vector<VkFence> inFlightFences;
    //     VkResult createSwapchain(VkFormat& swapchainImageFormat);
    //     VkResult createImageViews(const VkFormat swapchainImageFormat);
    //     VkResult createShaderModule(VkShaderModule& shaderModule, const std::vector<char>& code);
    //     VkResult createFramebuffers();
    //     VkResult createInstanceBuffers();
    //     VkResult createBuffer(const VkDeviceSize size, const VkBufferUsageFlags usageFlags, const VkMemoryPropertyFlags propertyFlags, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    //     VkResult copyBuffer(const VkBuffer srcBuffer, VkBuffer& dstBuffer, const VkDeviceSize size);
    //     VkResult recreateSwapchain();
    //     void cleanupSwapchain();
    //     void cleanupInstanceBuffers();
    // };
}