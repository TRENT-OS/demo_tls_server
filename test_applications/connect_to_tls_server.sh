#!/bin/bash -eu

#-------------------------------------------------------------------------------
# Copyright (C) 2021, HENSOLDT Cyber GmbH
#
# Start openssl client and connect to demo TLS server.
#-------------------------------------------------------------------------------

SCRIPT_DIR="$(cd "$(dirname "$0")" >/dev/null 2>&1 && pwd)"

#-------------------------------------------------------------------------------
function print_usage_help()
{
    echo "Usage: $(basename $0) [-h|-c <cipher>]"
    echo "  -h : Show usage info (optional)."
    echo "  -c : Use given cipher (optional)."
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

CIPHER=""

if [ $# -ge 1 ]; then
    while getopts ":hc:" ARG; do
        case "${ARG}" in
            h)
                print_usage_help
                exit 0
                ;;
            c)
                CIPHER="-cipher ${OPTARG}"
                ;;
            \?)
                print_err "invalid parameter ${OPTARG}"
                print_usage_help
                exit 1
                ;;
            :)
                print_err "incomplete parameter ${OPTARG}"
                print_usage_help
                exit 1
                ;;
        esac
    done
fi

#-------------------------------------------------------------------------------
# Connect
#-------------------------------------------------------------------------------

cd ${SCRIPT_DIR}

openssl s_client -tls1_2 -state -verify_return_error \
    -CAfile certs/CA.crt -cert certs/client.crt -key certs/client.key \
    -connect 172.17.0.1:5560 \
    ${CIPHER}
