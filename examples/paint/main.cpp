#include "silk/Engine.h"

#include <iostream>

// TODO
// - 2D paint
//      - Get previous & current frame's mouse position
//      - Draw capsule from previous to curr mouse pos (7/2)
//      - Control diameter with scroll
//      - Swap color with numbers
//      - add a clear button to reset
//      - Improve API: minimize code duplication
// - flatland RC or holographic RC (7/13)
// - Improve API
// - screen space RC, Input System, "Phox" Engine, ...
int main()
{
    const uint32_t WIDTH = 960;
    const uint32_t HEIGHT = 960;
    const char* APPLICATION_NAME = "Paint";
    silk::WindowContext windowContext(WIDTH, HEIGHT, APPLICATION_NAME);

    silk::DeviceContextCreateInfo deviceContextCreateInfo{};
    deviceContextCreateInfo.applicationName = APPLICATION_NAME;

    silk::DeviceContext deviceContext(windowContext.getWindow(), deviceContextCreateInfo);

    silk::RenderPassContext renderPassContext(deviceContext);

    silk::SwapchainContext swapchainContext(windowContext.getWindow(), deviceContext, renderPassContext.getRenderPass());

    silk::DescriptorSetLayoutContext descriptorSetLayoutContext(deviceContext.getDevice());

    silk::PipelineContextCreateInfo pipelineContextCreateInfo{};
    pipelineContextCreateInfo.descriptorSetLayouts = { descriptorSetLayoutContext.getDescriptorSetLayout() };

    silk::PipelineContext pipelineContext(deviceContext.getDevice(), renderPassContext.getRenderPass(), pipelineContextCreateInfo);

    silk::CommandPoolContext commandPoolContext(deviceContext);

    const int MAX_FRAMES_IN_FLIGHT = 2;
    silk::DescriptorPoolContext descriptorPoolContext(deviceContext.getDevice(), {}, MAX_FRAMES_IN_FLIGHT);

    return EXIT_SUCCESS;
}