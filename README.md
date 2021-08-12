# Demo TLS Server

Temporary demo system for the TLS server proof-of-concept (PoC).

## Build

seos_sandbox/scripts/open_trentos_build_env.sh ./build.sh demo_tls_server

## Run

seos_sandbox/scripts/open_trentos_test_env.sh -d "-p 5560:5560" -d "-v $(pwd)/src/demos/demo_tls_server/docker:/docker" -d "--entrypoint=/docker/entrypoint.sh" src/demos/demo_tls_server/run_demo.sh build-zynq7000-Debug-demo_tls_server OS-SDK/pkg

## Telnet

```bash
> telnet 172.17.0.1 5560
Trying 172.17.0.1...
Connected to 172.17.0.1.
Escape character is '^]'.
hello world
```

NOTE: For the escape character type 'Ctrl+AltGr+9'. Afterwards telnet can be exited with the 'close' or 'quit' command.
