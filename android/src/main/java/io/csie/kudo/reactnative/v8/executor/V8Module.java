package io.csie.kudo.reactnative.v8.executor;

import android.os.Build;
import android.os.Looper;
import android.os.MessageQueue;
import android.os.SystemClock;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.ReactContextBaseJavaModule;
import com.facebook.react.bridge.RuntimeExecutor;
import com.facebook.react.bridge.UiThreadUtil;

public class V8Module
    extends ReactContextBaseJavaModule implements MessageQueue.IdleHandler {
  private long mLastMainLoopIdleCallbackTime = 0;
  private static final long MAIN_LOOP_IDLE_THROTTLE = 1000;

  public V8Module(ReactApplicationContext reactContext) {
    super(reactContext);
    registerMainIdleHandler();
  }

  @Override
  public void onCatalystInstanceDestroy() {
    unregisterMainIdleHandler();
    super.onCatalystInstanceDestroy();
  }

  @Override
  public String getName() {
    return "V8Module";
  }

  private void registerMainIdleHandler() {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
      Looper.getMainLooper().getQueue().addIdleHandler(this);
    } else {
      UiThreadUtil.runOnUiThread(
          () -> { Looper.myQueue().addIdleHandler(this); });
    }
  }

  private void unregisterMainIdleHandler() {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
      Looper.getMainLooper().getQueue().removeIdleHandler(this);
    } else {
      UiThreadUtil.runOnUiThread(
          () -> { Looper.myQueue().removeIdleHandler(this); });
    }
  }

  // MessageQueue.IdleHandler implementations

  @Override
  public boolean queueIdle() {
    if (getReactApplicationContext().hasActiveReactInstance() &&
        SystemClock.uptimeMillis() - mLastMainLoopIdleCallbackTime >
            MAIN_LOOP_IDLE_THROTTLE) {
      final RuntimeExecutor runtimeExecutor = getReactApplicationContext()
                                                  .getCatalystInstance()
                                                  .getRuntimeExecutor();
      V8Executor.onMainLoopIdle(runtimeExecutor);
      mLastMainLoopIdleCallbackTime = SystemClock.uptimeMillis();
    }
    return true;
  }
}
