Import("env", "projenv")

lto_flags = ["-flto=auto", "-fuse-linker-plugin"]

for e in (env, projenv):
    for key in ("CCFLAGS", "CFLAGS", "CXXFLAGS", "LINKFLAGS"):
        if key in e:
            e[key] = [f for f in e[key] if str(f) != "-fno-lto"]

    ar = e.subst("$AR")
    ranlib = e.subst("$RANLIB")
    if ar.endswith("-ar") and "gcc-ar" not in ar:
        e.Replace(AR=ar[:-3] + "-gcc-ar")
    if ranlib.endswith("-ranlib") and "gcc-ranlib" not in ranlib:
        e.Replace(RANLIB=ranlib[:-7] + "-gcc-ranlib")

    e.Append(CCFLAGS=lto_flags, LINKFLAGS=lto_flags)

print("LTO AR:", env.subst("$AR"))
print("LTO LINKFLAGS:", [f for f in env["LINKFLAGS"] if "lto" in str(f) or "plugin" in str(f)])
