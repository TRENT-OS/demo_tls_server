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

#include "OS_Error.h"
#include "OS_Socket.h"

#include "mbedtls/certs.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ssl.h"
#include "mbedtls/debug.h"

//------------------------------------------------------------------------------

//! Maximum buffer size for send / receive functions.
#define MAX_NW_SIZE OS_DATAPORT_DEFAULT_SIZE

#define DEBUG_LEVEL 0

#define HTTP_RESPONSE \
    "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n" \
    "<h2>mbed TLS Test Server</h2>\r\n" \
    "<p>Successful connection using: %s</p>\r\n"

//------------------------------------------------------------------------------

static const char cTlsServerRootCert[] = TLS_SERVER_ROOT_CERT;
static const size_t cTlsServerRootCertLen = sizeof( cTlsServerRootCert );

static const char cTlsServerCert[] = TLS_SERVER_CERT;
static const size_t cTlsServerCertLen = sizeof( cTlsServerCert );

static const char cTlsServerKey[] = TLS_SERVER_KEY;
static const size_t cTlsServerKeyLen = sizeof( cTlsServerKey );

static const int cCipherSuites[] =
{
    MBEDTLS_TLS_DHE_RSA_WITH_AES_128_GCM_SHA256,
    MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
    MBEDTLS_TLS_RSA_WITH_AES_128_CBC_SHA,

    0 // list needs to be 0 terminated
};

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
            Debug_LOG_ERROR("A FATAL_ERROR occurred in the Network Stack component.");
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
            Debug_LOG_TRACE("OS_Socket_getPendingEvents() returned "
                            "without any pending events");
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

static int
sendFunc(
    void* ctx,
    const unsigned char* buf,
    size_t len)
{
    OS_Socket_Handle_t* socket = (OS_Socket_Handle_t*)ctx;
    size_t n = len > MAX_NW_SIZE ? MAX_NW_SIZE : len;

    OS_Error_t err = OS_Socket_write(*socket, buf, n, &n);

    switch (err)
    {
    case OS_SUCCESS:
        Debug_LOG_INFO(
            "OS_Socket_write() sent %zu bytes of data", n);
        break;

    case OS_ERROR_TRY_AGAIN:
        Debug_LOG_TRACE(
            "OS_Socket_write() reported try again");
        n = MBEDTLS_ERR_SSL_WANT_WRITE;
        break;

    default:
        Debug_LOG_ERROR("OS_Socket_write() failed, error %d", err);
        n = -1;
        break;
    }

    return n;
}

static int
recvFunc(
    void* ctx,
    unsigned char* buf,
    size_t len)
{
    OS_Socket_Handle_t* socket = (OS_Socket_Handle_t*)ctx;
    size_t n = len > MAX_NW_SIZE ? MAX_NW_SIZE : len;

    OS_Error_t err = OS_Socket_read(*socket, buf, n, &n);

    switch (err)
    {
    case OS_SUCCESS:
        Debug_LOG_INFO(
            "OS_Socket_read() received %zu bytes of data", n);
        break;

    case OS_ERROR_TRY_AGAIN:
        Debug_LOG_TRACE(
            "OS_Socket_read() reported try again");
        n = MBEDTLS_ERR_SSL_WANT_READ;
        break;

    case OS_ERROR_CONNECTION_CLOSED:
        Debug_LOG_INFO(
            "OS_Socket_read() reported connection closed");
        n = -1;
        break;

    case OS_ERROR_NETWORK_CONN_SHUTDOWN:
        Debug_LOG_INFO(
            "OS_Socket_read() reported connection shutdown");
        n = -1;
        break;

    default:
        Debug_LOG_ERROR(
            "OS_Socket_read() failed, error %d", err);
        n = -1;
        break;
    }

    return n;
}

// This function is called by mbedTLS to log messages.
static void
logDebug(
    void*       ctx,
    int         level,
    const char* file,
    int         line,
    const char* str)
{
    char msg[256];
    UNUSED_VAR(ctx);
    UNUSED_VAR(level);

    if (level < 1 || level > 4)
    {
        return;
    }

    size_t len_file_name = strlen(file);
    const size_t max_len_file = 16;
    bool is_short_file_name = (len_file_name <= max_len_file);

    snprintf(
        msg,
        sizeof(msg),
        "[%s%s:%05i]: %s",
        is_short_file_name ? "" : "...",
        &file[ is_short_file_name ?  0 : len_file_name - max_len_file],
        line,
        str);

    // Replace '\n' in mbedtls string with '\0' because otherwise it would
    // result in extra empty lines because Debug_LOG_XXX adds another '\n'.
    size_t msg_len = strlen(msg);
    if (msg_len > 0 && msg[msg_len - 1] == '\n')
    {
        msg[msg_len - 1] = '\0';
    }

    // Translate mbedTLS level to appropriate debug output
    switch (level)
    {
    case 1: // Error
        Debug_LOG_ERROR("%s", msg);
        break;
    case 2: // State change
    case 3: // Informational
        Debug_LOG_INFO("%s", msg);
        break;
    case 4: // Verbose
        Debug_LOG_DEBUG("%s", msg);
        break;
    default:
        // Do nothing
        break;
    }
}

