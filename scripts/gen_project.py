#!/usr/bin/env python3
import argparse
import glob
import os
import subprocess
import sys
from lib.patcher import ProjectConfigPatcher

RNV8_REPO_ROOT = os.path.dirname(os.path.dirname(__file__))


def parse_args():
    arg_parser = argparse.ArgumentParser()

    arg_parser.add_argument(
        "--rn-version", type=str, required=True, help="react-native version"
    )
    arg_parser.add_argument(
        "--npm-source",
        type=str,
        help="react-native-v8 install source from npm",
    )
    arg_parser.add_argument(
        "--v8-android-variant",
        type=str,
        default="v8-android-jit",
        help="v8-android variant to install",
    )

    arg_parser.add_argument("project_dir", action="store", help="Generated project dir")

    args = arg_parser.parse_args()

    has_error = False
    if os.path.exists(args.project_dir):
        print("Project dir existed - {}".format(args.project_dir))
        has_error = True

    if has_error:
        arg_parser.print_help()
        sys.exit(1)

    return args


def main():
    args = parse_args()
    subprocess.run(
        ["bun", "link"],
    )
    subprocess.run(
        ["bunx", "@react-native-community/cli", "init", args.project_dir, "--pm", "bun", "--version", args.rn_version]
    )

    os.chdir(args.project_dir)
    subprocess.run(["bun", "add", args.v8_android_variant])

    if not args.npm_source:
        subprocess.run(
            [
                "bun",
                "link",
                "react-native-v8",
                "--save",
            ]
        )
    else:
        subprocess.run(["bun", "add", "react-native-v8@{}".format(args.npm_source)])

    patcher = ProjectConfigPatcher()
    patcher.add_v8_support()
    patcher.add_vm_hint()
    subprocess.run(
        ["bun", "react-native", "run-android", "--mode", "release", "--no-packager"]
    )


if __name__ == "__main__":
    main()
