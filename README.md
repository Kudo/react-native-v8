[![npm version](https://badge.fury.io/js/react-native-v8.svg)](https://badge.fury.io/js/react-native-v8)
[![Build for Android](https://github.com/Kudo/react-native-v8/actions/workflows/android.yml/badge.svg)](https://github.com/Kudo/react-native-v8/actions/workflows/android.yml)

# React Native with V8 Runtime

This project aims to support V8 replacement runtime for React Native. Designed as opt-in package, it is easy to integrate with existing React Native projects.

> **Note** From react-native-v8 2.0, the minimum requirement for react-native is 0.71.2.

For earlier versions, please check out the older readmes.

- [react-native>=0.66.0 readme](https://github.com/Kudo/react-native-v8/blob/1.0-stable/README.md)
- [react-native<0.66.0 readme](https://github.com/Kudo/react-native-v8/blob/0.67-stable/README.md)

## Installation for Expo projects (>= SDK 48)

For managed projects, you can install through the single command:

```sh
$ npx expo install react-native-v8 v8-android-jit
```

- Please make sure you have [`"android.jsEngine": "jsc"`](https://docs.expo.dev/versions/latest/config/app/#jsengine-2).

For bare projects, you can run `npx expo prebuild -p android --clean` after the installation to prebuild again.

## Installation for React Native >= 0.71

1. Install `react-native-v8` and a [v8-android variant](#v8-variants). For example, the `v8-android-jit`:

```sh
$ yarn add react-native-v8 v8-android-jit
```

2. Exclude unused libraries to reduce APK size

```diff
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
```

3. Setup V8 in the `MainApplication.java`.

```diff
--- a/android/app/src/main/java/com/rn071/MainApplication.java
+++ b/android/app/src/main/java/com/rn071/MainApplication.java
@@ -1,15 +1,20 @@
 package com.rn071;

 import android.app.Application;
+
 import com.facebook.react.PackageList;
 import com.facebook.react.ReactApplication;
 import com.facebook.react.ReactNativeHost;
 import com.facebook.react.ReactPackage;
+import com.facebook.react.bridge.JavaScriptExecutorFactory;
 import com.facebook.react.defaults.DefaultNewArchitectureEntryPoint;
 import com.facebook.react.defaults.DefaultReactNativeHost;
+import com.facebook.react.modules.systeminfo.AndroidInfoHelpers;
 import com.facebook.soloader.SoLoader;
 import java.util.List;

+import io.csie.kudo.reactnative.v8.executor.V8ExecutorFactory;
+
 public class MainApplication extends Application implements ReactApplication {

   private final ReactNativeHost mReactNativeHost =
@@ -42,6 +47,15 @@ public class MainApplication extends Application implements ReactApplication {
         protected Boolean isHermesEnabled() {
           return BuildConfig.IS_HERMES_ENABLED;
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
```

For React Native 0.73 and above, you can use the following code instead:

```diff
--- a/android/app/src/main/java/com/rn073/MainApplication.kt
+++ b/android/app/src/main/java/com/rn073/MainApplication.kt
@@ -1,15 +1,20 @@
 package com.rn073;

 import android.app.Application
+
 import com.facebook.react.PackageList
 import com.facebook.react.ReactApplication
 import com.facebook.react.ReactHost
 import com.facebook.react.ReactNativeHost
 import com.facebook.react.ReactPackage
+import com.facebook.react.bridge.JavaScriptExecutorFactory
 import com.facebook.react.defaults.DefaultNewArchitectureEntryPoint.load
 import com.facebook.react.defaults.DefaultReactHost.getDefaultReactHost
 import com.facebook.react.defaults.DefaultReactNativeHost
 import com.facebook.react.flipper.ReactNativeFlipper
+import com.facebook.react.modules.systeminfo.AndroidInfoHelpers
 import com.facebook.soloader.SoLoader

+import io.csie.kudo.reactnative.v8.executor.V8ExecutorFactory;
+
 class MainApplication : Application(), ReactApplication {

   override val reactNativeHost: ReactNativeHost =
      object : DefaultReactNativeHost(this) {
        override fun getPackages(): List<ReactPackage> =
            PackageList(this).packages.apply {
              // Packages that cannot be auto linked yet can be added manually here, for example:
              // add(MyReactNativePackage())
            }
        override fun getJSMainModuleName(): String = "index"

        override fun getUseDeveloperSupport(): Boolean = BuildConfig.DEBUG

        override val isNewArchEnabled: Boolean = BuildConfig.IS_NEW_ARCHITECTURE_ENABLED
        override val isHermesEnabled: Boolean = BuildConfig.IS_HERMES_ENABLED
+
+       override fun getJavaScriptExecutorFactory(): JavaScriptExecutorFactory =
+            V8ExecutorFactory(
+                applicationContext,
+                packageName,
+                AndroidInfoHelpers.getFriendlyDeviceName(),
+                useDeveloperSupport
+            )
       };

    override val reactHost: ReactHost
```

4. Disable Hermes

```diff
--- a/android/gradle.properties
+++ b/android/gradle.properties
@@ -41,4 +41,4 @@ newArchEnabled=false

 # Use this property to enable or disable the Hermes JS engine.
 # If set to false, you will be using JSC instead.
-hermesEnabled=true
+hermesEnabled=false
```

5. `yarn android`

6. You can verify whether it works by evaluating the `global._v8runtime().version` JavaScript statement.

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

## iOS Support (Experimental)

You could try the experimental ios support on an Expo project which SDK version is greater or equal to 48.

For managed projects, you can install through the single command:

```sh
$ npx expo install react-native-v8 v8-ios
```

- Please make sure you have [`"ios.jsEngine": "jsc"`](https://docs.expo.dev/versions/latest/config/app/#jsengine-1).

For bare projects, you can run `npx expo prebuild -p ios --clean` after the installation to prebuild again.

## FAQ

### How to reduce APK size ?

The V8 currently bundled by default supports [Intl](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Intl) and the ICU data costs about 7MiB per ABI. If you are not going to use `Intl`, you could use no-Intl version to reduce APK size. (jsc-android have no Intl by default)

## TODO

- [x] Performance comparison with JavaScriptCore in React Native

  Please check https://github.com/Kudo/react-native-js-benchmark for the benchmark and result

- [x] V8 inspector integration
- [ ] V8 snapshot integration
- [ ] V8 code cache experiment
