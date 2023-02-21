#include "HelloTriangleApp.h"
#include "QueueFamilyIndices.h"
#include "SwapChainSupportDetails.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

namespace HelloTriangleAppPrivate
{
    static constexpr char* ourAppName = "HelloTriangle";
    static constexpr char* ourEngineName = "HelloVulkan";
    static constexpr int ourWidth = 800;
    static constexpr int ourHeight = 600;
    static constexpr int ourMaxFramesInFlight = 2;

    static const std::vector<const char*> ourValidationLayers =
    {
        "VK_LAYER_KHRONOS_validation"
    };

    static const std::vector<const char*> ourDeviceExtensions =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT aMessageSeverity, VkDebugUtilsMessageTypeFlagsEXT aMessageType, const VkDebugUtilsMessengerCallbackDataEXT* aCallbackData, void* anUserData)
    {
        if (aMessageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            std::cerr << "Error for validation layer: " << aCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }

    static VkResult CreateDebugUtilsMessengerEXT(VkInstance aVkInstance, const VkDebugUtilsMessengerCreateInfoEXT* aVkDebugUtilsMessengerCreateInfo, const VkAllocationCallbacks* aVkAllocationCallbacks, VkDebugUtilsMessengerEXT* aVkDebugUtilsMessenger)
    {
        PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(aVkInstance, "vkCreateDebugUtilsMessengerEXT"));
        if (vkCreateDebugUtilsMessenger)
        {
            return vkCreateDebugUtilsMessenger(aVkInstance, aVkDebugUtilsMessengerCreateInfo, aVkAllocationCallbacks, aVkDebugUtilsMessenger);
        }
        else
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    static void DestroyDebugUtilsMessengerEXT(VkInstance aVkInstance, VkDebugUtilsMessengerEXT aVkDestroyDebugUtilsMessenger, const VkAllocationCallbacks* aVkAllocationCallbacks)
    {
        PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(aVkInstance, "vkDestroyDebugUtilsMessengerEXT"));
        if (vkDestroyDebugUtilsMessenger)
            vkDestroyDebugUtilsMessenger(aVkInstance, aVkDestroyDebugUtilsMessenger, aVkAllocationCallbacks);
    }
}

HelloTriangleApp::HelloTriangleApp()
    : myGLFWWindow(nullptr)
    , myVkInstance(nullptr)
    , myVkDebugMessenger(nullptr)
    , myVkSurface(nullptr)
    , myVkPhysicalDevice(nullptr)
    , myVkDevice(nullptr)
    , myVkGraphicsPipeline(nullptr)
    , myVkCommandPool(nullptr)
    , myVkGraphicsQueue(nullptr)
    , myVkPresentQueue(nullptr)
    , myVkSwapChain(nullptr)
    , myVkSwapChainImageFormat(VkFormat::VK_FORMAT_UNDEFINED)
    , myVkSwapChainExtent()
    , myVkRenderPass(nullptr)
    , myVkPipelineLayout(nullptr)
    , myCurrentFrameIndex(0)
    , myIsFramebufferResized(false)
{
}

void HelloTriangleApp::Run()
{
    myResourcesPath = std::filesystem::current_path().generic_string() + "/Debug/Resources/";

    InitializeWindow();
    InitializeVulkan();
    MainLoop();
    Cleanup();
}

void HelloTriangleApp::InitializeWindow()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    myGLFWWindow = glfwCreateWindow(HelloTriangleAppPrivate::ourWidth, HelloTriangleAppPrivate::ourHeight, HelloTriangleAppPrivate::ourAppName, nullptr, nullptr);
    glfwSetWindowUserPointer(myGLFWWindow, this);
    glfwSetFramebufferSizeCallback(myGLFWWindow, FramebufferResizeCallback);
}

void HelloTriangleApp::FramebufferResizeCallback(GLFWwindow* aWindow, int aWidth, int aHeight)
{
    HelloTriangleApp* helloTriangleApp = reinterpret_cast<HelloTriangleApp*>(glfwGetWindowUserPointer(aWindow));
    helloTriangleApp->myIsFramebufferResized = true;
}

