/* Copyright 2021 Columbia University SLD Group */

#ifndef __AXI_TRAITS_HPP__
#define __AXI_TRAITS_HPP__

/* AXI4 - Configuration Traits */

#include "cynw_axi4.h"

struct dma_if_width_traits : public axi4::axi4_width_traits_default
{
    static const unsigned ID_W       = ID_WIDTH;
    static const unsigned ADDR_W     = ADDR_WIDTH;
    static const unsigned data_bytes = DATA_WIDTH / 8;
};

typedef axi4::axi4_traits_template< dma_if_width_traits > dma_if_traits;

typedef axi4::axi4_multi_rw_adaptor< dma_if_traits > dma_adaptor_t;

typedef axi4::axi4_initiator< dma_if_traits > dma_initiator_t;

typedef axi4::axi4_target< dma_if_traits > dma_target_t;

/* AXI4-LITE - Configuration Traits */

#include "cynw_axi4_lite.h"

struct reg_if_width_traits : public axi4_lite::axi4_lite_width_traits
{
    static const int addr_bytes = 4;
    static const int data_bytes = 4;
};

typedef axi4_lite::axi4_lite_traits_template< reg_if_width_traits > reg_if_traits;

typedef axi4_lite::axi4_lite_single_rw_adaptor< reg_if_traits > reg_adaptor_t;

typedef axi4_lite::axi4_lite_initiator< reg_if_traits > reg_initiator_t;

typedef axi4_lite::axi4_lite_target< reg_if_traits > reg_target_t;

#endif /* __AXI_TRAITS_HPP__ */
