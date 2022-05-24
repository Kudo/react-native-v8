import { createRunOncePlugin } from "@expo/config-plugins";
import type { ConfigPlugin } from "@expo/config-plugins";
import { withBuildProperties } from "expo-build-properties";

const withV8ExpoAdapter: ConfigPlugin = (config) => {
  return withBuildProperties(config, {
    android: {
      packagingOptions: {
        exclude: ["**/**/libjsc*"],
      },
    },
  });
};

const pkg = require("react-native-v8/package.json");

export default createRunOncePlugin(withV8ExpoAdapter, pkg.name, pkg.version);
