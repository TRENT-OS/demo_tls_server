# Demo TLS Server

Temporary demo system for the TLS Server proof-of-concept (PoC).


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
- TLS Server page shows "Hello TLS!" message.

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
- TLS Server page shows "Hello TLS!" message.

### Curl

NOTE:
- Since the current TLS Server does not parse the concrete HTTP request, the
  actual request is irrelevant and the TLS Server will reply with the default
  page.

```bash
curl -k --output - --cacert certs/CA.crt --cert certs/client.crt --key certs/client.key https://172.17.0.1:5560/test.txt
```

### Demo TLS API

The Demo TLS API can be connected to the TLS Server after configuring the IP
address / port and server cert properly (see `demo_tls_api::DemoConfig.h`).

WARNING:
- Client authentication is not supported by Demo TLS API and has to be disabled
  by setting flag OS_Tls_FLAG_NO_VERIFY in OS_Tls_Config_t in `TlsServer.c`.
- For checking the client authentication there is a branch of Demo TLS API
  (**SEOS-0000-use-with-demo-tls-server**) that contains a configuration of the
  "library mode" that works together with the TLS Server. Since the TLS Server
  component (implementing a TLS client) does not yet support this configuration
  option, the "client mode" is disabled in this branch.

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
- Client authentication is not supported by nmap and has to be disabled by
  setting flag OS_Tls_FLAG_NO_VERIFY in OS_Tls_Config_t in `TlsServer.c`.

NOTE:
- The option `--max-parallelism 1` is used to avoid any unwanted side-effects
  when checking the TLS functionality.

```bash
nmap --script ssl-enum-ciphers -p 5560 172.17.0.1 --max-parallelism 1
```
