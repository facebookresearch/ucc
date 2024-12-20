/**
 * Copyright (c) 2020-2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 *
 * See file LICENSE for terms.
 */

#include "cl_doca_urom.h"
#include "utils/ucc_malloc.h"
#include "components/tl/ucc_tl.h"
#include "core/ucc_global_opts.h"
#include "utils/ucc_math.h"

#include <doca_log.h>

/* NOLINTNEXTLINE  TODO params is not used*/
UCC_CLASS_INIT_FUNC(ucc_cl_doca_urom_lib_t, const ucc_base_lib_params_t *params,
                    const ucc_base_config_t *config)
{
    const ucc_cl_doca_urom_lib_config_t *cl_config =
        ucc_derived_of(config, ucc_cl_doca_urom_lib_config_t);

    UCC_CLASS_CALL_SUPER_INIT(ucc_cl_lib_t, &ucc_cl_doca_urom.super,
                              &cl_config->super);
    memcpy(&self->cfg, cl_config, sizeof(*cl_config));

    cl_debug(&self->super, "initialized lib object: %p", self);

    return UCC_OK;
}

UCC_CLASS_CLEANUP_FUNC(ucc_cl_doca_urom_lib_t)
{
    cl_debug(&self->super, "finalizing lib object: %p", self);
}

UCC_CLASS_DEFINE(ucc_cl_doca_urom_lib_t, ucc_cl_lib_t);
static inline ucc_status_t check_tl_lib_attr(const ucc_base_lib_t *lib,
                                             ucc_tl_iface_t       *tl_iface,
                                             ucc_cl_lib_attr_t    *attr)
{
    ucc_tl_lib_attr_t tl_attr;
    ucc_status_t      status;

    memset(&tl_attr, 0, sizeof(tl_attr));
    status = tl_iface->lib.get_attr(NULL, &tl_attr.super);
    if (UCC_OK != status) {
        cl_error(lib, "failed to query tl %s lib attributes",
                 tl_iface->super.name);
        return status;
    }

    attr->super.attr.thread_mode =
        ucc_min(attr->super.attr.thread_mode, tl_attr.super.attr.thread_mode);
    attr->super.attr.coll_types |= tl_attr.super.attr.coll_types;
    attr->super.flags           |= tl_attr.super.flags;

    return UCC_OK;
}

ucc_status_t ucc_cl_doca_urom_get_lib_attr(const ucc_base_lib_t *lib,
                                           ucc_base_lib_attr_t  *base_attr)
{
    ucc_cl_doca_urom_lib_t  *cl_lib = ucc_derived_of(lib, ucc_cl_doca_urom_lib_t);
    ucc_cl_lib_attr_t       *attr   = ucc_derived_of(base_attr, ucc_cl_lib_attr_t);
    ucc_status_t             status;

    attr->tls = &cl_lib->super.tls.array;

    if (cl_lib->super.tls.requested) {
        status = ucc_config_names_array_dup(&cl_lib->super.tls_forced,
                                            &cl_lib->super.tls.array);
        if (UCC_OK != status) {
            return status;
        }
    }

    attr->tls_forced             = &cl_lib->super.tls_forced;
    attr->super.attr.thread_mode = UCC_THREAD_MULTIPLE;
    attr->super.attr.coll_types  = UCC_COLL_TYPE_ALLREDUCE;
    attr->super.flags            = 0;

    ucc_assert(tls->array.count >= 1);

    return UCC_OK;
}

ucc_status_t ucc_cl_doca_urom_get_lib_properties(ucc_base_lib_properties_t *prop)
{
    prop->default_team_size = 2;
    prop->min_team_size     = 2;
    prop->max_team_size     = UCC_RANK_MAX;

    return UCC_OK;
}
