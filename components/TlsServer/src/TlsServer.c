/*
 * TLS Server
 *
 * Copyright (C) 2021, HENSOLDT Cyber GmbH
 */

#include "TlsServerCerts.h"
#include "system_config.h"

#include "lib_compiler/compiler.h"
#include "lib_debug/Debug.h"
#include <sel4/sel4.h>
#include <camkes.h>
#include <string.h>

#include "interfaces/if_OS_Entropy.h"

#include "OS_Crypto.h"
#include "OS_Error.h"
#include "OS_Socket.h"
#include "OS_Tls.h"

//------------------------------------------------------------------------------

#define HTTP_RESPONSE \
    "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n" \
    "<h2>mbed TLS Test Server</h2>\r\n" \
    "<p>Successful connection: Hello TLS!</p>\r\n"

//------------------------------------------------------------------------------

static const if_OS_Socket_t networkStackCtx =
    IF_OS_SOCKET_ASSIGN(networkStack);

//------------------------------------------------------------------------------

static OS_Error_t
waitForNetworkStackInit(
    const if_OS_Socket_t* const ctx)
{
    OS_NetworkStack_State_t networkStackState;

    for (;;)
    {
        networkStackState = OS_Socket_getStatus(ctx);
        if (networkStackState == RUNNING)
        {
            // NetworkStack up and running.
            return OS_SUCCESS;
        }
        else if (networkStackState == FATAL_ERROR)
        {
            // NetworkStack will not come up.
            Debug_LOG_ERROR("A FATAL_ERROR occurred in the Network Stack "
                            "component.");
            return OS_ERROR_ABORTED;
        }

        seL4_Yield(); // Yield and try again next time.
    }
}

static OS_Error_t
waitForIncomingConnection(
    const int srvHandleId)
{
    OS_Error_t ret;

    // Wait for the event letting us know that the connection was successfully
    // established.
    for (;;)
    {
        ret = OS_Socket_wait(&networkStackCtx);
        if (ret != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_Socket_wait() failed, code %d", ret);
            break;
        }

        char evtBuffer[128];
        const size_t evtBufferSize = sizeof(evtBuffer);
        int numberOfSocketsWithEvents;

        ret = OS_Socket_getPendingEvents(
                  &networkStackCtx,
                  evtBuffer,
                  evtBufferSize,
                  &numberOfSocketsWithEvents);
        if (ret != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_Socket_getPendingEvents() failed, code %d",
                            ret);
            break;
        }

        if (numberOfSocketsWithEvents == 0)
        {
            Debug_LOG_TRACE("OS_Socket_getPendingEvents() returned without any "
                            "pending events");
            continue;
        }

        // We only opened one socket, so if we get more events, this is not ok.
        if (numberOfSocketsWithEvents != 1)
        {
            Debug_LOG_ERROR("OS_Socket_getPendingEvents() returned with "
                            "unexpected #events: %d", numberOfSocketsWithEvents);
            ret = OS_ERROR_INVALID_STATE;
            break;
        }

        OS_Socket_Evt_t event;
        memcpy(&event, evtBuffer, sizeof(event));

        if (event.socketHandle != srvHandleId)
        {
            Debug_LOG_ERROR("Unexpected handle received: %d, expected: %d",
                            event.socketHandle, srvHandleId);
            ret = OS_ERROR_INVALID_HANDLE;
            break;
        }

        // Socket has been closed by NetworkStack component.
        if (event.eventMask & OS_SOCK_EV_FIN)
        {
            Debug_LOG_ERROR("OS_Socket_getPendingEvents() returned "
                            "OS_SOCK_EV_FIN for handle: %d",
                            event.socketHandle);
            ret = OS_ERROR_NETWORK_CONN_REFUSED;
            break;
        }

        // Incoming connection received.
        if (event.eventMask & OS_SOCK_EV_CONN_ACPT)
        {
            Debug_LOG_DEBUG("OS_Socket_getPendingEvents() returned "
                            "connection established for handle: %d",
                            event.socketHandle);
            ret = OS_SUCCESS;
            break;
        }

        // Remote socket requested to be closed only valid for clients.
        if (event.eventMask & OS_SOCK_EV_CLOSE)
        {
            Debug_LOG_ERROR("OS_Socket_getPendingEvents() returned "
                            "OS_SOCK_EV_CLOSE for handle: %d",
                            event.socketHandle);
            ret = OS_ERROR_CONNECTION_CLOSED;
            break;
        }

        // Error received - print error.
        if (event.eventMask & OS_SOCK_EV_ERROR)
        {
            Debug_LOG_ERROR("OS_Socket_getPendingEvents() returned "
                            "OS_SOCK_EV_ERROR for handle: %d, code: %d",
                            event.socketHandle, event.currentError);
            ret = event.currentError;
            break;
        }
    }

    return ret;
}

