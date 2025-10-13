#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "vulkan.h"

#ifndef VK_EXT_DEBUG_REPORT_EXTENSION_NAME
#define VK_EXT_DEBUG_REPORT_EXTENSION_NAME "VK_EXT_debug_report"
#endif

int clamp(int d, int min, int max) {
  const int t = d < min ? min : d;
  return t > max ? max : t;
}

typedef struct {
    uint32_t graphicsFamily;
    uint32_t presentFamily;
	bool hasGraphicsFamilyValue;
	bool hasPresentFamilyValue;
} QueueFamilyIndices;

const int QueueFamilyIndicesCount = 2;

bool QueueFamilyIndices_IsComplete(QueueFamilyIndices* instance) {
	return (*instance).hasGraphicsFamilyValue && (*instance).hasPresentFamilyValue;
}

int QueueFamilyIndices_GetAt(QueueFamilyIndices* instance, int index) {
	switch (index)
	{
		case 0 : return (*instance).graphicsFamily;
		case 1 : return (*instance).presentFamily;
		default : return (uint32_t)(-1);
	}
}

typedef struct {
    VkSurfaceCapabilitiesKHR capabilities;
    int formatsCount;
	int presentModesCount;
	VkSurfaceFormatKHR* formats;
    VkPresentModeKHR* presentModes;
} SwapChainSupportDetails;

const char* const validationLayers[] = {
	"VK_LAYER_KHRONOS_validation"
};

const char* const deviceExtensions[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

void CreateInstance(const char* appName);
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT* createInfo);
void SetupDebugMessenger();
bool CheckValidationLayerSupport();
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
void PickPhysicalDevice();
bool IsDeviceSuitable(VkPhysicalDevice device);
QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
void CreateLogicalDevice();
void CreateSurface();
uint32_t* GetUniqueQueueFamilies(QueueFamilyIndices* indices, int *uniqueCount);
void CreateSwapChain();
bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const VkSurfaceFormatKHR* availableFormats, int availableFormatsCount);
VkPresentModeKHR ChooseSwapPresentMode(const VkPresentModeKHR* availablePresentModes, int availablePresentModesCount);
VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR* capabilities);
void CreateImageViews();
void CreateGraphicsPipeline();
void CreateRenderPass();
VkShaderModule CreateShaderModule(const uint32_t* code, size_t size);
void ReadSPIRV(const char* fileName, uint32_t** fileContents, size_t* fileSize);
long int GetFileSize(FILE *filePtr);

VkInstance instance;
VkDebugUtilsMessengerEXT debugMessenger;

VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
VkDevice device;

SDL_Window* window;
VkQueue graphicsQueue;
VkSurfaceKHR surface;
VkQueue presentQueue;

VkSwapchainKHR swapChain;
VkImage* swapChainImages;
uint32_t imageCount;
VkFormat swapChainImageFormat;
VkExtent2D swapChainExtent;
VkImageView* swapChainImageViews;

VkRenderPass renderPass;
VkPipelineLayout pipelineLayout;
VkPipeline graphicsPipeline;

void InitializeVulkan(const char* appName, SDL_Window* sdl_window)
{
	window = sdl_window;

	CreateInstance(appName);
	SetupDebugMessenger();
	CreateSurface();
	PickPhysicalDevice();
	CreateLogicalDevice();
	CreateSwapChain();
	CreateImageViews();
	CreateRenderPass();
	CreateGraphicsPipeline();
}

void CleanUpVulkan()
{
	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyRenderPass(device, renderPass, nullptr);	

	for (uint32_t i = 0; i < imageCount; i++)
	{
		vkDestroyImageView(device, swapChainImageViews[i], nullptr);
	}

	vkDestroySwapchainKHR(device, swapChain, nullptr);	
	vkDestroyDevice(device, nullptr);

	if (enableValidationLayers) {
		DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }
	
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);
}

void CreateInstance(const char* appName)
{
	if (enableValidationLayers && !CheckValidationLayerSupport()) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "VULKAN::validation layers requested, but not available!\n");
		exit(EXIT_FAILURE);
	}
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = appName;
	appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	Uint32 count_instance_extensions;
	const char* const* instance_extensions = SDL_Vulkan_GetInstanceExtensions(&count_instance_extensions);

	if (instance_extensions == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "VULKAN::instance extensions is null!\n");
		exit(EXIT_FAILURE);
	}

	int count_extensions = count_instance_extensions +  1;
	const char ** extensions = SDL_malloc(count_extensions * sizeof(const char*));
	extensions[0] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
	SDL_memcpy(&extensions[1], instance_extensions, count_instance_extensions * sizeof(const char*));
	
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	createInfo.enabledExtensionCount = count_extensions;
	createInfo.ppEnabledExtensionNames = extensions;

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = sizeof(validationLayers)/sizeof(validationLayers[0]);
		createInfo.ppEnabledLayerNames = validationLayers;

		PopulateDebugMessengerCreateInfo(&debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "VULKAN::failed to create instance!\n");
		exit(EXIT_FAILURE);
	}

	SDL_free(extensions);
}

