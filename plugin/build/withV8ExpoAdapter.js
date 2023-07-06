"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.updateIosAppDelegate = exports.updateAndroidAppGradle = void 0;
const config_plugins_1 = require("expo/config-plugins");
const generateCode_1 = require("./generateCode");
const withV8ExpoAdapter = (config, opts) => {
    const { android = true, ios = false } = opts ?? {};
    if (android) {
        config = withAndroidGradles(config);
    }
    if (ios) {
        config = withIosAppDelegate(config);
    }
    return config;
};
const pkg = require('react-native-v8/package.json');
exports.default = (0, config_plugins_1.createRunOncePlugin)(withV8ExpoAdapter, pkg.name, pkg.version);
/**
 * The config plugin for android
 */
const withAndroidGradles = (config) => {
    return (0, config_plugins_1.withAppBuildGradle)(config, (config) => {
        const jsEngine = config.android?.jsEngine ?? config.jsEngine;
        if (jsEngine !== 'jsc') {
            throw new Error('Must setup `expo.jsEngine` as `jsc` in app.json.');
        }
        if (config.modResults.language === 'groovy') {
            config.modResults.contents = updateAndroidAppGradle(config.modResults.contents);
        }
        else {
            throw new Error('Cannot update app gradle file for react-native-v8 because the file is not groovy');
        }
        return config;
    });
};
/**
 * The config plugin for ios
 */
const withIosAppDelegate = (config) => {
    return (0, config_plugins_1.withAppDelegate)(config, (config) => {
        const jsEngine = config.ios?.jsEngine ?? config.jsEngine;
        if (jsEngine !== 'jsc') {
            throw new Error('Must setup `expo.jsEngine` as `jsc` in app.json.');
        }
        if (config.modResults.language === 'objcpp') {
            config.modResults.contents = updateIosAppDelegate(config.modResults.contents);
        }
        else {
            throw new Error('Cannot update AppDelegate file for react-native-v8 because the file is not objcpp');
        }
        return config;
    });
};
/**
 * Updates **android/app/build.gradle** to add the packaging options
 */
function updateAndroidAppGradle(contents) {
    const mergeTag = 'react-native-v8';
    const packagingDsl = `
android {
    androidComponents {
        onVariants(selector().withBuildType('release')) {
            packaging.jniLibs.pickFirsts.set([])
            packaging.jniLibs.excludes.add('**/**/libjsc*')
            packaging.jniLibs.excludes.add('**/**/libhermes*')
        }
        onVariants(selector().withBuildType('debug')) {
            packaging.jniLibs.pickFirsts.set([])
            packaging.jniLibs.excludes.add('**/**/libjsc*')
            packaging.jniLibs.pickFirsts.add('**/**/libhermes*')
        }
    }
}
`;
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
/**
 * Updates **AppDelegate.mm**
 */
function updateIosAppDelegate(contents) {
    const mergeTagPrefix = 'react-native-v8';
    let didMerge = false;
    let didClear = false;
    const imports = `\
#ifndef FOLLY_NO_CONFIG
#define FOLLY_NO_CONFIG 1
#endif

#ifndef FOLLY_MOBILE
#define FOLLY_MOBILE 1
#endif

#ifndef FOLLY_USE_LIBCPP
#define FOLLY_USE_LIBCPP 1
#endif

#ifndef FOLLY_HAVE_PTHREAD
#define FOLLY_HAVE_PTHREAD 1
#endif

#import <memory>
#import <React/RCTCxxBridgeDelegate.h>
#import <React/RCTJSIExecutorRuntimeInstaller.h>
#import <RNV8/V8ExecutorFactory.h>
#if RCT_NEW_ARCH_ENABLED
#import <ReactCommon/RCTTurboModuleManager.h>
#endif
`;
    let mergeResults;
    mergeResults = (0, generateCode_1.mergeContents)({
        tag: `${mergeTagPrefix}:imports`,
        src: contents,
        newSrc: imports,
        anchor: /^#import "AppDelegate\.h"/m,
        offset: 1,
        comment: '//',
    });
    didMerge || (didMerge = mergeResults.didMerge);
    didClear || mergeResults.didClear;
    const cxxBridgeCategory = `\
#if RCT_NEW_ARCH_ENABLED
@interface AppDelegate() <RCTCxxBridgeDelegate, RCTTurboModuleManagerDelegate>
@end
#else
@interface AppDelegate() <RCTCxxBridgeDelegate>
@end
#endif // RCT_NEW_ARCH_ENABLED
`;
    mergeResults = (0, generateCode_1.mergeContents)({
        tag: `${mergeTagPrefix}:cxxBridgeCategory`,
        src: mergeResults.contents,
        newSrc: cxxBridgeCategory,
        anchor: /^@implementation AppDelegate$/m,
        offset: 0,
        comment: '//',
    });
    didMerge || (didMerge = mergeResults.didMerge);
    didClear || mergeResults.didClear;
    const jsExecutorFactoryForBridge = `
- (std::unique_ptr<facebook::react::JSExecutorFactory>)jsExecutorFactoryForBridge:(RCTBridge *)bridge
{
#if RCT_NEW_ARCH_ENABLED
  self.turboModuleManager = [[RCTTurboModuleManager alloc] initWithBridge:bridge
                                                                 delegate:self
                                                                jsInvoker:bridge.jsCallInvoker];
  // Necessary to allow NativeModules to lookup TurboModules
  [bridge setRCTTurboModuleRegistry:self.turboModuleManager];

#if RCT_DEV
  if (!RCTTurboModuleEagerInitEnabled()) {
    /**
     * Instantiating DevMenu has the side-effect of registering
     * shortcuts for CMD + d, CMD + i,  and CMD + n via RCTDevMenu.
     * Therefore, when TurboModules are enabled, we must manually create this
     * NativeModule.
     */
    [self.turboModuleManager moduleForName:"RCTDevMenu"];
  }
#endif // RCT_DEV

  return std::make_unique<rnv8::V8ExecutorFactory>(
      facebook::react::RCTJSIExecutorRuntimeInstaller([bridge, turboModuleManager = self.turboModuleManager](facebook::jsi::Runtime &runtime) {
        if (!bridge || !turboModuleManager) {
          return;
        }
        facebook::react::RuntimeExecutor syncRuntimeExecutor =
            [&](std::function<void(facebook::jsi::Runtime & runtime_)> &&callback) { callback(runtime); };
        [turboModuleManager installJSBindingWithRuntimeExecutor:syncRuntimeExecutor];
      }));
#else
  return std::make_unique<rnv8::V8ExecutorFactory>(facebook::react::RCTJSIExecutorRuntimeInstaller(nullptr));
#endif // RCT_NEW_ARCH_ENABLED
}
`;
    mergeResults = (0, generateCode_1.mergeContents)({
        tag: `${mergeTagPrefix}:jsExecutorFactoryForBridge`,
        src: mergeResults.contents,
        newSrc: jsExecutorFactoryForBridge,
        anchor: /^@end$/gm,
        findLastAnchor: true,
        offset: 0,
        comment: '//',
    });
    didMerge || (didMerge = mergeResults.didMerge);
    didClear || mergeResults.didClear;
    if (didMerge || didClear) {
        return mergeResults.contents;
    }
    return contents;
}
exports.updateIosAppDelegate = updateIosAppDelegate;
