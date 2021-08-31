/*
 * TLS Server
 *
 * Copyright (C) 2021, HENSOLDT Cyber GmbH
 */

#include "system_config.h"

#include "lib_compiler/compiler.h"
#include "lib_debug/Debug.h"
#include <camkes.h>
#include <string.h>

#include "interfaces/if_OS_Entropy.h"

#include "OS_Error.h"
#include "OS_Network.h"
#include "OS_NetworkStackClient.h"

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

    initNetworkClient();

    OS_NetworkServer_Socket_t tcpSocket =
    {
        .domain = OS_AF_INET,
        .type   = OS_SOCK_STREAM,
        .listen_port = TLS_SERVER_PORT,
        .backlog   = 1
    };

    OS_NetworkServer_Handle_t hServer;
    OS_Error_t err = OS_NetworkServerSocket_create(
                        NULL,
                        &tcpSocket,
                        &hServer);

    if (err != OS_SUCCESS)
    {
        Debug_LOG_ERROR("OS_NetworkServerSocket_create() failed, code %d", err);
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

    int ret = mbedtls_x509_crt_parse( &srvcert, (const unsigned char *) mbedtls_test_srv_crt,
                          mbedtls_test_srv_crt_len );
    if( ret != 0 )
    {
        Debug_LOG_ERROR( "  . failed  !  mbedtls_x509_crt_parse returned %d", ret );
        return -1;
    }

    ret = mbedtls_x509_crt_parse( &srvcert, (const unsigned char *) mbedtls_test_cas_pem,
                          mbedtls_test_cas_pem_len );
    if( ret != 0 )
    {
        Debug_LOG_ERROR( "  . failed  !  mbedtls_x509_crt_parse returned %d", ret );
        return -1;
    }

    ret = mbedtls_pk_parse_key( &pkey, (const unsigned char *) mbedtls_test_srv_key,
                         mbedtls_test_srv_key_len, NULL, 0 );
    if( ret != 0 )
    {
        Debug_LOG_ERROR( "  . failed  !  mbedtls_pk_parse_key returned %d", ret );
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
        Debug_LOG_ERROR( "  . failed  ! mbedtls_ctr_drbg_seed returned %d", ret );
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
        Debug_LOG_ERROR( "  . failed  ! mbedtls_ssl_config_defaults returned %d", ret );
        return -1;
    }

    mbedtls_ssl_conf_rng( &conf, mbedtls_ctr_drbg_random, &ctr_drbg );
    mbedtls_ssl_conf_dbg(&conf, logDebug, NULL);

    mbedtls_ssl_conf_ca_chain( &conf, srvcert.next, NULL );

    if( ( ret = mbedtls_ssl_conf_own_cert( &conf, &srvcert, &pkey ) ) != 0 )
    {
        Debug_LOG_ERROR( "  . failed  ! mbedtls_ssl_conf_own_cert returned %d", ret );
        return -1;
    }

    if( ( ret = mbedtls_ssl_setup( &ssl, &conf ) ) != 0 )
    {
        Debug_LOG_ERROR( "  . failed  ! mbedtls_ssl_setup returned %d", ret );
        return -1;
    }

    Debug_LOG_INFO( "  . ok" );

    for (;;)
    {
        mbedtls_ssl_session_reset( &ssl );

        // ---------------------------------------------------------------------
        // Wait until a client connects

        Debug_LOG_INFO( "  . Waiting for a remote connection ..." );

        OS_NetworkSocket_Handle_t hSocket;
        err = OS_NetworkServerSocket_accept(
                  hServer,
                  &hSocket);

        if (err != OS_SUCCESS)
        {
            Debug_LOG_ERROR("OS_NetworkServerSocket_accept() failed, error %d", err);
            goto exit;
        }

        mbedtls_ssl_set_bio( &ssl, &hSocket, sendFunc, recvFunc, NULL );

        Debug_LOG_INFO( "  . ok" );

        // ---------------------------------------------------------------------
        // Handshake

        Debug_LOG_INFO( "  . Performing the SSL/TLS handshake..." );

        while( ( ret = mbedtls_ssl_handshake( &ssl ) ) != 0 )
        {
            if( ret != MBEDTLS_ERR_SSL_WANT_READ
                && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
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
                continue;

            if( ret <= 0 )
            {
                switch( ret )
                {
                    case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
                        Debug_LOG_INFO( "  . connection was closed gracefully" );
                        break;

                    default:
                        Debug_LOG_ERROR( "  . mbedtls_ssl_read returned -0x%x", -ret );
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
            if( ret != MBEDTLS_ERR_SSL_WANT_WRITE )
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
            if( ret != MBEDTLS_ERR_SSL_WANT_READ
                && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
            {
                Debug_LOG_ERROR( "  . failed  ! mbedtls_ssl_close_notify returned -0x%04X", -ret );
                goto exit;
            }
        }

exit:
        OS_NetworkSocket_close(hSocket);

        Debug_LOG_INFO( "  . ok" );

        // ---------------------------------------------------------------------
    }

    return 0;
}
