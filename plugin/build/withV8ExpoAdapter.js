"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const config_plugins_1 = require("@expo/config-plugins");
const expo_build_properties_1 = require("expo-build-properties");
const withV8ExpoAdapter = (config) => {
    return (0, expo_build_properties_1.withBuildProperties)(config, {
        android: {
            packagingOptions: {
                exclude: ["**/**/libjsc*"],
            },
        },
    });
};
const pkg = require("react-native-v8/package.json");
exports.default = (0, config_plugins_1.createRunOncePlugin)(withV8ExpoAdapter, pkg.name, pkg.version);
