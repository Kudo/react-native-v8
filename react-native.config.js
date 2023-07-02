const v8IosInstalled = (function() {
 try {
    return !!require.resolve('v8-ios/package.json');
 } catch {
    return false;
 }
})();

module.exports = {
  dependency: {
    platforms: {
      // Only enable ios autolinking when v8-ios is installed
      ios: v8IosInstalled ? {} : null,
    },
  },
};
