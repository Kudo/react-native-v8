import { createRunOncePlugin, withAppBuildGradle } from '@expo/config-plugins';
import { mergeContents, MergeResults } from '@expo/config-plugins/build/utils/generateCode';
import type { ConfigPlugin } from '@expo/config-plugins';

const withV8ExpoAdapter: ConfigPlugin = (config) => {
  return withAppBuildGradle(config, (config) => {
    if (config.modResults.language === 'groovy') {
      config.modResults.contents = updateAndroidAppGradle(config.modResults.contents);
    } else {
      throw new Error(
        'Cannot update app gradle file for react-native-v8 because the file is not groovy.'
      );
    }
    return config;
  });
};

const pkg = require('react-native-v8/package.json');

export default createRunOncePlugin(withV8ExpoAdapter, pkg.name, pkg.version);

/**
 * Update *android/app/build.gradle* to add the packaging options
 */
export function updateAndroidAppGradle(contents: string): string {
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
