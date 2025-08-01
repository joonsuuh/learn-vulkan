#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
// #include <format>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include <vulkan/vk_platform.h>

#define GLFW_INCLUDE_VULKAN // REQUIRED only for GLFW CreateWindowSurface.
#include <GLFW/glfw3.h>

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector validationLayers = { "VK_LAYER_KHRONOS_validation" };

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

class HelloTriangleApplication
{
public:
    void run()
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window = nullptr;

    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;
    vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
    vk::raii::SurfaceKHR surface = nullptr;

    vk::raii::PhysicalDevice physicalDevice = nullptr;
    vk::raii::Device device = nullptr;

    vk::raii::Queue graphicsQueue = nullptr;
    vk::raii::Queue presentQueue = nullptr;

    vk::raii::SwapchainKHR swapChain = nullptr;
    std::vector<vk::Image> swapChainImages;
    vk::Format swapChainImageFormat = vk::Format::eUndefined;
    vk::Extent2D swapChainExtent;
    std::vector<vk::raii::ImageView> swapChainImageViews;

    vk::raii::PipelineLayout pipelineLayout = nullptr;
    vk::raii::Pipeline graphicsPipeline = nullptr;

    vk::raii::CommandPool commandPool = nullptr;
    std::vector<vk::raii::CommandBuffer> commandBuffers;
    uint32_t graphicsIndex = 0;

    std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    std::vector<vk::raii::Fence> inFlightFences;
    uint32_t semaphoreIndex = 0;
    uint32_t currentFrame = 0;

    bool frameBufferResized = false;

#ifdef __APPLE__
    std::vector<const char*> requiredDeviceExtension = {
        vk::KHRSwapchainExtensionName,
        vk::KHRSpirv14ExtensionName,
        vk::KHRSynchronization2ExtensionName,
        vk::KHRCreateRenderpass2ExtensionName,
        vk::KHRPortabilitySubsetExtensionName, // macOS
        vk::KHRShaderDrawParametersExtensionName
    };
#else
    std::vector<const char*> requiredDeviceExtension = {
        vk::KHRSwapchainExtensionName,
        vk::KHRSpirv14ExtensionName,
        vk::KHRSynchronization2ExtensionName,
        vk::KHRCreateRenderpass2ExtensionName
    };
#endif

