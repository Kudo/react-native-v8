"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.updateAndroidAppGradle = void 0;
const config_plugins_1 = require("@expo/config-plugins");
const generateCode_1 = require("@expo/config-plugins/build/utils/generateCode");
const withV8ExpoAdapter = (config) => {
    return (0, config_plugins_1.withAppBuildGradle)(config, (config) => {
        if (config.jsEngine !== 'jsc') {
            throw new Error('Must setup `expo.jsEngine` as `jsc` in app.json.');
        }
        if (config.modResults.language === 'groovy') {
            config.modResults.contents = updateAndroidAppGradle(config.modResults.contents);
        }
        else {
            throw new Error('Cannot update app gradle file for react-native-v8 because the file is not groovy.');
        }
        return config;
    });
};
const pkg = require('react-native-v8/package.json');
exports.default = (0, config_plugins_1.createRunOncePlugin)(withV8ExpoAdapter, pkg.name, pkg.version);
/**
 * Update *android/app/build.gradle* to add the packaging options
 */
function updateAndroidAppGradle(contents) {
    const mergeTag = 'react-native-v8';
    const packagingDsl = `
android {
    androidComponents {
        onVariants(selector().withBuildType('release')) {
            packaging.jniLibs.excludes.add('**/**/libjsc*')
            packaging.jniLibs.excludes.add('**/**/libhermes*')
        }
    }
}`;
    let mergeResults;
    mergeResults = (0, generateCode_1.mergeContents)({
        tag: mergeTag,
        src: contents,
        newSrc: packagingDsl,
        anchor: /^/,
        offset: contents.length,
        comment: '//',
    });
    if (mergeResults.didMerge || mergeResults.didClear) {
        return mergeResults.contents;
    }
    return contents;
}
exports.updateAndroidAppGradle = updateAndroidAppGradle;