//------------------------------------------------------------------------------

int
run(void)
{
    Debug_LOG_INFO("Starting TLS Server...");

    // Check and wait until the NetworkStack component is up and running.
    OS_Error_t err = waitForNetworkStackInit(&networkStackCtx);
    if (OS_SUCCESS != err)
    {
        Debug_LOG_ERROR("waitForNetworkStackInit() failed, code %d", err);
        return -1;
    }

    OS_Socket_Handle_t hServer;
    OS_Socket_Handle_t hSocket;

    err = OS_Socket_create(
              &networkStackCtx,
              &hServer,
              OS_AF_INET,
              OS_SOCK_STREAM);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_Socket_create() failed, code %d", err);
        return -1;
    }

    const OS_Socket_Addr_t dstAddr =
    {
        .addr = OS_INADDR_ANY_STR,
        .port = TLS_SERVER_PORT
    };

    err = OS_Socket_bind(
              hServer,
              &dstAddr);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_Socket_bind() failed, code %d", err);
        OS_Socket_close(hServer);
        return -1;
    }

    err = OS_Socket_listen(
              hServer,
              1);
    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_Socket_listen() failed, code %d", err);
        OS_Socket_close(hServer);
        return -1;
    }

    // -------------------------------------------------------------------------

    const OS_Crypto_Config_t cryptoCfg =
    {
        .mode = OS_Crypto_MODE_LIBRARY,
        .entropy = IF_OS_ENTROPY_ASSIGN(
            entropy_rpc,
            entropy_port),
    };

    OS_Tls_Config_t tlsConfig =
    {
        .mode = OS_Tls_MODE_LIBRARY,
        .library = {
            .socket = {
                .context    = &hSocket,
            },
            .flags = OS_Tls_FLAG_NONE,
            .crypto = {
                .policy     = NULL,
                .caCerts    = TLS_SERVER_ROOT_CERT,
                .ownCert    = TLS_SERVER_CERT,
                .privateKey = TLS_SERVER_KEY,
                .cipherSuites =
                OS_Tls_CIPHERSUITE_FLAGS(
                    OS_Tls_CIPHERSUITE_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
                    OS_Tls_CIPHERSUITE_DHE_RSA_WITH_AES_128_GCM_SHA256)
            }
        }
    };

    err = OS_Crypto_init(&tlsConfig.library.crypto.handle, &cryptoCfg);
    if (OS_SUCCESS != err)
    {
        Debug_LOG_ERROR("OS_Crypto_init() failed, code %d", err);
        return -1;
    }
    Debug_LOG_INFO("Crypto library successfully initialized");

    OS_Tls_Handle_t hTls;

    err = OS_Tls_init(&hTls, &tlsConfig);
    if (OS_SUCCESS != err)
    {
        Debug_LOG_ERROR("OS_Tls_init() failed, code %d", err);

        // Free previously allocated memory before return.
        OS_Crypto_free(tlsConfig.library.crypto.handle);

        return -1;
    }
    Debug_LOG_INFO("TLS library successfully initialized");

    // -------------------------------------------------------------------------

    for (;;)
    {
        // ---------------------------------------------------------------------
        // Wait until a client connects

        Debug_LOG_INFO( "Waiting for a remote connection..." );

        OS_Socket_Addr_t srcAddr = {0};

        do
        {
            err = waitForIncomingConnection(hServer.handleID);
            if (err != OS_SUCCESS)
            {
                Debug_LOG_ERROR("waitForIncomingConnection() failed, code % d",
                                err);
                goto close_connection;
            }

            err = OS_Socket_accept(
                      hServer,
                      &hSocket,
                      &srcAddr);
        }
        while (err == OS_ERROR_TRY_AGAIN);

        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_Socket_accept() failed, code %d", err);
            goto close_connection;
        }

        // ---------------------------------------------------------------------
        // Handshake

        do
        {
            seL4_Yield();
            err = OS_Tls_handshake(hTls);
        }
        while (err == OS_ERROR_WOULD_BLOCK);

        if (OS_SUCCESS != err)
        {
            Debug_LOG_ERROR("OS_Tls_handshake() failed, code %d", err);
            goto close_connection;
        }
        Debug_LOG_INFO("TLS connection established");

        // ---------------------------------------------------------------------
        // Receive request

        static char rxBuf[1024] = {0};
        const size_t cRxSize = sizeof(rxBuf);
        const size_t cRequiredRxSize = 1; // stop reading after at least 1 byte

        bool stopReading = false;
        size_t rxRemainingSize = cRxSize;

        while (!stopReading && (rxRemainingSize > 0))
        {
            size_t dataSize = rxRemainingSize;

            err = OS_Tls_read(hTls,
                              (rxBuf + (cRxSize - rxRemainingSize)),
                              &dataSize);

            switch (err)
            {
            case OS_SUCCESS:
                rxRemainingSize -= dataSize;

                if ((cRxSize - rxRemainingSize) >= cRequiredRxSize)
                {
                    // Stop reading if the required size has been received.
                    stopReading = true;
                }
                break;
            case OS_ERROR_WOULD_BLOCK:
                // Donate the remaining timeslice to a thread of the same
                // priority and try to read again with the next turn.
                seL4_Yield();
                break;
            case OS_ERROR_CONNECTION_CLOSED:
                Debug_LOG_WARNING("OS_Tls_read() connection closed by network "
                                  "stack");
                stopReading = true;
                break;
            case OS_ERROR_NETWORK_CONN_SHUTDOWN:
                Debug_LOG_WARNING("OS_Tls_read() connection reset by peer");
                stopReading = true;
                break;
            default:
                Debug_LOG_ERROR("OS_Tls_read() failed, code %d, bytes read %zu",
                                err, (cRxSize - rxRemainingSize));
                goto close_connection;
            }
        }

        rxBuf[cRxSize - 1] = '\0'; // ensure buffer is nul-terminated
        Debug_LOG_INFO("Received %zu bytes:\n%s",
                       (cRxSize - rxRemainingSize),
                       rxBuf);

        // ---------------------------------------------------------------------
        // Send response

        unsigned char txBuf[] = HTTP_RESPONSE;
        const size_t cTxSize = sizeof(txBuf);

        size_t txRemainingSize = cTxSize;

        while (txRemainingSize > 0)
        {
            size_t dataSize = txRemainingSize;
            err = OS_Tls_write(hTls,
                               (txBuf + (cTxSize - txRemainingSize)),
                               &dataSize);

            switch (err)
            {
            case OS_SUCCESS:
                txRemainingSize -= dataSize;
                break;
            case OS_ERROR_WOULD_BLOCK:
                // Donate the remaining timeslice to a thread of the same
                // priority and try to write again with the next turn.
                seL4_Yield();
                break;
            default:
                Debug_LOG_ERROR("OS_Tls_write() failed, code %d", err);
                goto close_connection;
            }
        }

        txBuf[cTxSize - 1] = '\0'; // ensure buffer is nul-terminated
        Debug_LOG_INFO("Sent %zu bytes:\n%s",
                       (cTxSize - txRemainingSize),
                       txBuf);

close_connection:
        // ---------------------------------------------------------------------
        // Close connection

        err = OS_Tls_reset(hTls);
        if (OS_SUCCESS != err)
        {
            Debug_LOG_ERROR("OS_Tls_reset() failed, code %d", err);
            break;
        }

        err = OS_Socket_close(hSocket);
        if (OS_SUCCESS != err)
        {
            Debug_LOG_ERROR("OS_Socket_close() failed, code %d", err);
            break;
        }

        Debug_LOG_INFO("TLS connection closed");

        // ---------------------------------------------------------------------
    }

    // -------------------------------------------------------------------------
    // Free memory

    OS_Crypto_free(tlsConfig.library.crypto.handle);
    OS_Tls_free(hTls);

    return -1;
}
