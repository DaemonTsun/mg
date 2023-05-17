
#pragma once

#include <vulkan/vulkan_core.h>

#include "shl/number_types.hpp"
#include "shl/linked_list.hpp"
#include "mg/impl/vk_memory.hpp"

namespace mg
{
typedef linked_list<vk_memory> memory_list;

struct memory_allocator
{
    mg::memory_list allocated_memory;
    mg::context *context;
    VkPhysicalDeviceMemoryProperties memory_properties;
};

void init(mg::memory_allocator *alloc, mg::context *ctx);
u32 find_memory_type_index(mg::memory_allocator *alloc, VkMemoryPropertyFlags flags, u32 filter = UINT32_MAX);
u32 find_exact_memory_type_index(mg::memory_allocator *alloc, VkMemoryPropertyFlags flags, u32 filter = UINT32_MAX);

mg::vk_memory *allocate_memory(mg::memory_allocator *alloc, VkDeviceSize size, VkMemoryPropertyFlags flag_requirements, mg::memory_binding_type binding_type, u32 filter = UINT32_MAX);
mg::vk_memory *allocate_memory_by_memory_type_index(mg::memory_allocator *alloc, VkDeviceSize size, mg::memory_binding_type binding_type, u32 memory_type_index);
mg::vk_memory *allocate_host_memory(mg::memory_allocator *alloc, VkDeviceSize size, mg::memory_binding_type binding_type, u32 filter = UINT32_MAX); // CPU visible memory
mg::vk_memory *allocate_host_coherent_memory(mg::memory_allocator *alloc, VkDeviceSize size, mg::memory_binding_type binding_type, u32 filter = UINT32_MAX);
mg::vk_memory *allocate_local_memory(mg::memory_allocator *alloc, VkDeviceSize size, mg::memory_binding_type binding_type, u32 filter = UINT32_MAX); // GPU memory

void free_memory(mg::memory_allocator *alloc, vk_memory *mem);
void free_all_memory(mg::memory_allocator *alloc);

void free(mg::memory_allocator *alloc);
}
