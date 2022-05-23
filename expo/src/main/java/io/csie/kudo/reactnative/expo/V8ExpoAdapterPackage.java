/*
 * Copyright (c) Kudo Chien.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */
package io.csie.kudo.reactnative.expo;

import android.content.Context;

import androidx.annotation.Nullable;

import com.facebook.react.bridge.JavaScriptExecutorFactory;
import com.facebook.react.modules.systeminfo.AndroidInfoHelpers;

import java.util.Collections;
import java.util.List;

import expo.modules.core.interfaces.Package;
import expo.modules.core.interfaces.ReactNativeHostHandler;
import io.csie.kudo.reactnative.v8.executor.V8ExecutorFactory;

public class V8ExpoAdapterPackage implements Package {
    @Override
    public List<? extends ReactNativeHostHandler> createReactNativeHostHandlers(Context context) {
        return Collections.singletonList(new ReactNativeHostHandler() {

            @Nullable
            @Override
            public String getBundleAssetName(boolean useDeveloperSupport) {
                final String v8BundleAssetName = V8ExecutorFactory.getBundleAssetName(context, useDeveloperSupport);
                if (v8BundleAssetName != null) {
                    return v8BundleAssetName;
                }
                return null;
            }

            @Override
            public JavaScriptExecutorFactory getJavaScriptExecutorFactory() {
                return new V8ExecutorFactory(
                        context,
                        context.getPackageName(),
                        AndroidInfoHelpers.getFriendlyDeviceName(),
                        BuildConfig.DEBUG);
            }
        });
    }
}
