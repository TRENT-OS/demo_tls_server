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

# TODO: This is a workaround to build this tempoary demo without modifying the
# mbedTLS library within the SDK. Usually, the mbedTLS library should be defined
# within the CMakeLists of mbedTLS (like the other mbedTLS libraries).

project(mbedtls_for_tls_server C)

# Relative path to the mbedTLS library.
set(MBEDTLS_PATH_RELATIVE "../../../seos_sandbox/libs/3rdParty/mbedtls")

# Sources for TLS server builds
set(SOURCES_TLS_SERVER
    "${MBEDTLS_PATH_RELATIVE}/library/aes.c"
    "${MBEDTLS_PATH_RELATIVE}/library/asn1parse.c"
    "${MBEDTLS_PATH_RELATIVE}/library/base64.c"
    "${MBEDTLS_PATH_RELATIVE}/library/bignum.c"
    "${MBEDTLS_PATH_RELATIVE}/library/cipher_wrap.c"
    "${MBEDTLS_PATH_RELATIVE}/library/cipher.c"
    "${MBEDTLS_PATH_RELATIVE}/library/ctr_drbg.c"
    "${MBEDTLS_PATH_RELATIVE}/library/debug.c"
    "${MBEDTLS_PATH_RELATIVE}/library/dhm.c"
    "${MBEDTLS_PATH_RELATIVE}/library/ecdh.c"
    "${MBEDTLS_PATH_RELATIVE}/library/ecp_curves.c"
    "${MBEDTLS_PATH_RELATIVE}/library/ecp.c"
    "${MBEDTLS_PATH_RELATIVE}/library/error.c"
    "${MBEDTLS_PATH_RELATIVE}/library/gcm.c"
    "${MBEDTLS_PATH_RELATIVE}/library/md_wrap.c"
    "${MBEDTLS_PATH_RELATIVE}/library/md.c"
    "${MBEDTLS_PATH_RELATIVE}/library/md5.c"
    "${MBEDTLS_PATH_RELATIVE}/library/oid.c"
    "${MBEDTLS_PATH_RELATIVE}/library/pem.c"
    "${MBEDTLS_PATH_RELATIVE}/library/pk_wrap.c"
    "${MBEDTLS_PATH_RELATIVE}/library/pk.c"
    "${MBEDTLS_PATH_RELATIVE}/library/pkparse.c"
    "${MBEDTLS_PATH_RELATIVE}/library/platform_util.c"
    "${MBEDTLS_PATH_RELATIVE}/library/platform.c"
    "${MBEDTLS_PATH_RELATIVE}/library/rsa_internal.c"
    "${MBEDTLS_PATH_RELATIVE}/library/rsa.c"
    "${MBEDTLS_PATH_RELATIVE}/library/sha1.c"
    "${MBEDTLS_PATH_RELATIVE}/library/sha256.c"
    "${MBEDTLS_PATH_RELATIVE}/library/ssl_ciphersuites.c"
    #"${MBEDTLS_PATH_RELATIVE}/library/ssl_cli.c"
    #"${MBEDTLS_PATH_RELATIVE}/library/ssl_cookie.c"
    "${MBEDTLS_PATH_RELATIVE}/library/ssl_srv.c"
    #"${MBEDTLS_PATH_RELATIVE}/library/ssl_ticket.c"
    "${MBEDTLS_PATH_RELATIVE}/library/ssl_tls.c"
    "${MBEDTLS_PATH_RELATIVE}/library/x509_crt.c"
    "${MBEDTLS_PATH_RELATIVE}/library/x509.c"
)

add_library(${PROJECT_NAME} INTERFACE)

target_sources(${PROJECT_NAME}
    INTERFACE
        ${SOURCES_TLS_SERVER}
)

target_include_directories(${PROJECT_NAME}
    INTERFACE
        "${MBEDTLS_PATH_RELATIVE}/include"
)

target_compile_options(${PROJECT_NAME}
    INTERFACE
       ${COMPILE_OPTIONS}
       # Explicitly not enable OS_Crypto to use raw mbedTLS for now.
       #-DUSE_OS_CRYPTO
)

target_compile_definitions(${PROJECT_NAME}
    INTERFACE
        MBEDTLS_CONFIG_FILE="config/config-tls-server.h"
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
        os_network_api
        mbedtls_for_tls_server
)

DeclareCAmkESComponent(
    NwStack
    SOURCES
        components/NwStack/src/NwStack.c
    C_FLAGS
        -Wall
        -Werror
        -DNUM_SOCKETS=NETWORK_STACK_NUM_SOCKETS
        -DCFG_ETH_ADDR=ETH_ADDR
        -DCFG_ETH_GATEWAY_ADDR=ETH_GATEWAY_ADDR
        -DCFG_ETH_SUBNET_MASK=ETH_SUBNET_MASK
    LIBS
        system_config
        os_core_api
        lib_debug
        os_network_lib
        TimeServer_client
)

DeclareCAmkESComponent(
    Ticker
    SOURCES
        components/Ticker/src/Ticker.c
    C_FLAGS
        -Wall
        -Werror
    LIBS
        os_core_api
        lib_debug
)

TimeServer_DeclareCAmkESComponent(
    TimeServer
)

EntropySource_DeclareCAmkESComponent(
    EntropySource
)

DeclareAndCreateCamkESSystem(DemoTlsServer.camkes)
GenerateSimulateScript()
