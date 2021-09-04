#!/bin/bash -e
if [[ ${CIRCLECI} ]]; then
    echo "export RN_VERSION=${npm_package_config_RN_VERSION}" >> $BASH_ENV
elif [[ ${GITHUB_ACTIONS} ]]; then
    echo "RN_VERSION=${npm_package_config_RN_VERSION}" >> $GITHUB_ENV
else
    export RN_VERSION=${npm_package_config_RN_VERSION}
fi
