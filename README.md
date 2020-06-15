[![npm version](https://badge.fury.io/js/react-native-v8.svg)](https://badge.fury.io/js/react-native-v8)
[![CircleCI](https://circleci.com/gh/Kudo/react-native-v8.svg?style=svg)](https://circleci.com/gh/Kudo/react-native-v8)

# React Native with V8 Runtime

The aim of this project is to support V8 runtime for React Native. Designed as opt-in package, it should easy to integrate with existing React Native projects.

## Integration with React Native

To make RN integration easier, we publish prebuilt AAR into [npm](https://www.npmjs.com/package/react-native-v8).

The versioning is aligned with React Native but suffixed with a `-patch.N` in version
E.g.
If your React Native version is `0.60.0`, you should use react-native-v8 `>=0.60.0-patch.0 <0.60.1`.

Following steps will take 0.60.0 as example.

1. Install react-native-v8

```sh
yarn add 'react-native-v8@>=0.60.0-patch.0 <0.60.1'

# [OPTIONAL] If to use different V8 version
# yarn add 'v8-android@7.8.1'
```

2. Modify your React Native build.gradle

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

 dependencies {
     implementation fileTree(dir: "libs", include: ["*.jar"])
     implementation "com.facebook.react:react-native:+"  // From node_modules
+    // Add v8-android - prebuilt libv8android.so into APK
+    implementation 'org.chromium:v8-android:+'

     // JSC from node_modules
     if (useIntlJsc) {
--- a/android/build.gradle
+++ b/android/build.gradle
@@ -24,8 +24,12 @@ allprojects {
     repositories {
         mavenLocal()
         maven {
-            // All of React Native (JS, Obj-C sources, Android binaries) is installed from npm
-            url("$rootDir/../node_modules/react-native/android")
+            // Replace AAR from original RN with AAR from react-native-v8
+            url("$rootDir/../node_modules/react-native-v8/dist")
+        }
+        maven {
+            // prebuilt libv8android.so
+            url("$rootDir/../node_modules/v8-android/dist")
         }
         maven {
             // Android JSC is installed from npm
```

3. Gradle rebuild or `react-native run-android`

4. Start application and could verify by JS `global._v8runtime().version` which expected to return V8 version.

## Builtin JavaScript object

`global._v8runtime()` has some builtin information such as v8 version.

```js
console.log(`V8 version is ${global._v8runtime().version}`);
```

Please note that `global._v8runtime()` existed only for V8 enabled environment but not React Native remote debugging mode.
For remote debugging mode, the JavaScript actually runs on Chrome from your host and there is no V8Runtime.

## V8 Features Flags

V8 provides many feature flags and the most important one should be JIT.
Currently JIT is disabled for [V8 lite mode](https://v8.dev/blog/v8-lite)

react-native-v8 use the V8 shared libray from [v8-android-buildscripts](https://github.com/Kudo/v8-android-buildscripts).
For detailed V8 features, please check [there](https://github.com/Kudo/v8-android-buildscripts/blob/master/README.md#v8-feature-flags).

You can do the following to use one of these features in place of [V8 lite mode](https://v8.dev/blog/v8-lite) which is packaged as the default for this repository:

1. Add `v8-android-jit` via yarn or npm

```
$ npm i v8-android-jit
```

```
$ yarn add v8-android-jit
```

2. Modify the gradle dependency to use `v8-android-jit` or other variants

```diff
--- a/android/build.gradle
+++ b/android/build.gradle
@@ -29,7 +29,7 @@ allprojects {
         }
         maven {
             // prebuilt libv8android.so
-            url("$rootDir/../node_modules/v8-android/dist")
+            url("$rootDir/../node_modules/v8-android-jit/dist")
         }
         maven {
             // Android JSC is installed from npm
```

This method can also be used to swap to other variants of V8. All possible variants include:

- v8-android
- v8-android-nointl
- v8-android-jit
- v8-android-jit-nointl

Simply switch out `v8-android-jit` in the steps provided with the variant that you would like to use.

## FAQ

### How to reduce APK size ?

The V8 currently bundled by default supports [Intl](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Intl) and the ICU data costs about 7MiB per ABI. If you are not going to use `Intl`, you could use no-Intl version to reduce APK size. (jsc-android and Hermes have no Intl by default)

## TODO

- [x] Performance comparison with JavaScriptCore in React Native

  Please check https://github.com/Kudo/react-native-js-benchmark for the benchmark and result

- [ ] V8 inspector integration
- [ ] V8 snapshot integration
- [ ] V8 code cache experiment
