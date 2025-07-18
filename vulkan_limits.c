#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <stdatomic.h>
#include <unistd.h>

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

static VkInstance create_instance(void)
{
    static const VkApplicationInfo ainfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Vulkan Limits Test",
        .applicationVersion = 1,
        .engineVersion = 1,
        .apiVersion = VK_API_VERSION_1_0,
    };

    static const VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &ainfo,
    };

    VkInstance instance;
    VkResult res = vkCreateInstance(&create_info, NULL, &instance);
    if (res != VK_SUCCESS) {
        fprintf(stderr, "Failed to create Vulkan instance: %s\n", to_str(res));
        exit(1);
    }

    return instance;
}

static VkPhysicalDevice get_phys_device(VkInstance instance)
{
    uint32_t count = 1;
    VkPhysicalDevice device;
    vkEnumeratePhysicalDevices(instance, &count, &device);

    if (!count) {
        fprintf(stderr, "No physical devices found.\n");
        exit(2);
    }

    return device;
}

static VkDevice create_device(VkPhysicalDevice phys_device)
{
    static const float priorities[16] = {1.0f};
    static const VkDeviceQueueCreateInfo qinfos[] = {
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pQueuePriorities = priorities,
            .queueFamilyIndex = 0,
            .queueCount = 16,
        }
    };

    static const VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = sizeof(qinfos) / sizeof(qinfos[0]),
        .pQueueCreateInfos = qinfos,
    };

    VkDevice device;
    VkResult res = vkCreateDevice(phys_device, &device_info, NULL, &device);
    if (res != VK_SUCCESS) {
        fprintf(stderr, "Failed to create device: %s\n", to_str(res));
        exit(1);
    }

    return device;
}

static int test_instance(void)
{
    printf("Trying to exhaust instance creation limit...\n");
    for (int num = 0;;) {
        create_instance();
        printf("  %d\n", ++num);
    }
}

static int test_device(void)
{
    VkInstance instance = create_instance();
    VkPhysicalDevice physical_device = get_phys_device(instance);

    printf("Trying to exhaust device creation limit...\n");
    for (int i = 0;;) {
        create_device(physical_device);
        printf("  %d\n", ++i);
    }
}

static int test_both(void)
{
    printf("Trying to exhaust combined instance+device creation limit...\n");
    for (int i = 0;;) {
        VkInstance instance = create_instance();
        VkPhysicalDevice physical_device = get_phys_device(instance);
        create_device(physical_device);
        printf("  %d\n", ++i);
    }
}

static int device_thread(void *arg)
{
    atomic_int *num = arg;
    create_device(get_phys_device(create_instance()));
    printf("  %d\n", atomic_fetch_add(num, 1) + 1);

    return pause();
}

static int test_threads(void)
{
    printf("Trying to exhaust threaded device creation limit...\n");
    for (atomic_int num = 0;;) {
        thrd_t thread;
        if (thrd_create(&thread, device_thread, &num) != thrd_success) {
            fprintf(stderr, "Failed to create thread.\n");
            exit(2);
        }

        thrd_detach(thread);
    }
};

static int test_hold(void)
{
    create_device(get_phys_device(create_instance()));
    printf("Holding device...\n");
    return pause();
}

static int test_memory(void)
{
    VkDevice device = create_device(get_phys_device(create_instance()));

    for (size_t size = 1; size; size <<= 1) {
        VkDeviceMemory mem;
        const VkMemoryAllocateInfo ainfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize  = size,
            .memoryTypeIndex = 0,
        };

        printf("Trying to allocate %zu bytes of memory...", size);
        VkResult res = vkAllocateMemory(device, &ainfo, NULL, &mem);
        if (res == VK_SUCCESS) {
            printf(" success\n");
        } else {
            printf(" failed: %s\n", to_str(res));
            return 1;
        }
    }

    return 0;
}

int main(const int argc, const char *const argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <instance|device|combined>\n", argv[0]);
        return 1;
    }

    const char *test = argv[1];
    if (!strcmp(test, "instance")) {
        return test_instance();
    } else if (!strcmp(test, "device")) {
        return test_device();
    } else if (!strcmp(test, "combined")) {
        return test_both();
    } else if (!strcmp(test, "threads")) {
        return test_threads();
    } else if (!strcmp(test, "hold")) {
        return test_hold();
    } else if (!strcmp(test, "memory")) {
        return test_memory();
    } else {
        fprintf(stderr, "Unknown test: %s\n", test);
        return 1;
    }

    return 0;
}
