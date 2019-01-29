#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <iostream>
#include <vector>
#include <optional>

const int kWindowWidth = 800;
const int kWindowHeight = 600;

class Application {
   public:
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphics_family;

        bool isComplete() {
            return graphics_family.has_value();
        }
    };

    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

   private:
    void initWindow() {
#ifdef NDEBUG
        std::cout << "Runing in RELEASE mode.\n";
#else
        std::cout << "Runing in DEBUG mode.\n";
#endif
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);  // tells GLFW to not create an OpenGL context
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);    // no resizable window
        glfw_window_ = glfwCreateWindow(kWindowWidth, kWindowHeight, "Vulkan", nullptr, nullptr);
    }

    void initVulkan() {
        createInstance();
        setupDebugMessenger();
        pickPhysicalDevice();
        createLogicalDevice();
    }

    void createInstance() {
        if (kEnableValidationLayers_ && !checkValidationLayerSupport())
            throw std::runtime_error("Validation layers requested, but not available!");

        std::cout << "Validation layers available." << std::endl;

        VkApplicationInfo app_info = {};  // optional struct (may help the driver optimize it)
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "Hello Triangle";
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName = "No Engine";
        app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo create_info = {};  // this struct is obligatory
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;

        auto extensions = getRequiredExtensions();
        create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        create_info.ppEnabledExtensionNames = extensions.data();

        create_info.enabledLayerCount = 0;  // validation layers to enable

        if (kEnableValidationLayers_) {
            create_info.enabledLayerCount = static_cast<uint32_t>(kValidationLayers_.size());
            create_info.ppEnabledLayerNames = kValidationLayers_.data();
        } else
            create_info.enabledLayerCount = 0;

        if (vkCreateInstance(&create_info, nullptr, &vk_instance_) != VK_SUCCESS)
            throw std::runtime_error("failed to create instance!");

        std::cout << "Vulkan instance successfully created." << std::endl;
    }

    bool checkValidationLayerSupport() {
        uint32_t layer_count;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

        std::vector<VkLayerProperties> available_layers(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

        std::cout << "Vulkan supported layers (" << layer_count << "):" << std::endl;
        for (const auto& available_layers : available_layers)
            std::cout << "\t" << available_layers.layerName << std::endl;

        for (const char* layer_name : kValidationLayers_) {
            bool layer_found = false;

            for (const auto& layer_properties : available_layers)
                if (strcmp(layer_name, layer_properties.layerName) == 0) {
                    layer_found = true;
                    break;
                }

            if (!layer_found) { return false; }
        }

        return true;
    }

    // Returns the list of available extensions based on whether validation layers are enabled or not.
    // In this case, the function will always load the GLFW required extensions and will optionally load the
    // Vulkan debug extensions (e.g. to print validation layers debug messages.)
    std::vector<const char*> getRequiredExtensions() {
        uint32_t glfw_extension_count = 0;
        const char** glfw_extensions;
        glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

        std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

        if (kEnableValidationLayers_) {
            // The macro bellow corresponds to the string 'VK_EXT_debug_utils'.
            // This extension will be necessary, for instance, to register the debug messenger callback debugCallBack().
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        uint32_t extension_count = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
        std::vector<VkExtensionProperties> supported_extensions(extension_count);
        vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, supported_extensions.data());
        std::cout << "Vulkan supported instance extentensions (" << extension_count << "):" << std::endl;

        for (const auto& supported_extension : supported_extensions) {
           std::cout << "\t" << supported_extension.extensionName << std::endl;
        }

        std::cout << "Required instance extentensions (" << extensions.size() << "):" << std::endl;
        for (const auto& extension : extensions) {
           std::cout << "\t" << extension << std::endl;
        }

        return extensions;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT message_type,
        const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
        void* p_user_data) {

        std::cerr << "validation layer: " << p_callback_data->pMessage << std::endl;
        return VK_FALSE;
    }


    // This function actually calls vkCreateDebugUtilsMessengerEXT() (which is an extension function and whose address has to
    // be obtained at runtime) in order to register the debug messenger callback debugCallback().
    // The function vkCreateDebugUtilsMessengerEXT() creates a VkDebugUtilsMessengerEXT object.
    // The function returns VK_ERROR_EXTENSION_NOT_PRESENT if the address of the registering function cannot be obtained.
    static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else
            return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    // Cleans up the resources associated to the VkDebugUtilsMessengerEXT object.
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr)
            func(instance, debugMessenger, pAllocator);
    }

    // Register the debugCallback() messenger function with Vulkan.
    void setupDebugMessenger() {
        if (!kEnableValidationLayers_) return;

        VkDebugUtilsMessengerCreateInfoEXT create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        create_info.pfnUserCallback = debugCallback; // the callback function
        create_info.pUserData = nullptr; // Optional

        if (CreateDebugUtilsMessengerEXT(vk_instance_, &create_info, nullptr, &debug_messenger_func_) != VK_SUCCESS)
            throw std::runtime_error("failed to set up debug messenger!");

        std::cout << "Debug messenger successfully set up." << std::endl;
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

        int i = 0;
        for (const auto& queue_family : queue_families) {
            if (queue_family.queueCount > 0 && queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                indices.graphics_family = i;

            if (indices.isComplete()) break;

            ++i;
        }

        return indices;
    }

    bool isDeviceSuitable(VkPhysicalDevice device) {
        VkPhysicalDeviceProperties device_properties;
        //VkPhysicalDeviceFeatures device_features;
        vkGetPhysicalDeviceProperties(device, &device_properties);
        //vkGetPhysicalDeviceFeatures(device, &device_features);

        QueueFamilyIndices indices = findQueueFamilies(device);

        return indices.isComplete() && (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
    }

    void pickPhysicalDevice() {
        uint32_t device_count = 0;
        vkEnumeratePhysicalDevices(vk_instance_, &device_count, nullptr);

        if (device_count == 0)
            throw std::runtime_error("failed to find GPUs with Vulkan support!");

        std::cout << "Vulkan-enabled GPU successfully found." << std::endl;

        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(vk_instance_, &device_count, devices.data());
        std::cout << "Number of Vulkan capable devices: " << devices.size() << std::endl;

        // picks the last suitable device (not necessarily the best one)
        for (const auto& device : devices)
            if (isDeviceSuitable(device)) {
                physical_device_ = device;
                break;
            }

        if (physical_device_ == VK_NULL_HANDLE)
            throw std::runtime_error("failed to find a suitable GPU!");

        std::cout << "GPU is suitable for graphics." << std::endl;
    }

    void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(physical_device_);

        VkDeviceQueueCreateInfo queue_create_info = {};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = indices.graphics_family.value();
        queue_create_info.queueCount = 1;

        float queue_priority = 1.0f;
        queue_create_info.pQueuePriorities = &queue_priority; // must be set even for a unique queue

        VkPhysicalDeviceFeatures device_features = {};

        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.pQueueCreateInfos = &queue_create_info;
        create_info.queueCreateInfoCount = 1;
        create_info.pEnabledFeatures = &device_features;

        create_info.enabledExtensionCount = 0;

        if (kEnableValidationLayers_) {
            create_info.enabledLayerCount = static_cast<uint32_t>(kValidationLayers_.size());
            create_info.ppEnabledLayerNames = kValidationLayers_.data();
        } else
            create_info.enabledLayerCount = 0;

        if (vkCreateDevice(physical_device_, &create_info, nullptr, &device_) != VK_SUCCESS)
            throw std::runtime_error("failed to create logical device!");

        std::cout << "Logical device successfully created." << std::endl;

        // creates only one queue (index 0) for the informed queue family.
        vkGetDeviceQueue(device_, indices.graphics_family.value(), 0, &graphics_queue_);
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(glfw_window_)) { glfwPollEvents(); }
    }

    void cleanup() {
        vkDestroyDevice(device_, nullptr); // associated queues are implicitly cleaned up.

        if (kEnableValidationLayers_)
            DestroyDebugUtilsMessengerEXT(vk_instance_, debug_messenger_func_, nullptr);

        vkDestroyInstance(vk_instance_, nullptr);
        glfwDestroyWindow(glfw_window_);
        glfwTerminate();
    }

    GLFWwindow* glfw_window_;
    VkInstance vk_instance_;
    VkDebugUtilsMessengerEXT debug_messenger_func_;

    // Standard validation layer that comes with LunarG Vulkan SDK
    const std::vector<const char*> kValidationLayers_ = {"VK_LAYER_LUNARG_standard_validation"};

    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VkDevice device_;
    VkQueue graphics_queue_;

#ifdef NDEBUG
    const bool kEnableValidationLayers_ = false;
#else
    const bool kEnableValidationLayers_ = true;
#endif

};

int main() {
    Application app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
