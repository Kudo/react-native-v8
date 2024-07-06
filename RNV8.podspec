require 'json'

package = JSON.parse(File.read(File.join(__dir__, 'package.json')))

reactNativeVersion = '0.0.0'
begin
  reactNativeVersion = `node --print "require('react-native/package.json').version"`
rescue
  reactNativeVersion = '0.0.0'
end
if ENV["REACT_NATIVE_OVERRIDE_VERSION"]
  reactNativeVersion = ENV["REACT_NATIVE_OVERRIDE_VERSION"]
end
reactNativeVersions = reactNativeVersion.split('.')
reactNativeMinorVersion = reactNativeVersions[1].to_i
reactNativePatchVersion = reactNativeVersions[2].to_i

folly_compiler_flags = '-DFOLLY_NO_CONFIG -DFOLLY_MOBILE=1 -DFOLLY_USE_LIBCPP=1 -DFOLLY_CFG_NO_COROUTINES=1 -Wno-comma -Wno-shorten-64-to-32'

Pod::Spec.new do |s|
  s.name        = 'RNV8'
  s.version     = package['version']
  s.summary     = package['description']
  s.description = package['description']
  s.license     = package['license']
  s.author      = package['author']
  s.homepage    = package['homepage']
  s.platform    = :ios, '13.0'

  s.source      = { git: 'https://github.com/Kudo/react-native-v8.git' }
  s.source_files = 'src/v8runtime/**/*.{h,cpp}', 'src/iosexecutor/**/*.{h,cpp,m,mm}'

  s.pod_target_xcconfig = {
    'CLANG_CXX_LANGUAGE_STANDARD' => 'c++17',
    'HEADER_SEARCH_PATHS' => "\"$(PODS_ROOT)/boost\"",
  }

  compiler_flags = folly_compiler_flags + ' ' + "-DREACT_NATIVE_MINOR_VERSION=#{reactNativeMinorVersion} -DREACT_NATIVE_PATCH_VERSION=#{reactNativePatchVersion}"
  s.compiler_flags = compiler_flags

  s.dependency 'v8-ios'
  s.dependency 'React-Core'
  s.dependency 'React-cxxreact'
end
