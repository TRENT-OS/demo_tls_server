# Demo TLS Server

Temporary demo system for the TLS server proof-of-concept (PoC).

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

### Nmap

WARNING:
- Client authentication has to be disabled in `TlsServer.c`. This can be done by
  disabling function `mbedtls_ssl_conf_authmode()`.
- Enumeration of allowed cipher suites is not very reliable with nmap because it
  often causes problems in the TLS server at the moment (asserts or crash in the
  network stack).

```bash
nmap --script ssl-enum-ciphers -p 5560 172.17.0.1
```
