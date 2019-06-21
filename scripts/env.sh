#!/bin/bash -e

function abs_path()
{
    readlink="readlink -f"
    if [[ "$(uname)" == "Darwin" ]]; then
        if [[ ! "$(command -v greadlink)" ]]; then
            echo "greadlink not found. Please install greadlink by \`brew install coreutils\`"
            exit 1
        fi
      readlink="greadlink -f"
    fi

    echo `$readlink $1`
}

CURR_DIR=$(dirname $(abs_path $0))
ROOT_DIR=$(dirname $CURR_DIR)
unset CURR_DIR

BUILD_DIR=$ROOT_DIR/build
DIST_DIR=$ROOT_DIR/dist
PATCHES_DIR=$ROOT_DIR/patches
