#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <vector>
#include <cstring>
#include <iostream>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const char* APPLICATION_NAME = "Silk Engine Main";
const char* ENGINE_NAME = "Silk Engine";

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#define DEBUG_MODE

#ifdef DEBUG_MODE
const bool enableValidationLayers = true;
#else
const bool enableValidationLayers = false;
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

void populateVkDebugUtilsMessengerCreateInfoEXT(VkDebugUtilsMessengerCreateInfoEXT& debugMessengerCreateInfo)
{
    debugMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugMessengerCreateInfo.pfnUserCallback = debugCallback;
}

int main()
{
    // init glfw
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, APPLICATION_NAME, nullptr, nullptr);

    // create VkInstance
    VkInstance instance;
    {
        // check validation layer support
        if (enableValidationLayers)
        {
            uint32_t layerCount;
            vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

            std::vector<VkLayerProperties> availableLayers(layerCount);
            vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
            
            for (const char* layerName : validationLayers)
            {
                bool layerFound = false;
                for (const auto& layerProperties : availableLayers)
                {
                    if (strcmp(layerName, layerProperties.layerName) == 0)
                    {
                        layerFound = true;
                        break;
                    }
                }
                
                if (!layerFound)
                {
                    std::cout << "Error: validation layers requested but not available!" << std::endl;
                    return EXIT_FAILURE;
                }
            }
        }

        VkApplicationInfo applicationInfo{};
        applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        applicationInfo.pApplicationName = APPLICATION_NAME;
        applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        applicationInfo.pEngineName = ENGINE_NAME;
        applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        applicationInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo instanceCreateInfo{};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pApplicationInfo = &applicationInfo;

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        if (enableValidationLayers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

        if (enableValidationLayers)
        {
            instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();

            VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{};
            populateVkDebugUtilsMessengerCreateInfoEXT(debugMessengerCreateInfo);

            instanceCreateInfo.pNext = &debugMessengerCreateInfo;
        }
        else
        {
            instanceCreateInfo.enabledLayerCount = 0;
            instanceCreateInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&instanceCreateInfo, nullptr, &instance) != VK_SUCCESS)
        {
            std::cout << "Error: failed to create VkInstance!" << std::endl;
            return EXIT_FAILURE;
        }
    }

    // create VkDebugUtilsMessengerEXT
    VkDebugUtilsMessengerEXT debugMessenger;
    {
        if (enableValidationLayers)
        {
            VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{};
            populateVkDebugUtilsMessengerCreateInfoEXT(debugMessengerCreateInfo);

            auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
            if (func == nullptr || func(instance, &debugMessengerCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS)
            {
                std::cout << "Error: failed to create VkDebugUtilsMessengerEXT!" << std::endl;
                return EXIT_FAILURE;
            }
        }
    }

    // main loop
    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    // cleanup
    if (enableValidationLayers)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            func(instance, debugMessenger, nullptr);
        }
    }
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}