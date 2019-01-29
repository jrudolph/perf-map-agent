#!/bin/bash

function runasroot() {
    if [[ "$EUID" -ne 0 ]]; then
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
