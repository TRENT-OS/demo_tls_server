# Demo TLS Server

Temporary demo system for the TLS server proof-of-concept (PoC).


- [Demo TLS Server](#demo-tls-server)
  - [Build](#build)
    - [Demo TLS Server](#demo-tls-server-1)
    - [Proxy](#proxy)
  - [Run](#run)
  - [Test Applications](#test-applications)
    - [OpenSSL](#openssl)
      - [Create Certificates](#create-certificates)
      - [Connect](#connect)
      - [Enumerate Cipher Suites](#enumerate-cipher-suites)
    - [Chrome](#chrome)
      - [Prerequisites](#prerequisites)
      - [Connect](#connect-1)
    - [Curl](#curl)
    - [Demo TLS API](#demo-tls-api)
    - [Nmap](#nmap)

## Build

### Demo TLS Server

```bash
seos_sandbox/scripts/open_trentos_build_env.sh seos_sandbox/build-system.sh src/demos/demo_tls_server zynq7000 build-zynq7000-Debug-demo_tls_server -DCMAKE_BUILD_TYPE=Debug
```

### Proxy

```bash
seos_sandbox/scripts/open_trentos_build_env.sh seos_sandbox/tools/proxy/build.sh seos_sandbox
```

## Run

```bash
seos_sandbox/scripts/open_trentos_test_env.sh -d "-p 5560:5560" -d "-v $(pwd)/src/demos/demo_tls_server/docker:/docker" -d "--entrypoint=/docker/entrypoint.sh" src/demos/demo_tls_server/run_demo.sh build-zynq7000-Debug-demo_tls_server build_proxy
```

## Test Applications

See `src/demos/demo_tls_server/test_applications`.

### OpenSSL

#### Create Certificates

- Run script `create_certs.sh` to create new server / client certs.
- Use option `-r` to create new root cert and cert chains.

#### Connect

- Run script `connect_to_tls_server.sh` to start the OpenSSL client.
- Trigger HTTP response by pressing enter (i.e. send anything).
- TLS server page shows used cipher suite.

#### Enumerate Cipher Suites

- Run script `enum_cipher_suites.sh`.

### Chrome

#### Prerequisites

- Open `chrome://settings/certificates`.
- Import `certs/CA.crt` under "Authorities".
- Import `certs/client.p12` under "Your certificates".

#### Connect

- Open `https://172.17.0.1:5560`.
- Select client certificate (empty password).
- TLS server page shows used cipher suite.

### Curl

NOTE:
- Since the current TLS Server does not parse the concrete HTTP request, the
  actual request is irrelevant and the TLS Server will reply with the default
  page.

```bash
curl -k --cacert certs/CA.crt --cert certs/client.crt --key certs/client.key https://172.17.0.1:5560/test.txt
```

### Demo TLS API

The Demo TLS API and the TLS client library `os_tls` have been prepared on
branch **SEOS-3055-tls-client-authentication**:
- IP address / port and server cert (see `demo_tls_api::DemoConfig.h`).
- Prototypical preparation of client authentication with client cert + key (see
  `os_tls::TlsLib.c`).

NOTE:
- Since the current TLS Server does not parse the concrete HTTP request, the
  actual request is irrelevant and the TLS Server will reply with the default
  page.

```bash
# Build demo
seos_sandbox/scripts/open_trentos_build_env.sh seos_sandbox/build-system.sh src/demos/demo_tls_api zynq7000 build-zynq7000-Debug-demo_tls_api -DCMAKE_BUILD_TYPE=Debug

# Build proxy
seos_sandbox/scripts/open_trentos_build_env.sh seos_sandbox/tools/proxy/build.sh seos_sandbox

# Run demo
seos_sandbox/scripts/open_trentos_test_env.sh src/demos/demo_tls_api/run_demo.sh build-zynq7000-Debug-demo_tls_api build_proxy
```

### Nmap

WARNING:
- Client authentication is not supported and has to be disabled in `TlsServer.c`
  (see `mbedtls_ssl_conf_authmode()`).
- Enumeration of allowed cipher suites is not very reliable with nmap because it
  often causes problems in the TLS server at the moment (assert or crash in the
  network stack).

```bash
nmap --script ssl-enum-ciphers -p 5560 172.17.0.1
```
