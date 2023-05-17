
#include <assert.h>

#include "shl/memory.hpp"
#include "shl/defer.hpp"
#include "shl/debug.hpp"

#include "mg/vk_error.hpp"
#include "mg/impl/context.hpp"
#include "mg/impl/descriptor_pool_manager.hpp"

void mg::init(mg::descriptor_pool_manager *mgr, mg::context *ctx)
{
    assert(mgr != nullptr);
    assert(ctx != nullptr);

    mgr->context = ctx;

    ::init(&mgr->descriptor_pools);
}

void mg::free(mg::descriptor_pool_manager *mgr)
{
    assert(mgr != nullptr);
    assert(mgr->context != nullptr);

    for_array(unman, &mgr->descriptor_pools)
        vkDestroyDescriptorPool(mgr->context->device, *unman, nullptr);

    ::free(&mgr->descriptor_pools);
}

void mg::create_descriptor_pool(mg::descriptor_pool_manager *mgr, VkDescriptorPoolCreateInfo *info, VkDescriptorPool *out)
{
    assert(mgr != nullptr);
    assert(info != nullptr);

    VkDescriptorPool descriptorPool;
    VkResult res = vkCreateDescriptorPool(mgr->context->device, info, nullptr, &descriptorPool);

    if (res != VK_SUCCESS)
        throw_vk_error(res, "%p could not create descriptor pool", mgr);

    *out = descriptorPool;
    ::add_at_end(&mgr->descriptor_pools, descriptorPool);
}

void mg::reset_pools(mg::descriptor_pool_manager *mgr)
{
    assert(mgr != nullptr);
    assert(mgr->context != nullptr);

    for_array(pool, &mgr->descriptor_pools)
        vkResetDescriptorPool(mgr->context->device, *pool, /*unused flags*/ 0);
}

