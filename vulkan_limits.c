#include <stdio.h>
#include <string.h>

#include <libplacebo/vulkan.h>
#include <vulkan/vulkan.h>

static const char *to_str(VkResult res)
{
    switch (res) {
    case VK_SUCCESS: return "VK_SUCCESS";
    case VK_NOT_READY: return "VK_NOT_READY";
    case VK_TIMEOUT: return "VK_TIMEOUT";
    case VK_EVENT_SET: return "VK_EVENT_SET";
    case VK_EVENT_RESET: return "VK_EVENT_RESET";
    case VK_INCOMPLETE: return "VK_INCOMPLETE";
    case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
    case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
    case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
    case VK_ERROR_UNKNOWN: return "VK_ERROR_UNKNOWN";
    default:  return "Unknown error";
    }
}

static int test_instance(void)
{
    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &(VkApplicationInfo) {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "Vulkan Limits Test",
            .applicationVersion = 1,
            .engineVersion = 1,
            .apiVersion = VK_API_VERSION_1_0,
        },
    };

    printf("Trying to exhaust instance creation limit...\n");
    for (int i = 0;;) {
        VkInstance instance;
        VkResult res = vkCreateInstance(&create_info, NULL, &instance);
        switch (res) {
        case VK_SUCCESS:
            printf("  %d\n", ++i);
            break;
        default:
            printf("Failed after %d instances: %s\n", i, to_str(res));
            return 1;
        }
    }
}

static int test_device(void)
{
    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &(VkApplicationInfo) {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "Vulkan Limits Test",
            .applicationVersion = 1,
            .engineVersion = 1,
            .apiVersion = VK_API_VERSION_1_0,
        },
    };

    VkInstance instance;
    VkResult res = vkCreateInstance(&create_info, NULL, &instance);
    if (res != VK_SUCCESS) {
        printf("Failed to create instance: %s\n", to_str(res));
        return 1;
    }

    uint32_t count = 1;
    VkPhysicalDevice physical_device;
    vkEnumeratePhysicalDevices(instance, &count, &physical_device);

    if (!count) {
        printf("No physical devices found.\n");
        vkDestroyInstance(instance, NULL);
        return 1;
    }

    VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = &(VkDeviceQueueCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = 0,
            .queueCount = 1,
            .pQueuePriorities = (float[]){1.0f},
        },
        .enabledExtensionCount = 0,
    };

    printf("Trying to exhaust device creation limit...\n");
    for (int i = 0;;) {
        VkDevice device;
        VkResult res = vkCreateDevice(physical_device, &device_create_info, NULL, &device);
        switch (res) {
        case VK_SUCCESS:
            printf("  %d\n", ++i);
            break;
        default:
            printf("Failed after %d devices: %s\n", i, to_str(res));
            return 1;
        }
    }
}

static int test_both(void)
{
    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &(VkApplicationInfo) {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "Vulkan Limits Test",
            .applicationVersion = 1,
            .engineVersion = 1,
            .apiVersion = VK_API_VERSION_1_0,
        },
    };

    VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = &(VkDeviceQueueCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = 0,
            .queueCount = 1,
            .pQueuePriorities = (float[]){1.0f},
        },
        .enabledExtensionCount = 0,
    };

    printf("Trying to exhaust combined instance+device creation limit...\n");
    for (int i = 0;;) {
        VkInstance instance;
        VkResult res = vkCreateInstance(&create_info, NULL, &instance);
        if (res != VK_SUCCESS) {
            printf("Failed to create instance after %d iterations: %s\n", i, to_str(res));
            return 1;
        }

        uint32_t count = 1;
        VkPhysicalDevice physical_device;
        vkEnumeratePhysicalDevices(instance, &count, &physical_device);
        if (!count) {
            printf("No physical devices found.\n");
            return 1;
        }

        VkDevice device;
        res = vkCreateDevice(physical_device, &device_create_info, NULL, &device);
        switch (res) {
        case VK_SUCCESS:
            printf("  %d\n", ++i);
            break;
        default:
            printf("Failed to create device after %d iterations: %s\n", i, to_str(res));
            return 1;
        }
    }
}

static int test_pl_vulkan(void)
{
    pl_log log = pl_log_create(PL_API_VER, pl_log_params(
        .log_level = PL_LOG_WARN,
        .log_cb    = pl_log_color,
    ));

    pl_vk_inst inst = pl_vk_inst_create(log, pl_vk_inst_params(
        .debug = false,
    ));

    printf("Trying to exhaust pl_vulkan creation limit...\n");
    for (int i = 0;;) {
        pl_vulkan vulkan = pl_vulkan_create(log, pl_vulkan_params(
            .instance = inst->instance,
        ));

        if (!vulkan) {
            printf("Failed after %d instances\n", i);
            pl_log_destroy(&log);
            return 1;
        } else {
            printf("  %d\n", ++i);
        }
    }
}


int main(const int argc, const char *const argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <instance>\n", argv[0]);
        return 1;
    }

    const char *test = argv[1];
    if (!strcmp(test, "instance")) {
        return test_instance();
    } else if (!strcmp(test, "device")) {
        return test_device();
    } else if (!strcmp(test, "both")) {
        return test_both();
    } else if (!strcmp(test, "pl_vulkan")) {
        return test_pl_vulkan();
    } else {
        fprintf(stderr, "Unknown test: %s\n", test);
        return 1;
    }

    return 0;
}