void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT* createInfo) {
        (*createInfo) = (VkDebugUtilsMessengerCreateInfoEXT){};
        (*createInfo).sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        (*createInfo).messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        (*createInfo).messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        (*createInfo).pfnUserCallback = DebugCallback;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
        SDL_Log("VULKAN::validation layer: %s\n", pCallbackData->pMessage);

        return VK_FALSE;
}

void SetupDebugMessenger() {
	if (!enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	PopulateDebugMessengerCreateInfo(&createInfo);

	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "VULKAN::failed to set up debug messenger!\n");
		exit(EXIT_FAILURE);
	}
}

void CreateSurface() {
	if (SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface) != true) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "VULKAN::failed to create window surface!\n");
		exit(EXIT_FAILURE);
	}
}

void PickPhysicalDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "VULKAN::failed to find GPUs with Vulkan support!");
		exit(EXIT_FAILURE);
	}

	VkPhysicalDevice devices[deviceCount];
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices);

	for (int i = 0; i < deviceCount; i++) {
		if (IsDeviceSuitable(devices[i])) {
			physicalDevice = devices[i];
			break;
		}
	}

	if (physicalDevice == VK_NULL_HANDLE) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "VULKAN::failed to find a suitable GPU!");
		exit(EXIT_FAILURE);
	}
}

void CreateSwapChain() {
	SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats, swapChainSupport.formatsCount);
	VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes, swapChainSupport.presentModesCount);
	VkExtent2D extent = ChooseSwapExtent(&swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);
	uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "VULKAN::failed to create swap chain!");
		exit(EXIT_FAILURE);
	}

	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
	swapChainImages = (VkImage*)malloc(imageCount * sizeof(VkImage));
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages);

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

void CreateImageViews()
{
	swapChainImageViews = (VkImageView*)malloc(imageCount * sizeof(VkImageView));

	for (uint32_t i = 0; i < imageCount; i++)
	{
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapChainImageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		
		if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "VULKAN::failed to create image views!");
			exit(EXIT_FAILURE);
		}
	}
}

void CreateRenderPass() {
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapChainImageFormat;
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

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "VULKAN::failed to create render pass!");
		exit(EXIT_FAILURE);
	}
}

void CreateGraphicsPipeline()
{
	uint32_t* vertShaderCode;
	size_t vertFileSize;
	ReadSPIRV("../assets/vert.spv", &vertShaderCode, &vertFileSize);

	//uint32_t* fragShaderCode;
	//size_t fragFileSize;
	//ReadSPIRV("../assets/frag.spv", fragShaderCode, &fragFileSize);
	
	VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode, vertFileSize);
	//VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode, fragFileSize);

/*
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

	VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

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

	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "VULKAN::failed to create pipeline layout!");
		exit(EXIT_FAILURE);
	}

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
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "VULKAN::failed to create graphics pipeline!");
		exit(EXIT_FAILURE);
	}

	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
*/
}

VkShaderModule CreateShaderModule(const uint32_t* code, size_t size) {
	
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = size;
	createInfo.pCode = code;
	
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "VULKAN::failed to create shader module!\n");
		exit(EXIT_FAILURE);
	}

	return shaderModule;
}

VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const VkSurfaceFormatKHR* availableFormats, int availableFormatsCount) {
	for (int i = 0; i < availableFormatsCount; i++) {
		if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormats[i];
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR ChooseSwapPresentMode(const VkPresentModeKHR* availablePresentModes, int availablePresentModesCount) {
	for (int i = 0; i < availablePresentModesCount; i++) {
		if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentModes[i];
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR* capabilities) {
	if ((*capabilities).currentExtent.width != UINT32_MAX) {
		return (*capabilities).currentExtent;
	} else {
		int width, height;
		SDL_GetWindowSizeInPixels(window, &width, &height);

		VkExtent2D actualExtent = {
			(uint32_t)(width),
			(uint32_t)(height)
		};

		actualExtent.width = clamp(actualExtent.width, (*capabilities).minImageExtent.width, (*capabilities).maxImageExtent.width);
		actualExtent.height = clamp(actualExtent.height, (*capabilities).minImageExtent.height, (*capabilities).maxImageExtent.height);

		return actualExtent;
	}
}

SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.formatsCount = formatCount;
		details.formats = (VkSurfaceFormatKHR*)malloc(formatCount * sizeof(VkSurfaceFormatKHR));
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats);
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.presentModesCount = presentModeCount;
		details.presentModes = (VkPresentModeKHR*)malloc(presentModeCount * sizeof(VkPresentModeKHR));
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes);
	}

	return details;
}

bool IsDeviceSuitable(VkPhysicalDevice device) {
	QueueFamilyIndices indices = FindQueueFamilies(device);
	
	bool extensionsSupported = CheckDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
		swapChainAdequate = swapChainSupport.formatsCount > 0 && swapChainSupport.presentModesCount > 0;
	}	

	return QueueFamilyIndices_IsComplete(&indices) && extensionsSupported && swapChainAdequate;
}

