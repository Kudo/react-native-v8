Change Log
==========

Version 0.61.4-patch.0 / 0.60.6-patch.1 / 0.59.10-patch.3 *(2019-11-05)*
-------------------------------------------------------

 * Upgrade v8-android to 7.8.0 (V8 7.8.279.17)
 * Add dedicate V8Executor instead of patching JSCExecutor to prevent annoying error like https://github.com/Kudo/react-native-v8/issues/29
 
 Also backport for
 * 0.61.2-patch.0
 * 0.61.1-patch.2
 * 0.61.3-patch.0
 
Version 0.60.5-patch.0 *(2019-08-29)*
-----------------------------

 * Support React Native v0.60.5
 
Version 0.60.4-patch.2 / 0.59.10-patch.2 *(2019-08-29)*
-------------------------------------------------------

 * Fix parsing UTF-8 JS script error
 * Fix JSI crash during convert property numeric names
 
Version 0.60.4-patch.1 *(2019-08-14)*
-----------------------------

 * Add heap memory statistics `global._v8runtime().memory`
 
Version 0.60.4-patch.0 *(2019-07-25)*
-----------------------------

 * Support React Native v0.60.4

Version 0.60.3-patch.0 *(2019-07-16)*
-----------------------------

 * Support React Native v0.60.3

Version 0.60.0-patch.1 *(2019-07-08)*
-----------------------------

 * Fix: Crash or JS exception from V8PointerValue being GCed.


Version 0.60.0-patch.0 *(2019-07-03)*
-----------------------------

 * Release V8 runtime support for React Native.
