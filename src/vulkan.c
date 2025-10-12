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
VkFormat swapChainImageFormat;
VkExtent2D swapChainExtent;



void InitializeVulkan(const char* appName, SDL_Window* sdl_window)
{
	window = sdl_window;

	CreateInstance(appName);
	SetupDebugMessenger();
	CreateSurface();
	PickPhysicalDevice();
	CreateLogicalDevice();
	CreateSwapChain();
}

void CleanUpVulkan()
{
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
