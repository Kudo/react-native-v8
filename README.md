[![npm version](https://badge.fury.io/js/react-native-v8.svg)](https://badge.fury.io/js/react-native-v8)
[![Build for Android](https://github.com/Kudo/react-native-v8/actions/workflows/android.yml/badge.svg)](https://github.com/Kudo/react-native-v8/actions/workflows/android.yml)

# React Native with V8 Runtime

This project aims to support V8 replacement runtime for React Native. Designed as opt-in package, it is easy to integrate with existing React Native projects.

## Installation for Expo projects (>= SDK 45)

For managed projects, you can install through the single command:

```sh
$ expo install react-native-v8 expo-build-properties
```

- Please make sure you don't have [`"jsEngine": "hermes"`](https://docs.expo.dev/guides/using-hermes/#android-setup).

For bare projects, you can run `expo prebuild -p android --clean` after the installation to prebuild again.

## Installation for React Native >= 0.66

1. Install `react-native-v8` and a [v8-android variant](#v8-variants). For example, the `v8-android-jit`:

```sh
$ yarn add react-native-v8 v8-android-jit
```

2. Exclude unused libraries to reduce APK size

````diff
--- a/android/app/build.gradle
+++ b/android/app/build.gradle
@@ -161,11 +161,18 @@ android {
             }
         }
     }
+
+    packagingOptions {
+        // Make sure libjsc.so does not packed in APK
+        exclude "**/libjsc.so"
+    }
 }

3. Setup V8 in the `MainApplication.java`.

```diff
--- a/android/app/src/main/java/com/rn067/MainApplication.java
+++ b/android/app/src/main/java/com/rn067/MainApplication.java
@@ -2,15 +2,20 @@ package com.rn067;

 import android.app.Application;
 import android.content.Context;
+
 import com.facebook.react.PackageList;
 import com.facebook.react.ReactApplication;
 import com.facebook.react.ReactInstanceManager;
 import com.facebook.react.ReactNativeHost;
 import com.facebook.react.ReactPackage;
+import com.facebook.react.bridge.JavaScriptExecutorFactory;
+import com.facebook.react.modules.systeminfo.AndroidInfoHelpers;
 import com.facebook.soloader.SoLoader;
 import java.lang.reflect.InvocationTargetException;
 import java.util.List;

+import io.csie.kudo.reactnative.v8.executor.V8ExecutorFactory;
+
 public class MainApplication extends Application implements ReactApplication {

   private final ReactNativeHost mReactNativeHost =
@@ -33,6 +39,14 @@ public class MainApplication extends Application implements ReactApplication {
         protected String getJSMainModuleName() {
           return "index";
         }
+
+        @Override
+        protected JavaScriptExecutorFactory getJavaScriptExecutorFactory() {
+          return new V8ExecutorFactory(
+              getApplicationContext(),
+              getPackageName(),
+              AndroidInfoHelpers.getFriendlyDeviceName(),
+              getUseDeveloperSupport());
+        }
       };

   @Override
````

4. [OPTIONAL] If you encounter gradle errors from JVM Out-of-memory, please increase the JVM memory size

```diff
--- a/android/gradle.properties
+++ b/android/gradle.properties
@@ -11,6 +11,7 @@
 # The setting is particularly useful for tweaking memory settings.
 # Default value: -Xmx1024m -XX:MaxPermSize=256m
 # org.gradle.jvmargs=-Xmx2048m -XX:MaxPermSize=512m -XX:+HeapDumpOnOutOfMemoryError -Dfile.encoding=UTF-8
+org.gradle.jvmargs=-Xmx2048m -XX:MaxMetaspaceSize=512m

 # When configured, Gradle will run in incubating parallel mode.
 # This option should only be used with decoupled projects. More details, visit
```

5. `yarn android`

6. You can verify whether it works by evaluating the `global._v8runtime().version` JavaScript statement.

## Installation for React Native < 0.66

`react-native-v8` v1 does not support previous react-native versions. Please check [the installation guide from the previous version](https://github.com/Kudo/react-native-v8/blob/0.67-stable/README.md).

## Builtin JavaScript object

`global._v8runtime()` has some builtin information such as v8 version.

```js
console.log(`V8 version is ${global._v8runtime().version}`);
```

Please note that `global._v8runtime()` existed only for V8 enabled environment but not React Native remote debugging mode.
For remote debugging mode, the JavaScript actually runs on Chrome from your host and there is no V8Runtime.

## V8 Variants

`react-native-v8` depends on a V8 shared library built from [v8-android-buildscripts](https://github.com/Kudo/v8-android-buildscripts). `v8-android-jit` is the recommended V8 variant. This is a full featured V8 with both [JIT](https://en.wikipedia.org/wiki/Just-in-time_compilation) and [Intl](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Intl). We provide other V8 variants to fulfill your needs.
For instance, if you want to reduce memory usage, the non JIT variant, aka [V8 lite mode](https://v8.dev/blog/v8-lite).
For detailed V8 features, please check [the v8-android-buildscripts feature flags](https://github.com/Kudo/v8-android-buildscripts/blob/master/README.md#v8-feature-flags).

| Package name            | JIT | Intl |
| ----------------------- | --- | ---- |
| `v8-android-jit`        | Yes | Yes  |
| `v8-android-jit-nointl` | Yes | No   |
| `v8-android`            | No  | Yes  |
| `v8-android-nointl`     | No  | No   |

## iOS Support (Experimented)

We did have experimented iOS support. To adopt V8 for Xcodeproj gets a little complicated, so we have a pre-shaped template.
Please check [react-native-template-v8](packages/react-native-template-v8/README.md) for more information.

## FAQ

### How to reduce APK size ?

The V8 currently bundled by default supports [Intl](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Intl) and the ICU data costs about 7MiB per ABI. If you are not going to use `Intl`, you could use no-Intl version to reduce APK size. (jsc-android have no Intl by default)

## TODO

- [x] Performance comparison with JavaScriptCore in React Native

  Please check https://github.com/Kudo/react-native-js-benchmark for the benchmark and result

- [x] V8 inspector integration
- [ ] V8 snapshot integration
- [ ] V8 code cache experiment
