#include <stdlib.h>

#include "vulkan.h"

#ifndef VK_EXT_DEBUG_REPORT_EXTENSION_NAME
#define VK_EXT_DEBUG_REPORT_EXTENSION_NAME "VK_EXT_debug_report"
#endif

typedef struct QueueFamilyIndices {
    uint32_t graphicsFamily;
    bool isComplete;
} QueueFamilyIndices;

const char* const validationLayers[] = {
	"VK_LAYER_KHRONOS_validation"
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

VkInstance instance;
VkDebugUtilsMessengerEXT debugMessenger;
VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;


void InitializeVulkan(const char* appName)
{
	CreateInstance(appName);
	SetupDebugMessenger();
	PickPhysicalDevice();
}

void CleanUpVulkan()
{
	if (enableValidationLayers) {
		DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }	
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

void PickPhysicalDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "failed to find GPUs with Vulkan support!");
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
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "failed to find a suitable GPU!");
		exit(EXIT_FAILURE);
	}
}
bool IsDeviceSuitable(VkPhysicalDevice device) {
	QueueFamilyIndices indices = FindQueueFamilies(device);

	return indices.isComplete;
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
			indices.isComplete = true;
			break;
		}
	}

	return indices;
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
