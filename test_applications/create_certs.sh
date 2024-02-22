#!/bin/bash -eu

#-------------------------------------------------------------------------------
#
# Copyright (C) 2021-2024, HENSOLDT Cyber GmbH
# 
# SPDX-License-Identifier: GPL-2.0-or-later
#
# For commercial licensing, contact: info.cyber@hensoldt.net
#
# Create a root certificate (CA) and two signed certificates (CSRs).
#-------------------------------------------------------------------------------


SCRIPT_DIR="$(cd "$(dirname "$0")" >/dev/null 2>&1 && pwd)"

#-------------------------------------------------------------------------------
function print_usage_help()
{
    echo "Usage: $(basename $0) [-h|-v|-r]"
    echo "  -h : Show usage info (optional)."
    echo "  -v : Print and verify certificates (optional)."
    echo "  -r : Create new root certificate (optional)."
}

#-------------------------------------------------------------------------------
function print_err()
{
    local MSG=$1
    echo "ERROR: ${MSG}" >&2
}

#-------------------------------------------------------------------------------
# Arguments
#-------------------------------------------------------------------------------

VERIFY=0
CREATE_NEW_ROOT=0

if [ $# -ge 1 ]; then
    while getopts ":hvr" ARG; do
        case "${ARG}" in
            h)
                print_usage_help
                exit 0
                ;;
            v)
                VERIFY=1
                ;;
            r)
                CREATE_NEW_ROOT=1
                ;;
            \?)
                print_err "invalid parameter ${OPTARG}"
                print_usage_help
                exit 1
                ;;
        esac
    done
fi

#-------------------------------------------------------------------------------
# Certs folder
#-------------------------------------------------------------------------------

mkdir -p "${SCRIPT_DIR}/certs"
cd "${SCRIPT_DIR}/certs"

#-------------------------------------------------------------------------------
# Root certificate
#-------------------------------------------------------------------------------

if [[ ! -f "CA.key" || ! -f "CA.crt" ]]; then
    CREATE_NEW_ROOT=1
fi

if [ "$CREATE_NEW_ROOT" = 1 ]; then

    rm -f CA.srl # remove CA serial number file

    openssl genrsa -out CA.key 2048

    openssl req -x509 -new -nodes -key CA.key -sha256 -days 365 -out CA.crt \
        -subj "/C=DE/ST=Bayern/L=Ottobrunn/O=HENSOLDT Cyber GmbH/CN=dev-hc-root"

fi

#-------------------------------------------------------------------------------
# Server certificate
#-------------------------------------------------------------------------------

openssl genrsa -out server.key 2048

openssl req -new -key server.key -out server.csr \
    -subj "/C=DE/ST=Bayern/L=Ottobrunn/O=HENSOLDT Cyber GmbH/CN=dev-hc-server" \

openssl x509 -req -in server.csr -CA CA.crt -CAkey CA.key -CAcreateserial \
    -out server.crt -days 365 -sha256 -extfile ../server_cert.ext

#-------------------------------------------------------------------------------
# Client certificate
#-------------------------------------------------------------------------------

openssl genrsa -out client.key 2048

openssl req -new -key client.key -out client.csr \
    -subj "/C=DE/ST=Bayern/L=Ottobrunn/O=HENSOLDT Cyber GmbH/CN=dev-hc-client"

openssl x509 -req -in client.csr -CA CA.crt -CAkey CA.key -CAcreateserial \
    -out client.crt -days 365 -sha256

openssl pkcs12 -export -inkey client.key -in client.crt -passout pass: \
    -out client.p12

#-------------------------------------------------------------------------------
# Verify certificates
#-------------------------------------------------------------------------------

if [ ${VERIFY} -eq 1 ]; then
    openssl x509 -noout -text -in client.crt
    openssl x509 -noout -text -in server.crt
    openssl x509 -noout -text -in CA.crt

    openssl verify -CAfile CA.crt server.crt
    openssl verify -CAfile CA.crt client.crt
fi