    void initWindow()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, frameBufferResizeCallback);
    }

    static void frameBufferResizeCallback(GLFWwindow* window, int width,
                                          int height)
    {
        auto app = reinterpret_cast<HelloTriangleApplication*>(
            glfwGetWindowUserPointer(window));
        app->frameBufferResized = true;
    }

    void initVulkan()
    {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createGraphicsPipeline();
        createCommandPool();
        createCommandBuffers();
        createSyncObjects();
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            drawFrame();
        }

        device.waitIdle();
    }

    void cleanupSwapChain()
    {
        swapChainImageViews.clear();
        swapChain = nullptr;
    }

    void recreateSwapChain()
    {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0)
        {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        device.waitIdle();
        cleanupSwapChain();

        createSwapChain();
        createImageViews();
    }

    void cleanup()
    {
        cleanupSwapChain();
        glfwDestroyWindow(window);

        glfwTerminate();
    }

    void createInstance()
    {
        constexpr vk::ApplicationInfo appInfo {
            .pApplicationName = "Hello Triangle",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "No Engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = vk::ApiVersion14
        };

        // Get the required layers
        std::vector<char const*> requiredLayers;
        if (enableValidationLayers)
        {
            requiredLayers.assign(validationLayers.begin(),
                                  validationLayers.end());
        }

        // Check if the required layers are supported by the Vulkan
        // implementation.
        auto layerProperties = context.enumerateInstanceLayerProperties();
        for (auto const& requiredLayer : requiredLayers)
        {
            if (std::ranges::none_of(layerProperties,
                                     [requiredLayer](auto const& layerProperty)
                                     {
                                         return strcmp(layerProperty.layerName,
                                                       requiredLayer) == 0;
                                     }))
            {
                throw std::runtime_error("Required layer not supported: " +
                                         std::string(requiredLayer));
            }
        }

        // Get the required extensions.
        auto requiredExtensions = getRequiredExtensions();

        // Check if the required extensions are supported by the Vulkan
        // implementation.
        auto extensionProperties =
            context.enumerateInstanceExtensionProperties();
        for (auto const& requiredExtension : requiredExtensions)
        {
            // std::cout << std::format("Required Extension: {}\n",
            //                          requiredExtension);
            if (std::ranges::none_of(
                    extensionProperties,
                    [requiredExtension](auto const& extensionProperty)
                    {
                        return strcmp(extensionProperty.extensionName,
                                      requiredExtension) == 0;
                    }))
            {
                throw std::runtime_error("Required extension not supported: " +
                                         std::string(requiredExtension));
            }
        }

        vk::InstanceCreateInfo createInfo {
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
            .ppEnabledLayerNames = requiredLayers.data(),
            .enabledExtensionCount =
                static_cast<uint32_t>(requiredExtensions.size()),
            .ppEnabledExtensionNames = requiredExtensions.data()
        };
#ifdef __APPLE__
            createInfo.flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif
        instance = vk::raii::Instance(context, createInfo);
    }

    void setupDebugMessenger()
    {
        if (!enableValidationLayers)
            return;

        vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
        vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
        vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT {
            .messageSeverity = severityFlags,
            .messageType = messageTypeFlags,
            .pfnUserCallback = &debugCallback
        };
        debugMessenger = instance.createDebugUtilsMessengerEXT(
            debugUtilsMessengerCreateInfoEXT);
    }

    void createSurface()
    {
        VkSurfaceKHR _surface;
        if (glfwCreateWindowSurface(*instance, window, nullptr, &_surface) != 0)
        {
            throw std::runtime_error("failed to create window surface!");
        }
        surface = vk::raii::SurfaceKHR(instance, _surface);
    }

    void pickPhysicalDevice()
    {
        std::vector<vk::raii::PhysicalDevice> devices =
            instance.enumeratePhysicalDevices();
        const auto devIter = std::ranges::find_if(
            devices,
            [&](auto const& device)
            {
                // Check if the device supports the Vulkan 1.3 API version
                bool supportsVulkan1_3 =
                    device.getProperties().apiVersion >= VK_API_VERSION_1_3;

                // Check if any of the queue families support graphics
                // operations
                auto queueFamilies = device.getQueueFamilyProperties();
                bool supportsGraphics = std::ranges::any_of(
                    queueFamilies,
                    [](auto const& qfp)
                    {
                        return !!(qfp.queueFlags &
                                  vk::QueueFlagBits::eGraphics);
                    });

                // Check if all required device extensions are available
                auto availableDeviceExtensions =
                    device.enumerateDeviceExtensionProperties();
                bool supportsAllRequiredExtensions = std::ranges::all_of(
                    requiredDeviceExtension,
                    [&availableDeviceExtensions](
                        auto const& requiredDeviceExtension)
                    {
                        return std::ranges::any_of(
                            availableDeviceExtensions,
                            [requiredDeviceExtension](
                                auto const& availableDeviceExtension)
                            {
                                return strcmp(availableDeviceExtension
                                                  .extensionName,
                                              requiredDeviceExtension) == 0;
                            });
                    });

                auto features = device.template getFeatures2<
                    vk::PhysicalDeviceFeatures2,
                    vk::PhysicalDeviceVulkan13Features,
                    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
                bool supportsRequiredFeatures =
                    features.template get<vk::PhysicalDeviceVulkan13Features>()
                        .dynamicRendering &&
                    features
                        .template get<
                            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>()
                        .extendedDynamicState;

                return supportsVulkan1_3 && supportsGraphics &&
                       supportsAllRequiredExtensions &&
                       supportsRequiredFeatures;
            });
        if (devIter != devices.end())
        {
            physicalDevice = *devIter;
        }
        else
        {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    void createLogicalDevice()
    {
        // find the index of the first queue family that supports graphics
        std::vector<vk::QueueFamilyProperties> queueFamilyProperties =
            physicalDevice.getQueueFamilyProperties();

        // get the first index into queueFamilyProperties which supports
        // graphics
        auto graphicsQueueFamilyProperty = std::ranges::find_if(
            queueFamilyProperties,
            [](auto const& qfp)
            {
                return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) !=
                       static_cast<vk::QueueFlags>(0);
            });
        assert(graphicsQueueFamilyProperty != queueFamilyProperties.end() &&
               "No graphics queue family found!");

        graphicsIndex = static_cast<uint32_t>(std::distance(
            queueFamilyProperties.begin(), graphicsQueueFamilyProperty));

        auto presentIndex =
            physicalDevice.getSurfaceSupportKHR(graphicsIndex, *surface)
                ? graphicsIndex
                : static_cast<uint32_t>(queueFamilyProperties.size());
        if (presentIndex == queueFamilyProperties.size())
        {
            for (size_t i = 0; i < queueFamilyProperties.size(); i++)
            {
                if ((queueFamilyProperties[i].queueFlags &
                     vk::QueueFlagBits::eGraphics) &&
                    physicalDevice.getSurfaceSupportKHR(
                        static_cast<uint32_t>(i), *surface))
                {
                    graphicsIndex = static_cast<uint32_t>(i);
                    presentIndex = graphicsIndex;
                    break;
                }
            }
            if (presentIndex == queueFamilyProperties.size())
            {
                // there's nothing like a single family index that supports both
                // graphics and present -> look for another family index that
                // supports present
                for (size_t i = 0; i < queueFamilyProperties.size(); i++)
                {
                    if (physicalDevice.getSurfaceSupportKHR(
                            static_cast<uint32_t>(i), *surface))
                    {
                        presentIndex = static_cast<uint32_t>(i);
                        break;
                    }
                }
            }
        }
        if ((graphicsIndex == queueFamilyProperties.size()) ||
            (presentIndex == queueFamilyProperties.size()))
        {
            throw std::runtime_error("Could not find a queue for graphics or "
                                     "present -> terminating");
        }

        // query for Vulkan 1.3 features
        vk::StructureChain<vk::PhysicalDeviceFeatures2,
                           vk::PhysicalDeviceVulkan13Features,
                           vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
            featureChain = {
                {}, // vk::PhysicalDeviceFeatures2
                { .synchronization2 = vk::True,
                  .dynamicRendering =
                      vk::True }, // vk::PhysicalDeviceVulkan13Features
                { .extendedDynamicState = vk::
                      True } // vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
            };

        // create a Device
        float queuePriority = 0.0f;
        vk::DeviceQueueCreateInfo deviceQueueCreateInfo { .queueFamilyIndex =
                                                              graphicsIndex,
                                                          .queueCount = 1,
                                                          .pQueuePriorities =
                                                              &queuePriority };
        vk::DeviceCreateInfo deviceCreateInfo {
            .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &deviceQueueCreateInfo,
            .enabledExtensionCount =
                static_cast<uint32_t>(requiredDeviceExtension.size()),
            .ppEnabledExtensionNames = requiredDeviceExtension.data()
        };

        device = vk::raii::Device(physicalDevice, deviceCreateInfo);
        graphicsQueue = vk::raii::Queue(device, graphicsIndex, 0);
        presentQueue = vk::raii::Queue(device, presentIndex, 0);
    }

    void createSwapChain()
    {
        auto surfaceCapabilites =
            physicalDevice.getSurfaceCapabilitiesKHR(surface);
        swapChainImageFormat = chooseSwapSurfaceFormat(
            physicalDevice.getSurfaceFormatsKHR(surface));
        swapChainExtent = chooseSwapExtent(surfaceCapabilites);
        auto minImageCount = std::max(3u, surfaceCapabilites.minImageCount);
        minImageCount = (surfaceCapabilites.maxImageCount > 0 &&
                         minImageCount > surfaceCapabilites.maxImageCount)
                            ? surfaceCapabilites.maxImageCount
                            : minImageCount;

        uint32_t imageCount = surfaceCapabilites.minImageCount + 1;
        if (surfaceCapabilites.maxImageCount > 0 &&
            imageCount > surfaceCapabilites.maxImageCount)
        {
            imageCount = surfaceCapabilites.maxImageCount;
        }
        vk::SwapchainCreateInfoKHR swapChainCreateInfo {
            .flags = vk::SwapchainCreateFlagsKHR(),
            .surface = surface,
            .minImageCount = minImageCount,
            .imageFormat = swapChainImageFormat,
            .imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear,
            .imageExtent = swapChainExtent,
            .imageArrayLayers = 1,
            .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
            .imageSharingMode = vk::SharingMode::eExclusive,
            .preTransform = surfaceCapabilites.currentTransform,
            .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
            .presentMode = chooseSwapPresentMode(
                physicalDevice.getSurfacePresentModesKHR(surface)),
            .clipped = vk::True
        };

        swapChain = vk::raii::SwapchainKHR(device, swapChainCreateInfo);
        swapChainImages = swapChain.getImages();
    }

    void createImageViews()
    {
        swapChainImageViews.clear();

        vk::ImageViewCreateInfo imageViewCreateInfo {
            .viewType = vk::ImageViewType::e2D,
            .format = swapChainImageFormat,
            .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
        };

        for (auto image : swapChainImages)
        {
            imageViewCreateInfo.image = image;
            swapChainImageViews.emplace_back(device, imageViewCreateInfo);
        }
    }

    void createGraphicsPipeline()
    {
        vk::raii::ShaderModule shaderModule =
            createShaderModule(readFile(SHADER_DIR "slang.spv"));

        vk::PipelineShaderStageCreateInfo vertShaderStageInfo {
            .stage = vk::ShaderStageFlagBits::eVertex,
            .module = shaderModule,
            .pName = "vertMain"
        };
        vk::PipelineShaderStageCreateInfo fragShaderStageInfo {
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = shaderModule,
            .pName = "fragMain"
        };
        vk::PipelineShaderStageCreateInfo shaderStages[] = {
            vertShaderStageInfo, fragShaderStageInfo
        };

        vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
        vk::PipelineInputAssemblyStateCreateInfo inputAssembly {
            .topology = vk::PrimitiveTopology::eTriangleList
        };
        vk::PipelineViewportStateCreateInfo viewportState { .viewportCount = 1,
                                                            .scissorCount = 1 };

        vk::PipelineRasterizationStateCreateInfo rasterizer {
            .depthClampEnable = vk::False,
            .rasterizerDiscardEnable = vk::False,
            .polygonMode = vk::PolygonMode::eFill,
            .cullMode = vk::CullModeFlagBits::eBack,
            .frontFace = vk::FrontFace::eClockwise,
            .depthBiasEnable = vk::False,
            .depthBiasSlopeFactor = 1.0f,
            .lineWidth = 1.0f
        };
        vk::PipelineMultisampleStateCreateInfo multisampling {
            .rasterizationSamples = vk::SampleCountFlagBits::e1,
            .sampleShadingEnable = vk::False
        };
        vk::PipelineDepthStencilStateCreateInfo depthStencil;

        vk::PipelineColorBlendAttachmentState colorBlendAttachment {
            .blendEnable = vk::False,
            .colorWriteMask = vk::ColorComponentFlagBits::eR |
                              vk::ColorComponentFlagBits::eG |
                              vk::ColorComponentFlagBits::eB |
                              vk::ColorComponentFlagBits::eA
        };
        vk::PipelineColorBlendStateCreateInfo colorBlending {
            .logicOpEnable = vk::False,
            .logicOp = vk::LogicOp::eCopy,
            .attachmentCount = 1,
            .pAttachments = &colorBlendAttachment
        };

        std::vector dynamicStates = { vk::DynamicState::eViewport,
                                      vk::DynamicState::eScissor };
        vk::PipelineDynamicStateCreateInfo dynamicState {
            .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
            .pDynamicStates = dynamicStates.data()
        };

        vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
        pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

        vk::PipelineRenderingCreateInfo pipelineRederingCreateInfo {
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &swapChainImageFormat
        };
        vk::GraphicsPipelineCreateInfo pipelineInfo {
            .pNext = &pipelineRederingCreateInfo,
            .stageCount = 2,
            .pStages = shaderStages,
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState = &multisampling,
            .pDepthStencilState = &depthStencil,
            .pColorBlendState = &colorBlending,
            .pDynamicState = &dynamicState,
            .layout = pipelineLayout,
            .renderPass = nullptr
        };

        graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);
    }

    void createCommandPool()
    {
        vk::CommandPoolCreateInfo poolInfo {
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = graphicsIndex
        };

        commandPool = vk::raii::CommandPool(device, poolInfo);
    }

    void createCommandBuffers()
    {
        commandBuffers.clear();
        vk::CommandBufferAllocateInfo allocInfo {
            .commandPool = commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = MAX_FRAMES_IN_FLIGHT
        };
        commandBuffers = vk::raii::CommandBuffers(device, allocInfo);
    }

    void createSyncObjects()
    {
        presentCompleteSemaphores.clear();
        renderFinishedSemaphores.clear();
        inFlightFences.clear();

        for (size_t i = 0; i < swapChainImages.size(); i++)
        {
            presentCompleteSemaphores.emplace_back(
                vk::raii::Semaphore(device, vk::SemaphoreCreateInfo()));
            renderFinishedSemaphores.emplace_back(
                vk::raii::Semaphore(device, vk::SemaphoreCreateInfo()));
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            inFlightFences.emplace_back(vk::raii::Fence(
                device, { .flags = vk::FenceCreateFlagBits::eSignaled }));
        }
    }

    void recordCommandBuffer(uint32_t imageIndex)
    {
        commandBuffers[currentFrame].begin({});

        transition_image_layout(
            imageIndex,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            {},
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::PipelineStageFlagBits2::eTopOfPipe,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput);

        vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
        vk::RenderingAttachmentInfo attachmentInfo = {
            .imageView = swapChainImageViews[imageIndex],
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = clearColor
        };

        vk::RenderingInfo renderingInfo = {
            .renderArea = { .offset = { 0, 0 }, .extent = swapChainExtent },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &attachmentInfo
        };

        commandBuffers[currentFrame].beginRendering(renderingInfo);

        commandBuffers[currentFrame].bindPipeline(
            vk::PipelineBindPoint::eGraphics, graphicsPipeline);

        commandBuffers[currentFrame].setViewport(
            0,
            vk::Viewport(0.0f,
                         0.0f,
                         static_cast<float>(swapChainExtent.width),
                         static_cast<float>(swapChainExtent.height),
                         0.0f,
                         1.0f));
        commandBuffers[currentFrame].setScissor(
            0, vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent));

        commandBuffers[currentFrame].draw(3, 1, 0, 0);

        commandBuffers[currentFrame].endRendering();

        transition_image_layout(
            imageIndex,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            vk::AccessFlagBits2::eColorAttachmentWrite,         // srcAccessMask
            {},                                                 // dstAccessMask
            vk::PipelineStageFlagBits2::eColorAttachmentOutput, // srcStage
            vk::PipelineStageFlagBits2::eBottomOfPipe           // dstStage
        );

        commandBuffers[currentFrame].end();
    }

    void transition_image_layout(uint32_t imageIndex, vk::ImageLayout oldLayout,
                                 vk::ImageLayout newLayout,
                                 vk::AccessFlags2 srcAccessMask,
                                 vk::AccessFlags2 dstAccessMask,
                                 vk::PipelineStageFlags2 srcStageMask,
                                 vk::PipelineStageFlags2 dstStageMask)
    {
        vk::ImageMemoryBarrier2 barrier = {
            .srcStageMask = srcStageMask,
            .srcAccessMask = srcAccessMask,
            .dstStageMask = dstStageMask,
            .dstAccessMask = dstAccessMask,
            .oldLayout = oldLayout,
            .newLayout = newLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = swapChainImages[imageIndex],
            .subresourceRange = { .aspectMask = vk::ImageAspectFlagBits::eColor,
                                  .baseMipLevel = 0,
                                  .levelCount = 1,
                                  .baseArrayLayer = 0,
                                  .layerCount = 1 }
        };
        vk::DependencyInfo dependencyInfo = { .dependencyFlags = {},
                                              .imageMemoryBarrierCount = 1,
                                              .pImageMemoryBarriers =
                                                  &barrier };
        commandBuffers[currentFrame].pipelineBarrier2(dependencyInfo);
    }

    void drawFrame()
    {
        // while (vk::Result::eTimeout ==
        //        device.waitForFences(
        //            *inFlightFences[currentFrame], vk::True, UINT64_MAX))
        //     ;
        presentQueue.waitIdle();

        auto [result, imageIndex] = swapChain.acquireNextImage(
            UINT64_MAX, *presentCompleteSemaphores[semaphoreIndex], nullptr);
        if (result == vk::Result::eErrorOutOfDateKHR)
        {
            recreateSwapChain();
            return;
        }
        if (result != vk::Result::eSuccess &&
            result != vk::Result::eSuboptimalKHR)
        {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        device.resetFences(*inFlightFences[currentFrame]);
        commandBuffers[currentFrame].reset();
        recordCommandBuffer(imageIndex);

        vk::PipelineStageFlags waitDestinationStageMask(
            vk::PipelineStageFlagBits::eColorAttachmentOutput);
        const vk::SubmitInfo submitInfo {
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*presentCompleteSemaphores[semaphoreIndex],
            .pWaitDstStageMask = &waitDestinationStageMask,
            .commandBufferCount = 1,
            .pCommandBuffers = &*commandBuffers[currentFrame],
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &*renderFinishedSemaphores[imageIndex]
        };
        graphicsQueue.submit(submitInfo, *inFlightFences[currentFrame]);

        const vk::PresentInfoKHR presentInfoKHR {
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*renderFinishedSemaphores[imageIndex],
            .swapchainCount = 1,
            .pSwapchains = &*swapChain,
            .pImageIndices = &imageIndex
        };
        result = presentQueue.presentKHR(presentInfoKHR);
        if (result == vk::Result::eErrorOutOfDateKHR ||
            result == vk::Result::eSuboptimalKHR || frameBufferResized)
        {
            frameBufferResized = false;
            recreateSwapChain();
        }
        else if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error("failed to present swap chain image!");
        }
        semaphoreIndex =
            (semaphoreIndex + 1) % presentCompleteSemaphores.size();
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    [[nodiscard]] vk::raii::ShaderModule
    createShaderModule(const std::vector<char>& code) const
    {
        vk::ShaderModuleCreateInfo createInfo {
            .codeSize = code.size() * sizeof(char),
            .pCode = reinterpret_cast<const uint32_t*>(code.data())
        };
        vk::raii::ShaderModule shaderModule { device, createInfo };

        return shaderModule;
    }

    std::vector<const char*> getRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        auto glfwExtensions =
            glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector extensions(glfwExtensions,
                               glfwExtensions + glfwExtensionCount);
        if (enableValidationLayers)
        {
            extensions.push_back(vk::EXTDebugUtilsExtensionName);
        }

#ifdef __APPLE__
        extensions.push_back(vk::KHRGetPhysicalDeviceProperties2ExtensionName);
        extensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
#endif

        return extensions;
    }

    static vk::Format chooseSwapSurfaceFormat(
        const std::vector<vk::SurfaceFormatKHR>& availableFormats)
    {
        const auto formatIt = std::ranges::find_if(
            availableFormats,
            [](const auto& format)
            {
                return format.format == vk::Format::eB8G8R8A8Srgb &&
                       format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
            });
        return formatIt != availableFormats.end() ? formatIt->format
                                                  : availableFormats[0].format;
    }

    static vk::PresentModeKHR chooseSwapPresentMode(
        const std::vector<vk::PresentModeKHR>& availablePresentModes)
    {
        return std::ranges::any_of(
                   availablePresentModes,
                   [](const vk::PresentModeKHR value)
                   { return vk::PresentModeKHR::eMailbox == value; })
                   ? vk::PresentModeKHR::eMailbox
                   : vk::PresentModeKHR::eFifo;
    }

    vk::Extent2D
    chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
    {
        if (capabilities.currentExtent.width !=
            std::numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        return { std::clamp<uint32_t>(width,
                                      capabilities.minImageExtent.width,
                                      capabilities.maxImageExtent.width),
                 std::clamp<uint32_t>(height,
                                      capabilities.minImageExtent.height,
                                      capabilities.maxImageExtent.height) };
    }

    static std::vector<char> readFile(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error("failed to open file!");
        }

        std::vector<char> buffer(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

        // std::cout << "SPIRV bytecode size: "
        //           << static_cast<std::streamsize>(buffer.size()) << '\n';
        file.close();
        return buffer;
    }

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL
    debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                  vk::DebugUtilsMessageTypeFlagsEXT type,
                  const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
                  void* pUserData)
    {
        // for some reason tree sitter works with this useless comment
        if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError ||
            severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
        {
            std::cerr << "validation layer: type " << vk::to_string(type)
                      << " msg: " << pCallbackData->pMessage << std::endl;
        }

        return vk::False;
    }
};

int main()
{
    try
    {
        HelloTriangleApplication app;
        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
