
#pragma once

#include "shl/fixed_array.hpp"
#include "shl/array.hpp"
#include "shl/linked_list.hpp"
#include "shl/number_types.hpp"

namespace mg
{
struct context;

struct descriptor_pool_type_size_weight
{
    VkDescriptorType type;
    float weight;
};

struct descriptor_pool_manager
{
    mg::context *context;

    array<VkDescriptorPool> descriptor_pools;
};

void init(mg::descriptor_pool_manager *mgr, mg::context *ctx);
void free(mg::descriptor_pool_manager *mgr);

void create_descriptor_pool(mg::descriptor_pool_manager *mgr, VkDescriptorPoolCreateInfo *info, VkDescriptorPool *out);

// make sure descriptors are not being used currently in a frame (VkWaitForFences)
void reset_pools(mg::descriptor_pool_manager *mgr);
};

