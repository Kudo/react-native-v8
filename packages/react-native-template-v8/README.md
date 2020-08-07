# React Native Template V8
  The template to adopt V8 JavaScript Executor

## Supported Platform
- iOS

## Usage

```shell
npx react-native init RNV8IOS --template react-native-template-v8
```

For iOS, there is one additional handy step.

```shell
open RNV8IOS/ios/RNV8IOS.xcworkspace
```

After Xcode opened the workspace,
please select V8Executor project's `Build Phrases`,
drag and drop the `Codesign libv8.dylib` after `[CP] Copy Pods Resources`

<img src="https://user-images.githubusercontent.com/46429/89116746-38d22580-d4ca-11ea-908c-2d075c765baf.png" width="800">