bool CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    VkExtensionProperties availableExtensions[extensionCount];
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions);

	int deviceExtensionsCount = sizeof(deviceExtensions) / sizeof(deviceExtensions[0]);
	
	bool allRequiredExtensionsAvailable = true;

	for (int i = 0; i < deviceExtensionsCount; i++)
	{
		allRequiredExtensionsAvailable = false;
		for (int j = 0; j < extensionCount; j++)
		{
			if (strcmp(deviceExtensions[i], availableExtensions[j].extensionName) == 0)
			{
				allRequiredExtensionsAvailable = true;
				break;
			}
		}
		if(allRequiredExtensionsAvailable == false)
			break;
	}

    return allRequiredExtensionsAvailable;
}

void CreateLogicalDevice() {
	QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);

	VkDeviceQueueCreateInfo queueCreateInfos[QueueFamilyIndicesCount] = {};
	
	int uniqueQueueFamilyCount = 0;
	uint32_t* uniqueQueueFamilies = GetUniqueQueueFamilies(&indices, &uniqueQueueFamilyCount);

	SDL_Log("VULKAN::Unique Queue Family Count : %d\n", uniqueQueueFamilyCount);

	float queuePriority = 1.0f;
	for(int i = 0; i < uniqueQueueFamilyCount; i++)
	{
		queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfos[i].queueFamilyIndex = uniqueQueueFamilies[i];
		queueCreateInfos[i].queueCount = 1;
		queueCreateInfos[i].pQueuePriorities = &queuePriority;
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.pQueueCreateInfos = queueCreateInfos;
	createInfo.queueCreateInfoCount = uniqueQueueFamilyCount;

	createInfo.pEnabledFeatures = &deviceFeatures;
	
	createInfo.ppEnabledExtensionNames = deviceExtensions;
	createInfo.enabledExtensionCount = sizeof(deviceExtensions)/sizeof(deviceExtensions[0]);

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = sizeof(validationLayers)/sizeof(validationLayers[0]);
		createInfo.ppEnabledLayerNames = validationLayers;
	} else {
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "VULKAN::failed to create logical device!");
		exit(EXIT_FAILURE);
	}

	vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
	vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
}

QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) {
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	VkQueueFamilyProperties queueFamilies[queueFamilyCount];
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

	for (int i = 0; i < queueFamilyCount; i++) {
		if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
			indices.hasGraphicsFamilyValue = true;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (presentSupport) {
			indices.presentFamily = i;
			indices.hasPresentFamilyValue = true;
		}

		if (QueueFamilyIndices_IsComplete(&indices)) {
			break;
		}
	}

	return indices;
}

uint32_t* GetUniqueQueueFamilies(QueueFamilyIndices* indices, int *uniqueCount)
{
	uint32_t* unique = (uint32_t*)malloc(QueueFamilyIndicesCount * sizeof(uint32_t));

	for (int i = 0; i < QueueFamilyIndicesCount; i++)
	{
		bool isUnique = true;

		for (int j = 0; j < i; j++)
		{
			if (QueueFamilyIndices_GetAt(indices, i) == QueueFamilyIndices_GetAt(indices, j))
			{
				isUnique = false;
				break;
			}
		}

		if (isUnique)
			unique[(*uniqueCount)++] = QueueFamilyIndices_GetAt(indices, i);
	}
	return unique;
}

bool CheckValidationLayerSupport() {
	unsigned int layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	VkLayerProperties availableLayers[layerCount];
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);
		
	int length = sizeof(validationLayers)/sizeof(validationLayers[0]);

	for (int i = 0; i < length; i++) {
		bool layerFound = false;

		for (int j = 0; j < layerCount; j++) {
			if (strcmp(validationLayers[i], availableLayers[j].layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void ReadSPIRV(const char* fileName, uint32_t** fileContents, size_t* fileSize)
{
	FILE* filePtr = fopen(fileName, "rb");
	if (filePtr == NULL) {
		printf("Cannot open %s", fileName);
		exit(EXIT_FAILURE);
	}

	*fileSize = (size_t)GetFileSize(filePtr);
	
	if ((*fileSize) % 4 != 0) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "VULKAN::Invalid SPIR-V file. Incorrect number of bytes!\n");
		exit(EXIT_FAILURE);
	}

	*fileContents = (uint32_t*)malloc(*fileSize);

	size_t bytesRead = fread(*fileContents, 1, *fileSize, filePtr);

	if (bytesRead != *fileSize) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "VULKAN::Failed to Read File. Incorrect number of bytes read!\n");
		exit(EXIT_FAILURE);
	}

	fclose(filePtr);
}

long int GetFileSize(FILE *filePtr)
{
	fpos_t pos;
	if (fgetpos(filePtr, &pos) != 0)
	{
		printf("get position failed");
		return -1;
	}
	
	if(fseek(filePtr, 0, SEEK_END) != 0)
	{
		printf("Seek to end of file failed");
		return -1;
	}

	long int filePos = ftell(filePtr);
	if(filePos == -1L)
	{
		printf("Failed to get file position");
		return -1;
	}

	if (fsetpos(filePtr, &pos) != 0)
	{
		printf("set position failed");
		return -1;
	}

	return filePos;
}