static int
entropyWrapper(
    void*          ctx,
    unsigned char* buf,
    size_t         len)
{
    if_OS_Entropy_t* entropy = (if_OS_Entropy_t*)ctx;

    size_t s = entropy->read(len);
    memcpy(buf, OS_Dataport_getBuf(entropy->dataport), s);

    // If we cannot return as many bytes as requested, produce an error
    return (s == len) ? 0 : -1;
}

//------------------------------------------------------------------------------

int
run(void)
{
    Debug_LOG_INFO("Starting TLS Server");

    // Check and wait until the NetworkStack component is up and running.
    OS_Error_t err = waitForNetworkStackInit(&networkStackCtx);
    if (OS_SUCCESS != err)
    {
        Debug_LOG_ERROR("waitForNetworkStackInit() failed with: %d", err);
        return -1;
    }

    OS_Socket_Handle_t hServer;
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
    // Init mbedtls

    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ctr_drbg_init( &ctr_drbg );

    mbedtls_ssl_context ssl;
    mbedtls_ssl_init( &ssl );

    mbedtls_ssl_config conf;
    mbedtls_ssl_config_init( &conf );

    mbedtls_x509_crt srvcert;
    mbedtls_x509_crt_init( &srvcert );

    mbedtls_pk_context pkey;
    mbedtls_pk_init( &pkey );

    mbedtls_debug_set_threshold( DEBUG_LEVEL );

    // -------------------------------------------------------------------------
    // Init entropy source

    if_OS_Entropy_t entropy = IF_OS_ENTROPY_ASSIGN(
                                entropy_rpc,
                                entropy_port);

    // -------------------------------------------------------------------------
    // Load the certificates and private RSA key

    Debug_LOG_INFO( "  . Loading the server cert. and key..." );

    int ret = mbedtls_x509_crt_parse( &srvcert, (const unsigned char *) cTlsServerCert,
                          cTlsServerCertLen );
    if( ret != 0 )
    {
        Debug_LOG_ERROR( "  . failed  !  mbedtls_x509_crt_parse returned -0x%04X", -ret );
        return -1;
    }

    ret = mbedtls_x509_crt_parse( &srvcert, (const unsigned char *) cTlsServerRootCert,
                          cTlsServerRootCertLen );
    if( ret != 0 )
    {
        Debug_LOG_ERROR( "  . failed  !  mbedtls_x509_crt_parse returned -0x%04X", -ret );
        return -1;
    }

    ret = mbedtls_pk_parse_key( &pkey, (const unsigned char *) cTlsServerKey,
                         cTlsServerKeyLen, NULL, 0 );
    if( ret != 0 )
    {
        Debug_LOG_ERROR( "  . failed  !  mbedtls_pk_parse_key returned -0x%04X", -ret );
        return -1;
    }

    Debug_LOG_INFO( "  . ok" );

    // -------------------------------------------------------------------------
    // Seed the RNG

    Debug_LOG_INFO( "  . Seeding the random number generator..." );

    const char * pers = "ssl_server";

    if( ( ret = mbedtls_ctr_drbg_seed( &ctr_drbg, entropyWrapper, &entropy,
                               (const unsigned char *) pers,
                               strlen( pers ) ) ) != 0 )
    {
        Debug_LOG_ERROR( "  . failed  ! mbedtls_ctr_drbg_seed returned -0x%04X", -ret );
        return -1;
    }

    Debug_LOG_INFO( "  . ok" );

    // -------------------------------------------------------------------------
    // Setup stuff

    Debug_LOG_INFO( "  . Setting up the SSL data...." );

    if( ( ret = mbedtls_ssl_config_defaults( &conf,
                    MBEDTLS_SSL_IS_SERVER,
                    MBEDTLS_SSL_TRANSPORT_STREAM,
                    MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 )
    {
        Debug_LOG_ERROR( "  . failed  ! mbedtls_ssl_config_defaults returned -0x%04X", -ret );
        return -1;
    }

    // Enable client authentication, i.e. client needs to provide a certificate.
    mbedtls_ssl_conf_authmode( &conf, MBEDTLS_SSL_VERIFY_REQUIRED );

    // Set allowed cipher suites.
    mbedtls_ssl_conf_ciphersuites( &conf, cCipherSuites );

    mbedtls_ssl_conf_rng( &conf, mbedtls_ctr_drbg_random, &ctr_drbg );
    mbedtls_ssl_conf_dbg(&conf, logDebug, NULL);

    mbedtls_ssl_conf_ca_chain( &conf, srvcert.next, NULL );

    if( ( ret = mbedtls_ssl_conf_own_cert( &conf, &srvcert, &pkey ) ) != 0 )
    {
        Debug_LOG_ERROR( "  . failed  ! mbedtls_ssl_conf_own_cert returned -0x%04X", -ret );
        return -1;
    }

    if( ( ret = mbedtls_ssl_setup( &ssl, &conf ) ) != 0 )
    {
        Debug_LOG_ERROR( "  . failed  ! mbedtls_ssl_setup returned -0x%04X", -ret );
        return -1;
    }

    Debug_LOG_INFO( "  . ok" );

    for (;;)
    {
        mbedtls_ssl_session_reset( &ssl );

        // ---------------------------------------------------------------------
        // Wait until a client connects

        Debug_LOG_INFO( "  . Waiting for a remote connection ..." );

        OS_Socket_Handle_t hSocket;
        OS_Socket_Addr_t srcAddr = {0};

        do
        {
            err = waitForIncomingConnection(hServer.handleID);
            if (err != OS_SUCCESS)
            {
                Debug_LOG_ERROR("waitForIncomingConnection() failed, error %d", err);
                goto exit;
            }

            err = OS_Socket_accept(
                      hServer,
                      &hSocket,
                      &srcAddr);
        }
        while (err == OS_ERROR_TRY_AGAIN);

        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_Socket_accept() failed, error %d", err);
            goto exit;
        }

        mbedtls_ssl_set_bio( &ssl, &hSocket, sendFunc, recvFunc, NULL );

        Debug_LOG_INFO( "  . ok" );

        // ---------------------------------------------------------------------
        // Handshake

        Debug_LOG_INFO( "  . Performing the SSL/TLS handshake..." );

        while( ( ret = mbedtls_ssl_handshake( &ssl ) ) != 0 )
        {
            if( ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE )
            {
                seL4_Yield(); // Yield and try again next time.
            }
            else
            {
                Debug_LOG_ERROR( "  . failed  ! mbedtls_ssl_handshake returned -0x%04X", -ret );
                goto exit;
            }
        }

        Debug_LOG_INFO( "  . ok" );

        // ---------------------------------------------------------------------
        // Read the HTTP request

        int len;
        unsigned char buf[1024];

        Debug_LOG_INFO( "  .  < Read from client:" );

        for (;;)
        {
            len = sizeof( buf ) - 1;
            memset( buf, 0, sizeof( buf ) );
            ret = mbedtls_ssl_read( &ssl, buf, len );

            if( ret == MBEDTLS_ERR_SSL_WANT_READ )
            {
                seL4_Yield(); // Yield and try again next time.
                continue;
            }

            if( ret <= 0 )
            {
                switch( ret )
                {
                    case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
                        Debug_LOG_INFO( "  . connection was closed gracefully" );
                        break;

                    default:
                        Debug_LOG_ERROR( "  . mbedtls_ssl_read returned -0x%04X", -ret );
                        break;
                }

                break;
            }

            len = ret;
            Debug_LOG_INFO( "  . %d bytes read %s", len, (char *) buf );

            if( ret > 0 )
                break;
        }

        // ---------------------------------------------------------------------
        // Write the 200 response

        Debug_LOG_INFO( "  .  > Write to client:" );

        len = sprintf( (char *) buf, HTTP_RESPONSE,
                            mbedtls_ssl_get_ciphersuite( &ssl ) );

        while( ( ret = mbedtls_ssl_write( &ssl, buf, len ) ) <= 0 )
        {
            if( ret == MBEDTLS_ERR_SSL_WANT_WRITE )
            {
                seL4_Yield(); // Yield and try again next time.
            }
            else
            {
                Debug_LOG_ERROR( "  . failed  ! mbedtls_ssl_write returned -0x%04X", -ret );
                goto exit;
            }
        }

        len = ret;
        Debug_LOG_INFO( "  . %d bytes written %s", len, (char *) buf );

        // ---------------------------------------------------------------------
        // Close connection

        Debug_LOG_INFO( "  . Closing the connection..." );

        while( ( ret = mbedtls_ssl_close_notify( &ssl ) ) < 0 )
        {
            if( ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE )
            {
                seL4_Yield(); // Yield and try again next time.
            }
            else
            {
                Debug_LOG_ERROR( "  . failed  ! mbedtls_ssl_close_notify returned -0x%04X", -ret );
                goto exit;
            }
        }

exit:
        OS_Socket_close(hSocket);

        Debug_LOG_INFO( "  . ok" );

        // ---------------------------------------------------------------------
    }

    return 0;
}
