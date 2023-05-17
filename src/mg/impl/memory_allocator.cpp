
#include <assert.h>

#include "shl/debug.hpp"

#include "mg/vk_error.hpp"
#include "mg/impl/context.hpp"
#include "mg/impl/memory_allocator.hpp"

void free_memory(mg::memory_allocator *alloc, u64 i, mg::vk_memory *mem)
{
    vkFreeMemory(alloc->context->device, mem->memory, nullptr);
    
    mg::free(mem);
    ::remove_elements(&alloc->allocated_memory, i, 1);
}

void mg::init(mg::memory_allocator *alloc, mg::context *ctx)
{
    assert(alloc != nullptr);
    assert(ctx != nullptr);

    trace("setting up memory allocator %p\n", alloc);

    alloc->context = ctx;
    
    ::init(&alloc->allocated_memory);
    vkGetPhysicalDeviceMemoryProperties(ctx->physical_device, &alloc->memory_properties);
}

// methods
u32 mg::find_memory_type_index(mg::memory_allocator *alloc, VkMemoryPropertyFlags flags, u32 filter)
{
    assert(alloc != nullptr);

    for (u32 i = 0; i < alloc->memory_properties.memoryTypeCount; i++)
        if ((filter & (1 << i)) && (alloc->memory_properties.memoryTypes[i].propertyFlags & flags) == flags)
            return i;
    
    return UINT32_MAX;
}

u32 mg::find_exact_memory_type_index(mg::memory_allocator *alloc, VkMemoryPropertyFlags flags, u32 filter)
{
    assert(alloc != nullptr);

    for (u32 i = 0; i < alloc->memory_properties.memoryTypeCount; i++)
        if ((filter & (1 << i)) && alloc->memory_properties.memoryTypes[i].propertyFlags == flags)
            return i;
    
    return UINT32_MAX;
}

mg::vk_memory *mg::allocate_memory(mg::memory_allocator *alloc, VkDeviceSize size, VkMemoryPropertyFlags flag_requirements, mg::memory_binding_type binding_type, u32 filter)
{
    assert(alloc != nullptr);
    u32 i = find_exact_memory_type_index(alloc, flag_requirements, filter);
    
    return allocate_memory_by_memory_type_index(alloc, size, binding_type, i);
}

mg::vk_memory *mg::allocate_memory_by_memory_type_index(mg::memory_allocator *alloc, VkDeviceSize size, mg::memory_binding_type binding_type, u32 index)
{
    assert(alloc != nullptr);
    assert(index != UINT32_MAX);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = size;
    allocInfo.memoryTypeIndex = index;
    
    VkDeviceMemory vkmem;
    VkResult res = vkAllocateMemory(alloc->context->device, &allocInfo, nullptr, &vkmem);

    if (res != VK_SUCCESS)
        throw_vk_error(res, "%p failed to allocate memory", alloc);
    
    const auto flag_bits = alloc->memory_properties.memoryTypes[index].propertyFlags;
    
    // get the first already allocated memory of size > than current memory
    // and insert before it.
    for_list(i, mem, &alloc->allocated_memory)
        if (mem->size > size)
            break;

    auto *node = ::insert_elements(&alloc->allocated_memory, i, 1);
    mg::init(&node->value, vkmem, size, flag_bits, binding_type, index);
    
    return &node->value;
}

mg::vk_memory *mg::allocate_host_memory(mg::memory_allocator *alloc, VkDeviceSize size, mg::memory_binding_type binding_type, u32 filter)
{
    assert(alloc != nullptr);
    u32 i = find_memory_type_index(alloc, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, filter);
    
    return allocate_memory_by_memory_type_index(alloc, size, binding_type, i);
}

mg::vk_memory *mg::allocate_host_coherent_memory(mg::memory_allocator *alloc, VkDeviceSize size, mg::memory_binding_type binding_type, u32 filter)
{
    assert(alloc != nullptr);
    u32 i = find_memory_type_index(alloc, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, filter);
    
    return allocate_memory_by_memory_type_index(alloc, size, binding_type, i);
}

mg::vk_memory *mg::allocate_local_memory(mg::memory_allocator *alloc, VkDeviceSize size, mg::memory_binding_type binding_type, u32 filter)
{
    assert(alloc != nullptr);
    u32 i = find_memory_type_index(alloc, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, filter);
    
    return allocate_memory_by_memory_type_index(alloc, size, binding_type, i);
}

void mg::free_memory(mg::memory_allocator *alloc, mg::vk_memory *mem)
{
    assert(alloc != nullptr);

    for_list(i, amem, &alloc->allocated_memory)
        if (amem->memory == mem->memory)
        {
            ::free_memory(alloc, i, amem);
            break;
        }
}

void mg::free_all_memory(mg::memory_allocator *alloc)
{
    assert(alloc != nullptr);
    assert(alloc->context != nullptr);
    
    for_list(mem, &alloc->allocated_memory)
        vkFreeMemory(alloc->context->device, mem->memory, nullptr);
    
    ::clear(&alloc->allocated_memory);
}

void mg::free(mg::memory_allocator *alloc)
{
    assert(alloc != nullptr);

    mg::free_all_memory(alloc);

    ::free(&alloc->allocated_memory);
    alloc->context = nullptr;
}

