import os, platform

def is_active():
    return True

def get_name():
    return "Nintendo Wii"

def can_build():
    if os.getenv("DEVKITPRO"):
        return True
    return False

def get_flags():
    return [
        ("tools", False),
        ("module_bullet_enabled", False),
        ("module_mbedtls_enabled", False),
        ("module_gdnative_enabled", False),
        ("module_regex_enabled", False),
        ("module_upnp_enabled", False),
        ("module_webm_enabled", False),
        ("module_websocket_enabled", False),
        ("module_enet_enabled", False)
    ]

def get_opts():
    from SCons.Variables import PathVariable
    return [PathVariable("dkp_path", "The path to your DevKitPro installation. Required for building on Windows.", "", PathVariable.PathAccept)]

def create(env):
    return env.Clone(tools=["mingw"])

def configure(env):
    # Workaround for MinGW. See:
    # http://www.scons.org/wiki/LongCmdLinesOnWin32
    if os.name == "nt":

        import subprocess

        def mySubProcess(cmdline, env):
            # print("SPAWNED : " + cmdline)
            startupinfo = subprocess.STARTUPINFO()
            startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
            proc = subprocess.Popen(
                cmdline,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                startupinfo=startupinfo,
                shell=False,
                env=env,
            )
            data, err = proc.communicate()
            rv = proc.wait()
            if rv:
                print("=====")
                print(err.decode("utf-8"))
                print("=====")
            return rv

        def mySpawn(sh, escape, cmd, args, env):

            newargs = " ".join(args[1:])
            cmdline = cmd + " " + newargs

            rv = 0
            if len(cmdline) > 32000 and cmd.endswith("ar"):
                cmdline = cmd + " " + args[1] + " " + args[2] + " "
                for i in range(3, len(args)):
                    rv = mySubProcess(cmdline + args[i], env)
                    if rv:
                        break
            else:
                rv = mySubProcess(cmdline, env)

            return rv

        env["SPAWN"] = mySpawn

    # Set compilers
    dkp_path = env.get("dkp_path")
    if not dkp_path:
        if platform.system() != "Windows":
            dkp_path = os.getenv("DEVKITPRO")
        else:
            print("ERR: Please define 'dkp_path' to point to your DevKitPro folder.")
            exit(1)
    ppc_path = dkp_path + "/devkitPPC"
    tools_path = ppc_path + "/bin/"
    tool_prefix = "powerpc-eabi-"

    env["CC"] = tools_path + tool_prefix + "gcc"
    env["CXX"] = tools_path + tool_prefix + "g++"
    env["LD"] = tools_path + tool_prefix + "ld"
    env["AR"] = tools_path + tool_prefix + "ar"
    env["RANLIB"] = tools_path + tool_prefix + "ranlib"

    env.Append(
    CPPPATH=[
        dkp_path + "/libogc/include",
        ppc_path + "/powerpc-eabi/include"
    ],
    LIBPATH=[
        dkp_path + "/libogc/lib/wii"
    ],
    CCFLAGS=[
        "-mrvl", "-mcpu=750", "-meabi", "-mhard-float", "-O0"
    ],
    LINKFLAGS=[
        "-mrvl", "-mcpu=750", "-meabi", "-mhard-float"
    ],
    CPPDEFINES=[
        "GEKKO", "WII", "NO_THREADS"
    ],
    LIBS=[
        "wiiuse", "bte", "fat", "ogc", "m"
    ])

    env.Prepend(CPPPATH=["#platform/wii"])
    #TODO: Debug flags
