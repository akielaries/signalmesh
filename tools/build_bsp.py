#!/usr/bin/python3

"""
This builds the BSP (board support package) for a given module. BSP is used
loosely here and really just means the ChibiOS dependencies for the given
module's board
"""

import argparse
import subprocess
import os
import sys
import shutil
from pathlib import Path


COMMON_MAIN_C = Path("modules/core/bsp/src/main.c")
MAIN_DEST_REL = "main.c"


def copy_bsp(module_path):
    build_dir = Path(module_path) / "build"
    build_dir.mkdir(parents=True, exist_ok=True)  # create build directory if missing

    dest_main_c = build_dir / "main.c"
    shutil.copy(COMMON_MAIN_C, dest_main_c)
    print(f"Copied {COMMON_MAIN_C} to {dest_main_c}")

def clean_bsp(module_path):
    print("Cleaning BSP")
    subprocess.run(["make", "clean"], cwd=module_path, check=True)
    #subprocess.run(["rm", f"{module_path}/build/main.c"])

def run_make(makefile_dir):
    if not os.path.exists(os.path.join(makefile_dir, "Makefile")):
        print(f"No Makefile found in: {makefile_dir}")
        sys.exit(1)

    print(f"Building BSP from: {makefile_dir}")

    try:
        subprocess.run(["make", "-j4"], cwd=makefile_dir, check=True)
        print("Build completed successfully")
    except subprocess.CalledProcessError as e:
        print(f"Build failed with error code {e.returncode}")
        sys.exit(e.returncode)

def main():
    parser = argparse.ArgumentParser(description="Build BSP for a module using its Makefile")
    parser.add_argument(
        "--device",
        required=True,
        help="Path to module directory (e.g., modules/aam)"
    )
    parser.add_argument(
        "--clean",
        action="store_true",
        help="Clean BSP build before building"
    )

    args = parser.parse_args()
    module_path = os.path.abspath(args.device)

    if not os.path.isdir(module_path):
        print(f"Not a directory: {module_path}")
        sys.exit(1)

    if args.clean:
        clean_bsp(module_path)
        sys.exit(1)

    copy_bsp(module_path)
    run_make(module_path)

if __name__ == "__main__":
    main()
