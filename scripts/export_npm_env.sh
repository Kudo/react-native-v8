#!/bin/bash -e
if [[ ${CIRCLECI} ]]; then
    echo "export RN_VERSION=${npm_package_config_RN_VERSION}" >> $BASH_ENV
else
    export RN_VERSION=${npm_package_config_RN_VERSION}
fi
