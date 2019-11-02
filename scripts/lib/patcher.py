import io
import os
import re


class ProjectConfigPatcher:
    def __init__(self, rnv8_maven_repo, v8_maven_repo):
        self._rnv8_maven_repo = rnv8_maven_repo
        self._v8_maven_repo = v8_maven_repo

    @classmethod
    def _replace_file_content(cls,
                              file_path,
                              old_pattern,
                              new_pattern,
                              re_flags=0):
        with io.open(file_path, 'r', encoding='utf8') as f:
            content = str(f.read())
            new_content = re.sub(old_pattern,
                                 new_pattern,
                                 content,
                                 flags=re_flags)
        with io.open(file_path, 'w', encoding='utf8') as f:
            f.write(new_content)

    def _patch_root_gradle(self):
        root_gradle_path = os.path.abspath('android/build.gradle')
        self._replace_file_content(
            root_gradle_path,
            r'url\("\$rootDir/../node_modules/react-native/android"\)',
            'url("{}")'.format(self._rnv8_maven_repo))
        v8_maven_content = '''
        maven {{
            url("{}")
        }}'''.format(self._v8_maven_repo)
        self._replace_file_content(root_gradle_path,
                                   r'(maven {.*?/jsc-android/.*?})',
                                   '\\1' + v8_maven_content,
                                   re_flags=re.DOTALL)

    def _get_patched_packging_option(self, content):
        '''Add libjsc.so exclusion from packagingOptions

        NOTE(kudo): To keep implementation simple, here to use regexp instead of AST parsing.
        The drawback is that the build.gradle format should be fixed.

        Two cases:
        1. If there's no packagingOptions, add the section directly.
        2. If there is existing packagingOptions, append exclude libjsc.so and remove all libjsc.so lines.
        '''
        result = re.search(
            r'(?P<head>.*)(?P<body>^android {.*?^}$)(?P<foot>.*)', content,
            (re.DOTALL | re.MULTILINE))
        android_config_content = result.group('body')
        packaging_options_result = re.search(
            r'(?P<head>.*)^\s*?packagingOptions {\n(?P<body>.*?)\n\s*?}$(?P<foot>.*)',
            android_config_content, (re.DOTALL | re.MULTILINE))
        if packaging_options_result is None:
            packaging_options_content = '''
    packagingOptions {
        // Make sure libjsc.so does not packed in APK
        exclude "**/libjsc.so"
    }
'''
            new_android_config_content = android_config_content[:-1] \
                + packaging_options_content + '}\n'
        else:
            packaging_options_lines = \
                packaging_options_result.group('body').split('\n')
            packaging_options_lines = [
                line for line in packaging_options_lines
                if line.find('/libjsc.so') < 0
            ]
            packaging_options_lines.append('        exclude "**/libjsc.so"')
            packaging_options_content = '    packagingOptions {\n' \
                + '\n'.join(packaging_options_lines) \
                + '\n    }'
            new_android_config_content = packaging_options_result.group('head') \
                + packaging_options_content \
                + packaging_options_result.group('foot')

        new_content = result.group('head') \
            + new_android_config_content \
            + result.group('foot')

        return new_content

    def _patch_app_gradle(self):
        app_gradle_path = os.path.abspath('android/app/build.gradle')
        v8_dependency_content = '''\n    implementation \'org.chromium:v8-android:+\''''
        self._replace_file_content(
            app_gradle_path,
            r'(implementation "com.facebook.react:react-native:\+".*?$)',
            '\\1' + v8_dependency_content,
            re_flags=(re.DOTALL | re.MULTILINE))

        with io.open(app_gradle_path, 'r', encoding='utf8') as f:
            content = str(f.read())
        new_content = self._get_patched_packging_option(content)
        with io.open(app_gradle_path, 'w', encoding='utf8') as f:
            f.write(new_content)

    def _patch_appjs(self):
        appjs_path = os.path.abspath('App.js')
        v8_block = '''
          {global._v8runtime && (
            <View style={styles.engine}>
              <Text style={styles.footer}>
                Engine: V8 {global._v8runtime().version}
                {console.log(`=== V8 version[${global._v8runtime().version}] ===`)}
              </Text>
            </View>
          )}'''
        self._replace_file_content(
            appjs_path,
            r'({global.HermesInternal == null \? null.*?\)}$)',
            '\\1' + v8_block,
            re_flags=(re.DOTALL | re.MULTILINE))

    def add_v8_support(self):
        self._patch_root_gradle()
        self._patch_app_gradle()

    def add_vm_hint(self):
        self._patch_appjs()
