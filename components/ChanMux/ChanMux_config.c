/*
 * Channel MUX
 *
 * Copyright (C) 2021-2024, HENSOLDT Cyber GmbH
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * For commercial licensing, contact: info.cyber@hensoldt.net
 */

#include "system_config.h"
#include "ChanMux/ChanMux.h"
#include "ChanMuxNic.h"
#include <camkes.h>

//------------------------------------------------------------------------------
static unsigned int
resolveChannel(
    unsigned int sender_id,
    unsigned int chanNum_local)
{
    // TODO: This function is supposed to map a "local" channel number passed by
    // a sender to a "global" channel number used by the Proxy. Goal is, that
    // only ChanMUX is aware of the global channel number that the proxy needs.
    // All components should just use local channel numbers. Unfortunately, the
    // NIC protocol needs to be changed first, because is still uses global
    // channel numbers in the NIC_OPEN command. This is a legacy from the time
    // there the control channel was shared for multiple NICs, we do not plan to
    // use this any longer. For now this function does not implement any
    // mapping, but it does some access control at least. Component can only use
    // their channel numbers. We do not look into the protocol, thus rough NIC
    // drivers may still use anything in the NIC_OPEN command.

    switch (sender_id)
    {
    //----------------------------------
    case CHANMUX_ID_NIC:
        switch (chanNum_local)
        {
        //----------------------------------
        case CHANMUX_CHANNEL_NIC_CTRL: // TODO: Use local channel number.
            return CHANMUX_CHANNEL_NIC_CTRL;
        //----------------------------------
        case CHANMUX_CHANNEL_NIC_DATA: // TODO: Use local channel number.
            return CHANMUX_CHANNEL_NIC_DATA;
        //----------------------------------
        default:
            break;
        }
        break;
    //----------------------------------
    default:
        break;
    }

    return INVALID_CHANNEL;
}

//------------------------------------------------------------------------------
static struct
{
    uint8_t ctrl[128];
    // FIFO is big enough to store 1 minute of network "background" traffic.
    // Value found by manual testing, may differ in less noisy networks.
    uint8_t data[1024 * PAGE_SIZE];
} nic_fifo[2];

static struct
{
    ChanMux_Channel_t ctrl;
    ChanMux_Channel_t data;
} nic_channel[2];

//------------------------------------------------------------------------------
static const ChanMux_ChannelCtx_t channelCtx[] =
{

    CHANNELS_CTX_NIC_CTRL_DATA(
        CHANMUX_CHANNEL_NIC_CTRL,
        CHANMUX_CHANNEL_NIC_DATA,
        0,
        nwDriver_ctrl_portRead,
        nwDriver_ctrl_portWrite,
        nwDriver_data_portRead,
        nwDriver_data_portWrite,
        nwDriver_ctrl_eventHasData_emit,
        nwDriver_data_eventHasData_emit)

};

//------------------------------------------------------------------------------
// This is used by the ChanMux component.
const ChanMux_Config_t cfgChanMux =
{
    .resolveChannel = &resolveChannel,
    .numChannels    = ARRAY_SIZE(channelCtx),
    .channelCtx     = channelCtx
};
