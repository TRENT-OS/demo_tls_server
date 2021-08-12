/*
 * Filter Listener
 *
 * Copyright (C) 2021, HENSOLDT Cyber GmbH
 */

#include "system_config.h"

#include "lib_debug/Debug.h"
#include <camkes.h>
#include <string.h>

#include "OS_Error.h"
#include "OS_Network.h"
#include "OS_NetworkStackClient.h"

static void
initNetworkClient(void)
{
    static OS_Dataport_t dataports[FILTER_LISTENER_NUM_SOCKETS] =
    {
        OS_DATAPORT_ASSIGN(socket_1_port),
        OS_DATAPORT_ASSIGN(socket_2_port)
    };

    static OS_NetworkStackClient_SocketDataports_t config =
    {
        .number_of_sockets = ARRAY_SIZE(dataports),
        .dataport = dataports
    };

    OS_NetworkStackClient_init(&config);
}

static void
processRxData(
    void* const rxData,
    const size_t len)
{
    Debug_LOG_INFO("TODO: process received data");
}

//------------------------------------------------------------------------------
int
run(void)
{
    Debug_LOG_INFO("Starting Filter Listener");

    initNetworkClient();

    OS_NetworkServer_Socket_t tcp_socket =
    {
        .domain = OS_AF_INET,
        .type   = OS_SOCK_STREAM,
        .listen_port = FILTER_LISTENER_PORT,
        .backlog   = 1
    };

    OS_NetworkServer_Handle_t hServer;
    OS_Error_t ret = OS_NetworkServerSocket_create(
                        NULL,
                        &tcp_socket,
                        &hServer);

    if (ret != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_NetworkServerSocket_create() failed, code %d", ret);
        return -1;
    }

    static uint8_t rxData[OS_DATAPORT_DEFAULT_SIZE] = {0};

    for (;;)
    {
        Debug_LOG_INFO("Accepting new connection");
        OS_NetworkSocket_Handle_t hSocket;
        ret = OS_NetworkServerSocket_accept(
                  hServer,
                  &hSocket);

        if (ret != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_NetworkServerSocket_accept() failed, error %d",
                            ret);
            return -1;
        }

        // Loop until an error occurs.
        while (ret == OS_SUCCESS)
        {
            Debug_LOG_INFO("Waiting for a new message");

            size_t actualLenRx = 0;

            ret = OS_NetworkSocket_read(
                      hSocket,
                      rxData,
                      sizeof(rxData),
                      &actualLenRx);

            switch (ret)
            {
            case OS_SUCCESS:
                Debug_LOG_INFO(
                    "OS_NetworkSocket_read() received %zu bytes of data",
                    actualLenRx);

                processRxData(rxData, actualLenRx);

                continue;

            case OS_ERROR_CONNECTION_CLOSED:
                Debug_LOG_INFO(
                    "OS_NetworkSocket_read() reported connection closed");
                break;

            case OS_ERROR_NETWORK_CONN_SHUTDOWN:
                Debug_LOG_DEBUG(
                    "OS_NetworkSocket_read() reported connection closed");
                break;

            default:
                Debug_LOG_ERROR(
                    "OS_NetworkSocket_read() failed, error %d", ret);
                break;
            }
        }

        OS_NetworkSocket_close(hSocket);
    }

    return 0;
}
