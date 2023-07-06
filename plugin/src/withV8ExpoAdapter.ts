import assert from 'assert';
import { createRunOncePlugin, withAppBuildGradle, withAppDelegate } from 'expo/config-plugins';
import type { ConfigPlugin } from 'expo/config-plugins';

import { mergeContents, MergeResults } from './generateCode';

export type PluginOptions = {
  android?: boolean;
  ios?: boolean;
}

const withV8ExpoAdapter: ConfigPlugin<PluginOptions> = (config, opts) => {
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

export default createRunOncePlugin(withV8ExpoAdapter, pkg.name, pkg.version);

/**
 * The config plugin for android
 */
const withAndroidGradles: ConfigPlugin = (config) => {
  return withAppBuildGradle(config, (config) => {
    const jsEngine = config.android?.jsEngine ?? config.jsEngine;
    if (jsEngine !== 'jsc') {
      throw new Error('Must setup `expo.jsEngine` as `jsc` in app.json.');
    }
    if (config.modResults.language === 'groovy') {
      config.modResults.contents = updateAndroidAppGradle(config.modResults.contents);
    } else {
      throw new Error(
        'Cannot update app gradle file for react-native-v8 because the file is not groovy'
      );
    }
    return config;
  });
};

/**
 * The config plugin for ios
 */
const withIosAppDelegate: ConfigPlugin = (config) => {
  return withAppDelegate(config, (config) => {
    const jsEngine = config.ios?.jsEngine ?? config.jsEngine;
    if (jsEngine !== 'jsc') {
      throw new Error('Must setup `expo.jsEngine` as `jsc` in app.json.');
    }
    if (config.modResults.language === 'objcpp') {
      config.modResults.contents = updateIosAppDelegate(config.modResults.contents);
    } else {
      throw new Error(
        'Cannot update AppDelegate file for react-native-v8 because the file is not objcpp'
      );
    }
    return config;
  });
};

/**
 * Updates **android/app/build.gradle** to add the packaging options
 */
export function updateAndroidAppGradle(contents: string): string {
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

dependencies {
    debugImplementation('com.facebook.react:hermes-android')
}
`;

  let mergeResults: MergeResults;
  mergeResults = mergeContents({
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

/**
 * Updates **AppDelegate.mm**
 */
export function updateIosAppDelegate(contents: string): string {
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
  let mergeResults: MergeResults;
  mergeResults = mergeContents({
    tag: `${mergeTagPrefix}:imports`,
    src: contents,
    newSrc: imports,
    anchor: /^#import "AppDelegate\.h"/m,
    offset: 1,
    comment: '//',
  });
  didMerge ||= mergeResults.didMerge;
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
  mergeResults = mergeContents({
    tag: `${mergeTagPrefix}:cxxBridgeCategory`,
    src: mergeResults.contents,
    newSrc: cxxBridgeCategory,
    anchor: /^@implementation AppDelegate$/m,
    offset: 0,
    comment: '//',
  });
  didMerge ||= mergeResults.didMerge;
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
  mergeResults = mergeContents({
    tag: `${mergeTagPrefix}:jsExecutorFactoryForBridge`,
    src: mergeResults.contents,
    newSrc: jsExecutorFactoryForBridge,
    anchor: /^@end$/gm,
    findLastAnchor: true,
    offset: 0,
    comment: '//',
  });
  didMerge ||= mergeResults.didMerge;
  didClear || mergeResults.didClear;

  if (didMerge || didClear) {
    return mergeResults.contents;
  }

  return contents;
}
