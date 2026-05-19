#!/bin/sh

set -e

./build.sh

if [ $# -lt 2 ]; then
	echo "Usage: $0 <csv_file> <sql> [output_csv]" >&2
	exit 1
fi

./dbquery "$@"