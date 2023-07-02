module.exports = {
  dependency: {
    platforms: {
      // Only enable ios autolinking when v8-ios is installed
      ios: !!require.resolve('v8-ios/package.json'),
    },
  },
};
