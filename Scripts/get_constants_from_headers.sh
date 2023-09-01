#!/bin/bash

set -e

THIS_DIR="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
REPO_ROOT="$(dirname "$THIS_DIR")"

CONSTANTS_PY_FILE="${THIS_DIR}/header_constants.py"

HEADERS_TO_SEACH=(                                  \
    "${REPO_ROOT}/Common/inc/flash_config_data.h"   \
    "${REPO_ROOT}/Common/inc/flash_memory_map.h"    \
    "${REPO_ROOT}/Common/inc/lan.h"                 \
    "${REPO_ROOT}/Common/inc/dfu_messages.h"        \
)

function cat_headers()
{
    printf '%s\n' "${HEADERS_TO_SEACH[@]}" | xargs -I {} cat '{}'
}

# clear the constants file
set +e
rm $CONSTANTS_PY_FILE
touch $CONSTANTS_PY_FILE
set -e

# Want to cat all headers at once to get macros
# defined wrt other macros included in other headers

cat_headers | python3 $THIS_DIR/h2py.py >> $CONSTANTS_PY_FILE