void HelloTriangleApp::InitializeVulkan()
{
    CreateInstance();
    SetupDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapChain();
    CreateImageViews();
    CreateRenderPass();
    CreateGraphicsPipeline();
    CreateFramebuffers();
    CreateCommandPool();
    CreateCommandBuffers();
    CreateSyncObjects();
}

void HelloTriangleApp::MainLoop()
{
    while (!glfwWindowShouldClose(myGLFWWindow))
    {
        glfwPollEvents();
        DrawFrame();
    }

    vkDeviceWaitIdle(myVkDevice);
}

void HelloTriangleApp::CleanupSwapChain()
{
    for (VkFramebuffer& framebuffer : myVkSwapChainFramebuffers)
        vkDestroyFramebuffer(myVkDevice, framebuffer, nullptr);

    vkFreeCommandBuffers(myVkDevice, myVkCommandPool, static_cast<uint32_t>(myVkCommandBuffers.size()), myVkCommandBuffers.data());

    vkDestroyPipeline(myVkDevice, myVkGraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(myVkDevice, myVkPipelineLayout, nullptr);
    vkDestroyRenderPass(myVkDevice, myVkRenderPass, nullptr);

    for (VkImageView& imageView : myVkSwapChainImageViews)
        vkDestroyImageView(myVkDevice, imageView, nullptr);

    vkDestroySwapchainKHR(myVkDevice, myVkSwapChain, nullptr);
}

void HelloTriangleApp::Cleanup()
{
    CleanupSwapChain();

    for (unsigned int i = 0; i < HelloTriangleAppPrivate::ourMaxFramesInFlight; i++)
    {
        vkDestroySemaphore(myVkDevice, myVkRenderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(myVkDevice, myVkImageAvailableSemaphores[i], nullptr);
        vkDestroyFence(myVkDevice, myVkInFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(myVkDevice, myVkCommandPool, nullptr);

    vkDestroyDevice(myVkDevice, nullptr);

    if (enableValidationLayers)
        HelloTriangleAppPrivate::DestroyDebugUtilsMessengerEXT(myVkInstance, myVkDebugMessenger, nullptr);

    vkDestroySurfaceKHR(myVkInstance, myVkSurface, nullptr);
    vkDestroyInstance(myVkInstance, nullptr);

    glfwDestroyWindow(myGLFWWindow);

    glfwTerminate();
}

void HelloTriangleApp::RecreateSwapChain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(myGLFWWindow, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(myGLFWWindow, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(myVkDevice);

    CleanupSwapChain();

    CreateSwapChain();
    CreateImageViews();
    CreateRenderPass();
    CreateGraphicsPipeline();
    CreateFramebuffers();
    CreateCommandBuffers();
}

void HelloTriangleApp::CreateInstance()
{
    if (enableValidationLayers && !HasValidationLayerSupport())
        throw std::runtime_error("validation layers requested, but not available!");

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = HelloTriangleAppPrivate::ourAppName;
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    appInfo.pEngineName = HelloTriangleAppPrivate::ourEngineName;
    appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    std::vector<const char*> extensions = GetRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(HelloTriangleAppPrivate::ourValidationLayers.size());
        createInfo.ppEnabledLayerNames = HelloTriangleAppPrivate::ourValidationLayers.data();

        PopulateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &myVkInstance) != VK_SUCCESS)
        throw std::runtime_error("failed to create instance!");
}

void HelloTriangleApp::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& aCreateInfo)
{
    aCreateInfo = {};
    aCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    aCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    aCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    aCreateInfo.pfnUserCallback = HelloTriangleAppPrivate::DebugCallback;
}

void HelloTriangleApp::SetupDebugMessenger()
{
    if (!enableValidationLayers)
        return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    PopulateDebugMessengerCreateInfo(createInfo);

    if (HelloTriangleAppPrivate::CreateDebugUtilsMessengerEXT(myVkInstance, &createInfo, nullptr, &myVkDebugMessenger) != VK_SUCCESS)
        throw std::runtime_error("failed to set up debug messenger!");
}

void HelloTriangleApp::CreateSurface()
{
    if (glfwCreateWindowSurface(myVkInstance, myGLFWWindow, nullptr, &myVkSurface) != VK_SUCCESS)
        throw std::runtime_error("failed to create window surface!");
}

void HelloTriangleApp::PickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(myVkInstance, &deviceCount, nullptr);

    if (deviceCount == 0)
        throw std::runtime_error("failed to find GPUs with Vulkan support!");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(myVkInstance, &deviceCount, devices.data());

    for (const VkPhysicalDevice& device : devices)
    {
        if (IsDeviceSuitable(device))
        {
            myVkPhysicalDevice = device;
            break;
        }
    }

    if (!myVkPhysicalDevice)
        throw std::runtime_error("failed to find a suitable GPU!");
}

void HelloTriangleApp::CreateLogicalDevice()
{
    QueueFamilyIndices indices = GetQueueFamilyIndices(myVkPhysicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.myGraphicsFamily.value(), indices.myPresentFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(HelloTriangleAppPrivate::ourDeviceExtensions.size());
    createInfo.ppEnabledExtensionNames = HelloTriangleAppPrivate::ourDeviceExtensions.data();

    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(HelloTriangleAppPrivate::ourValidationLayers.size());
        createInfo.ppEnabledLayerNames = HelloTriangleAppPrivate::ourValidationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(myVkPhysicalDevice, &createInfo, nullptr, &myVkDevice) != VK_SUCCESS)
        throw std::runtime_error("failed to create logical device!");

    vkGetDeviceQueue(myVkDevice, indices.myGraphicsFamily.value(), 0, &myVkGraphicsQueue);
    vkGetDeviceQueue(myVkDevice, indices.myPresentFamily.value(), 0, &myVkPresentQueue);
}

void HelloTriangleApp::CreateSwapChain()
{
    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(myVkPhysicalDevice);

    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.myFormats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.myPresentModes);
    VkExtent2D extent = ChooseSwapExtent(swapChainSupport.myCapabilities);

    uint32_t imageCount = swapChainSupport.myCapabilities.minImageCount + 1;
    if (swapChainSupport.myCapabilities.maxImageCount > 0 && imageCount > swapChainSupport.myCapabilities.maxImageCount)
        imageCount = swapChainSupport.myCapabilities.maxImageCount;

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = myVkSurface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = GetQueueFamilyIndices(myVkPhysicalDevice);
    uint32_t queueFamilyIndices[] = { indices.myGraphicsFamily.value(), indices.myPresentFamily.value() };

    if (indices.myGraphicsFamily != indices.myPresentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.myCapabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(myVkDevice, &createInfo, nullptr, &myVkSwapChain) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(myVkDevice, myVkSwapChain, &imageCount, nullptr);
    myVkSwapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(myVkDevice, myVkSwapChain, &imageCount, myVkSwapChainImages.data());

    myVkSwapChainImageFormat = surfaceFormat.format;
    myVkSwapChainExtent = extent;
}

void HelloTriangleApp::CreateImageViews()
{
    myVkSwapChainImageViews.resize(myVkSwapChainImages.size());

    for (unsigned int i = 0; i < myVkSwapChainImages.size(); i++)
    {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = myVkSwapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = myVkSwapChainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(myVkDevice, &createInfo, nullptr, &myVkSwapChainImageViews[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create image views!");
        }
    }
}

void HelloTriangleApp::CreateRenderPass()
{
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = myVkSwapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(myVkDevice, &renderPassInfo, nullptr, &myVkRenderPass) != VK_SUCCESS)
        throw std::runtime_error("failed to create render pass!");
}

void HelloTriangleApp::CreateGraphicsPipeline()
{
    std::vector<char> vertShaderCode = ReadFile(myResourcesPath + "Shaders/vert.spv");
    std::vector<char> fragShaderCode = ReadFile(myResourcesPath + "Shaders/frag.spv");

    VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(myVkSwapChainExtent.width);
    viewport.height = static_cast<float>(myVkSwapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = myVkSwapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(myVkDevice, &pipelineLayoutInfo, nullptr, &myVkPipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("failed to create pipeline layout!");

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = myVkPipelineLayout;
    pipelineInfo.renderPass = myVkRenderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = nullptr;

    if (vkCreateGraphicsPipelines(myVkDevice, nullptr, 1, &pipelineInfo, nullptr, &myVkGraphicsPipeline) != VK_SUCCESS)
        throw std::runtime_error("failed to create graphics pipeline!");

    vkDestroyShaderModule(myVkDevice, fragShaderModule, nullptr);
    vkDestroyShaderModule(myVkDevice, vertShaderModule, nullptr);
}

void HelloTriangleApp::CreateFramebuffers()
{
    myVkSwapChainFramebuffers.resize(myVkSwapChainImageViews.size());

    for (unsigned int i = 0; i < myVkSwapChainImageViews.size(); i++)
    {
        VkImageView attachments[] =
        {
            myVkSwapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = myVkRenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = myVkSwapChainExtent.width;
        framebufferInfo.height = myVkSwapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(myVkDevice, &framebufferInfo, nullptr, &myVkSwapChainFramebuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("failed to create framebuffer!");
    }
}

void HelloTriangleApp::CreateCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = GetQueueFamilyIndices(myVkPhysicalDevice);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.myGraphicsFamily.value();

    if (vkCreateCommandPool(myVkDevice, &poolInfo, nullptr, &myVkCommandPool) != VK_SUCCESS)
        throw std::runtime_error("failed to create command pool!");
}

void HelloTriangleApp::CreateCommandBuffers()
{
    myVkCommandBuffers.resize(myVkSwapChainFramebuffers.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = myVkCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)myVkCommandBuffers.size();

    if (vkAllocateCommandBuffers(myVkDevice, &allocInfo, myVkCommandBuffers.data()) != VK_SUCCESS)
        throw std::runtime_error("failed to allocate command buffers!");

    for (unsigned int i = 0; i < myVkCommandBuffers.size(); i++)
    {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(myVkCommandBuffers[i], &beginInfo) != VK_SUCCESS)
            throw std::runtime_error("failed to begin recording command buffer!");

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = myVkRenderPass;
        renderPassInfo.framebuffer = myVkSwapChainFramebuffers[i];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = myVkSwapChainExtent;

        VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(myVkCommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(myVkCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, myVkGraphicsPipeline);

        vkCmdDraw(myVkCommandBuffers[i], 3, 1, 0, 0);

        vkCmdEndRenderPass(myVkCommandBuffers[i]);

        if (vkEndCommandBuffer(myVkCommandBuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("failed to record command buffer!");
    }
}

void HelloTriangleApp::CreateSyncObjects()
{
    myVkImageAvailableSemaphores.resize(HelloTriangleAppPrivate::ourMaxFramesInFlight);
    myVkRenderFinishedSemaphores.resize(HelloTriangleAppPrivate::ourMaxFramesInFlight);
    myVkInFlightFences.resize(HelloTriangleAppPrivate::ourMaxFramesInFlight);
    myVkImagesInFlight.resize(myVkSwapChainImages.size(), nullptr);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (unsigned int i = 0; i < HelloTriangleAppPrivate::ourMaxFramesInFlight; i++)
    {
        if (vkCreateSemaphore(myVkDevice, &semaphoreInfo, nullptr, &myVkImageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(myVkDevice, &semaphoreInfo, nullptr, &myVkRenderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(myVkDevice, &fenceInfo, nullptr, &myVkInFlightFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

void HelloTriangleApp::DrawFrame()
{
    vkWaitForFences(myVkDevice, 1, &myVkInFlightFences[myCurrentFrameIndex], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(myVkDevice, myVkSwapChain, UINT64_MAX, myVkImageAvailableSemaphores[myCurrentFrameIndex], nullptr, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        RecreateSwapChain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    if (myVkImagesInFlight[imageIndex])
        vkWaitForFences(myVkDevice, 1, &myVkImagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);

    myVkImagesInFlight[imageIndex] = myVkInFlightFences[myCurrentFrameIndex];

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { myVkImageAvailableSemaphores[myCurrentFrameIndex] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &myVkCommandBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = { myVkRenderFinishedSemaphores[myCurrentFrameIndex] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(myVkDevice, 1, &myVkInFlightFences[myCurrentFrameIndex]);

    if (vkQueueSubmit(myVkGraphicsQueue, 1, &submitInfo, myVkInFlightFences[myCurrentFrameIndex]) != VK_SUCCESS)
        throw std::runtime_error("failed to submit draw command buffer!");

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { myVkSwapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(myVkPresentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || myIsFramebufferResized)
    {
        myIsFramebufferResized = false;
        RecreateSwapChain();
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    myCurrentFrameIndex = (myCurrentFrameIndex + 1) % HelloTriangleAppPrivate::ourMaxFramesInFlight;
}

VkShaderModule HelloTriangleApp::CreateShaderModule(const std::vector<char>& aShaderCode)
{
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = aShaderCode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(aShaderCode.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(myVkDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        throw std::runtime_error("failed to create shader module!");

    return shaderModule;
}

VkSurfaceFormatKHR HelloTriangleApp::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& someAvailableFormats)
{
    for (const VkSurfaceFormatKHR& availableFormat : someAvailableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormat;
    }

    return someAvailableFormats[0];
}

VkPresentModeKHR HelloTriangleApp::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& someAvailablePresentModes)
{
    for (const VkPresentModeKHR& availablePresentMode : someAvailablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return availablePresentMode;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D HelloTriangleApp::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& aCapabilities)
{
    if (aCapabilities.currentExtent.width != UINT32_MAX)
    {
        return aCapabilities.currentExtent;
    }
    else
    {
        int width, height;
        glfwGetFramebufferSize(myGLFWWindow, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::max(aCapabilities.minImageExtent.width, std::min(aCapabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(aCapabilities.minImageExtent.height, std::min(aCapabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

SwapChainSupportDetails HelloTriangleApp::QuerySwapChainSupport(VkPhysicalDevice device)
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, myVkSurface, &details.myCapabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, myVkSurface, &formatCount, nullptr);

    if (formatCount != 0)
    {
        details.myFormats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, myVkSurface, &formatCount, details.myFormats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, myVkSurface, &presentModeCount, nullptr);

    if (presentModeCount != 0)
    {
        details.myPresentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, myVkSurface, &presentModeCount, details.myPresentModes.data());
    }

    return details;
}

bool HelloTriangleApp::IsDeviceSuitable(VkPhysicalDevice device)
{
    QueueFamilyIndices indices = GetQueueFamilyIndices(device);

    bool extensionsSupported = HasDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
        SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.myFormats.empty() && !swapChainSupport.myPresentModes.empty();
    }

    return indices.IsComplete() && extensionsSupported && swapChainAdequate;
}

bool HelloTriangleApp::HasDeviceExtensionSupport(VkPhysicalDevice aDevice)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(aDevice, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(aDevice, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(HelloTriangleAppPrivate::ourDeviceExtensions.begin(), HelloTriangleAppPrivate::ourDeviceExtensions.end());

    for (const VkExtensionProperties& extension : availableExtensions)
        requiredExtensions.erase(extension.extensionName);

    return requiredExtensions.empty();
}

QueueFamilyIndices HelloTriangleApp::GetQueueFamilyIndices(VkPhysicalDevice aDevice)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(aDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(aDevice, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const VkQueueFamilyProperties& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.myGraphicsFamily = i;

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(aDevice, i, myVkSurface, &presentSupport);

        if (presentSupport)
            indices.myPresentFamily = i;

        if (indices.IsComplete())
            break;

        i++;
    }

    return indices;
}

std::vector<const char*> HelloTriangleApp::GetRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return extensions;
}

bool HelloTriangleApp::HasValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : HelloTriangleAppPrivate::ourValidationLayers)
    {
        bool layerFound = false;

        for (const VkLayerProperties& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
            return false;
    }

    return true;
}

std::vector<char> HelloTriangleApp::ReadFile(const std::string& aPath)
{
    std::ifstream file(aPath, std::ios::ate | std::ios::binary);

    if (!file.is_open())
        throw std::runtime_error("failed to open file!");

    unsigned int fileSize = static_cast<unsigned int>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}
