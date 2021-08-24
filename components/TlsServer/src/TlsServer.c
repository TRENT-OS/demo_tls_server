/*
 * TLS Server
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

//! Maximum buffer size for send / receive functions.
#define MAX_NW_SIZE OS_DATAPORT_DEFAULT_SIZE


//------------------------------------------------------------------------------

static void
initNetworkClient(void)
{
    static OS_Dataport_t dataports[TLS_SERVER_NUM_SOCKETS] =
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

static int
sendFunc(
    void* ctx,
    const unsigned char* buf,
    size_t len)
{
    OS_NetworkSocket_Handle_t* socket = (OS_NetworkSocket_Handle_t*)ctx;
    size_t n = len > MAX_NW_SIZE ? MAX_NW_SIZE : len;

    OS_Error_t err = OS_NetworkSocket_write(*socket, buf, n, &n);

    if (OS_SUCCESS != err)
    {
        Debug_LOG_ERROR("OS_NetworkSocket_write() failed, error %d", err);
        n = -1;
    }

    return n;
}

static int
recvFunc(
    void* ctx,
    unsigned char* buf,
    size_t len)
{
    OS_NetworkSocket_Handle_t* socket = (OS_NetworkSocket_Handle_t*)ctx;
    size_t n = len > MAX_NW_SIZE ? MAX_NW_SIZE : len;

    OS_Error_t err = OS_NetworkSocket_read(*socket, buf, n, &n);

    switch (err)
    {
    case OS_SUCCESS:
        Debug_LOG_INFO(
            "OS_NetworkSocket_read() received %zu bytes of data", n);
        // n = n;
        break;

    case OS_ERROR_CONNECTION_CLOSED:
        Debug_LOG_INFO(
            "OS_NetworkSocket_read() reported connection closed");
        n = 0;
        break;

    case OS_ERROR_NETWORK_CONN_SHUTDOWN:
        Debug_LOG_INFO(
            "OS_NetworkSocket_read() reported connection shutdown");
        n = 0;
        break;

    default:
        Debug_LOG_ERROR(
            "OS_NetworkSocket_read() failed, error %d", err);
        n = -1;
        break;
    }

    return n;
}

static void
echoRxData(
    OS_NetworkSocket_Handle_t hSocket,
    const uint8_t* const bufRxData,
    const size_t bufLen)
{
    size_t sumLenTx = 0;

    while (sumLenTx < bufLen)
    {
        int actualLenTx = sendFunc(
            &hSocket,
            &bufRxData[sumLenTx],
            bufLen - sumLenTx);

        if (actualLenTx < 0)
        {
            Debug_LOG_ERROR("sendFunc() failed, error %d", actualLenTx);
            break;
        }

        sumLenTx += (size_t)actualLenTx;

        Debug_LOG_INFO("sendFunc() ok, send_desired: %zu, "
                        "send_current: %d, send_total: %zu",
                        bufLen, actualLenTx, sumLenTx);
    }
}


//------------------------------------------------------------------------------

int
run(void)
{
    Debug_LOG_INFO("Starting TLS Server");

    initNetworkClient();

    OS_NetworkServer_Socket_t tcp_socket =
    {
        .domain = OS_AF_INET,
        .type   = OS_SOCK_STREAM,
        .listen_port = TLS_SERVER_PORT,
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

    static uint8_t rxData[MAX_NW_SIZE] = {0};

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
        while (1)
        {
            Debug_LOG_INFO("Waiting for a new message");

            int actualLenRx = recvFunc(&hSocket, rxData, sizeof(rxData));

            if (actualLenRx > 0)
            {
                echoRxData(hSocket, rxData, (size_t)actualLenRx);
            }
            else if (actualLenRx == 0)
            {
                Debug_LOG_INFO("Connection closed/shutdown.");
                break;
            }
            else
            {
                Debug_LOG_ERROR("Connection failed, error %d", actualLenRx);
                break;
            }
        }

        OS_NetworkSocket_close(hSocket);
    }

    return 0;
}
