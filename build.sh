#!/bin/sh

set -e

if command -v apt-get >/dev/null 2>&1; then
    apt-get update
    apt-get install -y make g++
elif command -v dnf >/dev/null 2>&1; then
    dnf install -y make gcc-c++
elif command -v yum >/dev/null 2>&1; then
    yum install -y make gcc-c++
elif command -v pacman >/dev/null 2>&1; then
    pacman -S --noconfirm make gcc
elif command -v apk >/dev/null 2>&1; then
    apk add make g++
elif command -v zypper >/dev/null 2>&1; then
    zypper install -y make gcc-c++
fi

make