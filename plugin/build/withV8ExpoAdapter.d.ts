import type { ConfigPlugin } from 'expo/config-plugins';
declare const _default: ConfigPlugin<void>;
export default _default;
/**
 * Updates **android/app/build.gradle** to add the packaging options
 */
export declare function updateAndroidAppGradle(contents: string): string;
/**
 * Updates **AppDelegate.mm**
 */
export declare function updateIosAppDelegate(contents: string): string;
