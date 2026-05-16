import os
import shutil
Import("env")

lib_src = os.path.abspath(os.path.join(env["PROJECT_DIR"], "../../.."))
lib_name = os.path.basename(lib_src)
libdeps_dir = os.path.join(env["PROJECT_DIR"], ".pio", "libdeps", env["PIOENV"])
lib_dst = os.path.join(libdeps_dir, lib_name)

os.makedirs(libdeps_dir, exist_ok=True)

if os.path.islink(lib_dst):
    if os.path.realpath(lib_dst) != os.path.realpath(lib_src):
        os.unlink(lib_dst)
        os.symlink(lib_src, lib_dst)
elif os.path.exists(lib_dst):
    shutil.rmtree(lib_dst)
    os.symlink(lib_src, lib_dst)
else:
    os.symlink(lib_src, lib_dst)
