#!/bin/sh

set -e

if [ ! -f build/dbquery ]; then
    ./build.sh
fi

if [ $# -eq 1 ]; then
    ./build/dbquery "$1"
elif [ $# -eq 2 ]; then
    ./build/dbquery "$1" "$2"
else
    echo "Usage: $0 <data_file> [sql_command]" >&2
    exit 1
fi