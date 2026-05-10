import shutil
import os

Import("env")

libdeps_dir = os.path.join(env["PROJECT_DIR"], ".pio", "libdeps")

if os.path.exists(libdeps_dir):
    shutil.rmtree(libdeps_dir)
    print("Cleaned libdeps")
    