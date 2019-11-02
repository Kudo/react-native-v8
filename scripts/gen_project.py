#!/usr/bin/env python3
import argparse
import os
import subprocess
import sys
from lib.patcher import ProjectConfigPatcher

DEFAULT_NPM_RNV8_MAVEN_REPO = '$rootDir/../node_modules/react-native-v8/dist'
DEFAULT_NPM_V8_MAVEN_REPO = '$rootDir/../node_modules/v8-android/dist'


def parse_args():
    arg_parser = argparse.ArgumentParser()

    arg_parser.add_argument('--version',
                            '-V',
                            type=str,
                            required=True,
                            help='React Native version')
    arg_parser.add_argument(
        '--from',
        '-f',
        type=str,
        required=True,
        dest='rnv8_from',
        help='RNV8 install source, either "npm" or a maven repo path')

    arg_parser.add_argument('--v8_maven_repo',
                            type=str,
                            default=None,
                            help='Path to v8-android maven repo')

    arg_parser.add_argument('project_dir',
                            action='store',
                            help='Generated project dir')

    args = arg_parser.parse_args()
    has_error = False
    if args.rnv8_from != 'npm':
        if not os.path.isdir(args.rnv8_from):
            print('Install RNV8 source dir not existed - {}'.format(
                args.rnv8_from))
            has_error = True
        if args.v8_maven_repo is None:
            print(
                'v8_maven_repo should be explicitly specified for non-npm mode'
            )
            has_error = True

    if args.v8_maven_repo is not None and not os.path.isdir(
            args.v8_maven_repo):
        print('Explicit v8_maven_repo not existed - {}'.format(
            args.v8_maven_repo))
        has_error = True

    if os.path.exists(args.project_dir):
        print('Project dir existed - {}'.format(args.project_dir))
        has_error = True

    if has_error:
        arg_parser.print_help()
        sys.exit(1)

    return args


def main():
    args = parse_args()
    install_from_npm = args.rnv8_from == 'npm'
    rnv8_maven_repo = DEFAULT_NPM_RNV8_MAVEN_REPO if install_from_npm else os.path.abspath(
        args.rnv8_from)

    if install_from_npm:
        v8_maven_repo = os.path.abspath(
            args.v8_maven_repo
        ) if args.v8_maven_repo is not None else DEFAULT_NPM_V8_MAVEN_REPO
    else:
        v8_maven_repo = os.path.abspath(args.v8_maven_repo)

    subprocess.run([
        'npx', 'react-native', 'init', args.project_dir, '--version',
        args.version
    ])
    os.chdir(args.project_dir)
    if install_from_npm:
        next_version = args.version[:-1] + str(int(args.version[-1]) + 1)
        subprocess.run([
            'yarn', 'add',
            'react-native-v8@>={version}-patch.0 <{next_version}'.format(
                version=args.version, next_version=next_version)
        ])

    patcher = ProjectConfigPatcher(rnv8_maven_repo, v8_maven_repo)
    patcher.add_v8_support()
    patcher.add_vm_hint()
    subprocess.run(['react-native', 'run-android'])


if __name__ == '__main__':
    main()
