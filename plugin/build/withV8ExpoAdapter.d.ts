import type { ConfigPlugin } from 'expo/config-plugins';
export type PluginOptions = {
    android?: boolean;
    ios?: boolean;
};
declare const _default: ConfigPlugin<PluginOptions>;
export default _default;
/**
 * Updates **android/app/build.gradle** to add the packaging options
 */
export declare function updateAndroidAppGradle(contents: string): string;
/**
 * Updates **AppDelegate.mm**
 */
export declare function updateIosAppDelegate(contents: string): string;
export declare function updateIosAppDelegate50(contents: string): string;
