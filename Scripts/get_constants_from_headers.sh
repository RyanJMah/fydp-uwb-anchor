#!/bin/bash

set -e

THIS_DIR="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
REPO_ROOT="$(dirname "$THIS_DIR")"

CONSTANTS_PY_FILE="${THIS_DIR}/header_constants.py"

HEADERS_TO_SEACH=(                                  \
    "${REPO_ROOT}/Common/inc/flash_config_data.h"   \
    "${REPO_ROOT}/Common/inc/flash_memory_map.h"    \
    "${REPO_ROOT}/Common/inc/lan.h"                 \
)

# clear the constants file
set +e
rm $CONSTANTS_PY_FILE
touch $CONSTANTS_PY_FILE
set -e

for header in "${HEADERS_TO_SEACH[@]}"
do
    echo "Searching for constants in ${header}"
    cat $header | python3 $THIS_DIR/h2py.py >> $CONSTANTS_PY_FILE
    echo "" >> $CONSTANTS_PY_FILE
done
