#
# Demo TLS Server
#
# Copyright (C) 2021, HENSOLDT Cyber GmbH
#

cmake_minimum_required(VERSION 3.17)

os_set_config_file(
    "system_config.h"
    CONFIG_PROJECT_NAME "system_config"
)

#-------------------------------------------------------------------------------

project(demo_tls_server C)

# Overwrite the default log level of the lower layers to ERROR as the output
# otherwise gets too cluttered with debug prints.
set(LibUtilsDefaultZfLogLevel 5 CACHE STRING "" FORCE)

include("plat/${PLATFORM}/plat_nic.cmake")

DeclareCAmkESComponent(
    TlsServer
    INCLUDES
        components/TlsServer/include
    SOURCES
        components/TlsServer/src/TlsServer.c
    C_FLAGS
        -Wall -Werror
    LIBS
        system_config
        os_core_api
        lib_compiler
        lib_debug
        os_socket_client
        3rdparty_mbedtls_for_tls_server
)

NetworkStack_PicoTcp_DeclareCAmkESComponent(
    NwStack
    C_FLAGS
        -DNetworkStack_PicoTcp_USE_HARDCODED_IPADDR
        -DDEV_ADDR=ETH_ADDR
        -DGATEWAY_ADDR=ETH_GATEWAY_ADDR
        -DSUBNET_MASK=ETH_SUBNET_MASK
        -DOS_NETWORK_MAXIMUM_SOCKET_NO=NETWORK_STACK_NUM_SOCKETS
)

TimeServer_DeclareCAmkESComponent(
    TimeServer
)

EntropySource_DeclareCAmkESComponent(
    EntropySource
)

DeclareAndCreateCamkESSystem(DemoTlsServer.camkes)
GenerateSimulateScript()
