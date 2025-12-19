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

namespace silk
{
    void validateVkResult(VkResult result, const char* msg);

    VkSurfaceFormatKHR getPhysicalDeviceSurfaceFormat(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface);

    class SwapchainContext
    {
    public:
        SwapchainContext(GLFWwindow* window, const VkPhysicalDevice& physicalDevice, uint32_t graphicsQueueFamilyIndex, uint32_t presentQueueFamilyIndex, const VkSurfaceKHR& surface, const VkDevice& device, const VkRenderPass& renderPass);
        ~SwapchainContext();
        const VkExtent2D& getExtent() const;
        const VkSwapchainKHR& getSwapchain() const;
        const std::vector<VkFramebuffer>& getFramebuffers() const;
    private:
        VkDevice device;
        VkExtent2D extent;
        VkSwapchainKHR swapchain;
        std::vector<VkImageView> imageViews;
        std::vector<VkFramebuffer> framebuffers;
    };

    struct Tint
    {
        glm::vec4 color;
    };

    struct Quad{};

    struct CameraUBO
    {
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    struct Camera
    {
        float fovYAxis;
        glm::mat4 getOrthoMatrix(uint32_t screenWidth, uint32_t screenHeight) const;
        Camera(float fovy = 1.0f) : fovYAxis(fovy) {}
    };

    struct InstanceData
    {
        glm::mat4 model;
        glm::vec4 tint;
        static VkVertexInputBindingDescription getBindingDescription(uint32_t binding = 0);
        static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions(uint32_t binding = 0, uint32_t location = 0);
    };

    struct Vertex
    {
        glm::vec2 position;
        static VkVertexInputBindingDescription getBindingDescription(uint32_t binding = 0);
        static std::array<VkVertexInputAttributeDescription, 1> getAttributeDescriptions(uint32_t binding = 0, uint32_t location = 0);
    };

    class Engine
    {
    public:
        std::vector<std::function<void(float)>> updateCallbacks;
        Engine(int width, int height, const char* applicationName);
        ~Engine();
        void run();
        void shouldResizeFramebuffer();
        void memcpyCameraUBO(const CameraUBO& ubo);
        void updateInstanceBuffer(const std::vector<InstanceData>& instances);
        const VkExtent2D& getSwapchainExtent() const;
        void getCursorWorldSpace(Scene& scene, const Entity& cam, float* x, float* y) const;
        GLFWkeyfun setKeyCallback(GLFWkeyfun callback);
        GLFWmousebuttonfun setMouseButtonCallback(GLFWmousebuttonfun callback);
    private:
        const bool ENABLE_VALIDATION_LAYERS = true;
        const int MAX_FRAMES_IN_FLIGHT = 2;
        const char* ENGINE_NAME = "Silk Engine";
        const std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };
        const std::vector<const char*> DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        bool framebufferResized = true;
        uint32_t currentFrame = 0;
        uint32_t maxInstances = 100;
        GLFWwindow* window;
        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkSurfaceKHR surface;
        VkPhysicalDevice physicalDevice;
        VkDevice device;
        VkQueue graphicsQueue;
        VkQueue presentQueue;
        uint32_t graphicsQueueFamilyIndex, presentQueueFamilyIndex;
        VkSwapchainKHR swapchain;
        VkExtent2D swapchainExtent;
        std::vector<VkImageView> swapchainImageViews;
        VkRenderPass renderPass;
        VkDescriptorSetLayout descriptorSetLayout;
        VkPipelineLayout pipelineLayout;
        VkPipeline graphicsPipeline;
        std::vector<VkFramebuffer> swapchainFramebuffers;
        VkCommandPool commandPool;
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;
        std::vector<uint32_t> instanceCounts;
        std::vector<VkBuffer> instanceBuffers;
        std::vector<VkDeviceMemory> instanceBuffersMemory;
        std::vector<void*> instanceBuffersMapped;
        uint32_t indexCount;
        VkBuffer indexBuffer;
        VkDeviceMemory indexBufferMemory;
        std::vector<VkBuffer> uniformBuffers;
        std::vector<VkDeviceMemory> uniformBuffersMemory;
        std::vector<void*> uniformBuffersMapped;
        VkDescriptorPool descriptorPool;
        std::vector<VkDescriptorSet> descriptorSets;
        std::vector<VkCommandBuffer> commandBuffers;
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        VkResult createSwapchain(VkFormat& swapchainImageFormat);
        VkResult createImageViews(const VkFormat swapchainImageFormat);
        VkResult createShaderModule(VkShaderModule& shaderModule, const std::vector<char>& code);
        VkResult createFramebuffers();
        VkResult createInstanceBuffers();
        VkResult createBuffer(const VkDeviceSize size, const VkBufferUsageFlags usageFlags, const VkMemoryPropertyFlags propertyFlags, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
        VkResult copyBuffer(const VkBuffer srcBuffer, VkBuffer& dstBuffer, const VkDeviceSize size);
        VkResult recreateSwapchain();
        void cleanupSwapchain();
        void cleanupInstanceBuffers();
    };
}