#!/bin/bash

function runasroot() {
    WANT_UID=0
    if [[ "$1" == "-u" ]]; then
        WANT_UID=$2
    fi

    if [[ "$EUID" -ne "${WANT_UID}" ]]; then
        command -v sudo
        if [[ $? -eq 0 ]]; then
            sudo $@
        else
            echo "sudo is missing"
            exit 1
        fi
    else
        $@
    fi
}
