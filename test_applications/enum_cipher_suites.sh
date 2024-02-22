#!/bin/bash -u

#-------------------------------------------------------------------------------
#
# Copyright (C) 2021-2024, HENSOLDT Cyber GmbH
# 
# SPDX-License-Identifier: GPL-2.0-or-later
#
# For commercial licensing, contact: info.cyber@hensoldt.net
#
# Enumerate and test OpenSSL cipher suites.
#-------------------------------------------------------------------------------


SCRIPT_DIR="$(cd "$(dirname "$0")" >/dev/null 2>&1 && pwd)"

cd ${SCRIPT_DIR}

declare -a CIPHERS_SUPPORTED
CIPHERS_AVAILABLE=$(openssl ciphers 'ALL:COMPLEMENTOFALL' | sed -e 's/:/ /g')

echo "$(openssl version)"
echo
echo "Testing cipher suites:"

for CIPHER in ${CIPHERS_AVAILABLE[@]}
do
    echo -n "Testing $CIPHER... "

    RESULT=$(echo -n | openssl s_client -tls1_2 \
        -CAfile certs/CA.crt -cert certs/client.crt -key certs/client.key \
        -connect 172.17.0.1:5560 \
        -cipher $CIPHER 2>&1)

    if [[ "$RESULT" =~ "Cipher is ${CIPHER}" ]]
    then
        echo -n "*** OK ***"
        CIPHERS_SUPPORTED+=($CIPHER)
    fi

    echo
done

echo
echo "Supported cipher suites:"

for CIPHER in "${CIPHERS_SUPPORTED[@]}"
do
    echo $CIPHER
done

echo
