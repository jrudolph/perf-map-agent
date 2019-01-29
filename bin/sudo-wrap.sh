#!/bin/bash

function runasroot() {
    if [[ "$EUID" -ne 0 ]]; then
        sudo $@
    else
        $@
    fi
}
