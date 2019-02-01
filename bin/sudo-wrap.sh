#!/bin/bash

function runasroot() {
    WANT_UID=0
    if [[ "$1" == "-u" ]]; then
        WANT_UID=$2
        shift 2
    fi

    if [[ "$1" == "-g" ]]; then
        WANT_GID=$2
        shift 2
    fi

    if [[ "\#$EUID" == "${WANT_UID}" || "\#$EGID" == "${WANT_GID}" ]]; then
        command -v sudo
        if [[ $? -eq 0 ]]; then
            sudo -u ${WANT_UID} -g ${WANT_GID} $@
        else
            echo "sudo is missing"
            exit 1
        fi
    else
        $@
    fi
}
