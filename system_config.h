/*
 * System libraries configuration
 *
 * Copyright (C) 2021, HENSOLDT Cyber GmbH
 */

#pragma once

//-----------------------------------------------------------------------------
// Debug
//-----------------------------------------------------------------------------
#if !defined(NDEBUG)
#   define Debug_Config_STANDARD_ASSERT
#   define Debug_Config_ASSERT_SELF_PTR
#else
#   define Debug_Config_DISABLE_ASSERT
#   define Debug_Config_NO_ASSERT_SELF_PTR
#endif

#define Debug_Config_LOG_LEVEL                  Debug_LOG_LEVEL_INFO
#define Debug_Config_INCLUDE_LEVEL_IN_MSG
#define Debug_Config_LOG_WITH_FILE_LINE


//-----------------------------------------------------------------------------
// Memory
//-----------------------------------------------------------------------------
#define Memory_Config_USE_STDLIB_ALLOC


//-----------------------------------------------------------------------------
// NIC Driver
//-----------------------------------------------------------------------------
#define NIC_DRIVER_RINGBUFFER_NUMBER_ELEMENTS 16

#define NIC_DRIVER_RINGBUFFER_SIZE \
    (NIC_DRIVER_RINGBUFFER_NUMBER_ELEMENTS * 4096)


//-----------------------------------------------------------------------------
// ChanMUX
//-----------------------------------------------------------------------------
#define CHANMUX_CHANNEL_NIC_1_CTRL 4
#define CHANMUX_CHANNEL_NIC_1_DATA 5


//-----------------------------------------------------------------------------
// ChanMUX client
//-----------------------------------------------------------------------------
#define CHANMUX_ID_NIC_1 101


//-----------------------------------------------------------------------------
// Filter Listener
//-----------------------------------------------------------------------------
#define FILTER_LISTENER_NUM_SOCKETS     2
#define FILTER_LISTENER_PORT            5560


//-----------------------------------------------------------------------------
// Network Stack
//-----------------------------------------------------------------------------
#define NETWORK_STACK_1_NUM_SOCKETS     FILTER_LISTENER_NUM_SOCKETS
#define ETH_1_ADDR                      "10.0.0.10"
#define ETH_1_GATEWAY_ADDR              "10.0.0.1"
#define ETH_1_SUBNET_MASK               "255.255.255.0"
