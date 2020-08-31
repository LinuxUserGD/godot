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
        ("module_arkit_enabled", False),
        ("module_assimp_enabled", False),
        ("module_bullet_enabled", False),
        ("module_camera_enabled", False),
        ("module_csg_enabled", False),
        ("module_cvtt_enabled", False),
        ("module_dds_enabled", False),
        ("module_enet_enabled", False),
        ("module_etc_enabled", False),
        #("module_freetype_enabled", False),
        ("module_gdnative_enabled", False),
        #("module_gdscript_enabled", False),
        ("module_gridmap_enabled", False),
        ("module_hdr_enabled", False),
        ("module_jpg_enabled", False),
        ("module_jsonrpc_enabled", False),
        ("module_mbedtls_enabled", False),
        ("module_mobile_vr_enabled", False),
        ("module_ogg_enabled", False),
        ("module_opensimplex_enabled", False),
        ("module_opus_enabled", False),
        ("module_pvr_enabled", False),
        ("module_recast_enabled", False),
        #("module_regex_enabled", False),
        ("module_squish_enabled", False),
        ("module_stb_vorbis_enabled", False),
        ("module_svg_enabled", False),
        ("module_tga_enabled", False),
        ("module_theora_enabled", False),
        ("module_tinyexr_enabled", False),
        ("module_upnp_enabled", False),
        ("module_vhacd_enabled", False),
        ("module_visual_script_enabled", False),
        ("module_vorbis_enabled", False),
        ("module_webm_enabled", False),
        ("module_webp_enabled", False),
        ("module_webrtc_enabled", False),
        ("module_websocket_enabled", False),
        ("module_xatlas_unwrap_enabled", False),

        ("builtin_bullet", False),
        ("builtin_certs", False),
        ("builtin_enet", False),
        #("builtin_freetype", False),
        ("builtin_libogg", False),
        #("builtin_libpng", False),
        ("builtin_libtheora", False),
        ("builtin_libvorbis", False),
        ("builtin_libvpx", False),
        ("builtin_libwebp", False),
        ("builtin_wslay", False),
        ("builtin_mbedtls", False),
        ("builtin_miniupnpc", False),
        ("builtin_opus", False),
        #("builtin_pcre2", False),
        ("builtin_pcre2_with_jit", False),
        ("builtin_recast", False),
        ("builtin_squish", False),
        ("builtin_xatlas", False),
        #("builtin_zlib", False),
        #("builtin_zstd", False),

        ("disable_3d", True),
        #("disable_advanced_gui", True),
        ("deprecated", False),
        ("optimize", "size"),

        #("gdscript", False),
        ("minizip", False)
    ]

def get_opts():
    from SCons.Variables import PathVariable, BoolVariable
    return [
        PathVariable("dkp_path", "The path to your DevKitPro installation. Required for building on Windows.", "", PathVariable.PathAccept),
        BoolVariable("use_portlib_freetype", "If true, uses devkitpro's portlib of FreeType instead of the built-in.", True),
        BoolVariable("use_portlib_libpng", "If true, uses devkitpro's portlib of libpng instead of the built-in.", True),
        BoolVariable("use_portlib_zlib", "If true, uses devkitpro's portlib of zlib instead of the built-in.", True)
    ]

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
            env["dkp_path"] = dkp_path
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
        "-mrvl", "-mcpu=750", "-meabi", "-mhard-float", "-ffunction-sections", "-fdata-sections", "-fno-rtti", "-fno-exceptions"
    ],
    LINKFLAGS=[
        "-mrvl", "-mcpu=750", "-meabi", "-mhard-float", "-T", "platform/wii/pck_embed.ld", "-Wl,--gc-sections"
    ],
    CPPDEFINES=[
        "GEKKO", "WII", "NO_THREADS", "NO_SAFE_CAST"
    ],
    LIBS=[
        "wiiuse", "bte", "fat", "ogc", "m"
    ])

    if env["use_portlib_freetype"] or env["use_portlib_libpng"]:
        env.Append(
            LIBPATH=[
                dkp_path + "/portlibs/ppc/lib"
            ]
        )

    if env["use_portlib_freetype"] and env["module_freetype_enabled"]:
        env["builtin_freetype"] = False
        env.Append(
            CPPPATH=[
                dkp_path + "/portlibs/ppc/include/freetype2"
            ],
            LIBS=["freetype", "bz2"]
        )
    if env["use_portlib_libpng"]:
        env["builtin_libpng"] = False
        env.Append(
            CPPPATH=[
                dkp_path + "/portlibs/ppc/include/libpng16"
            ],
            LIBS=["png"]
        )
    
    if env["use_portlib_zlib"]:
        env["builtin_zlib"] = False
        env.Append(
            CPPPATH=[
                dkp_path + "/portlibs/ppc/include"
            ],
            LIBS=["z"]
        )

    if env["optimize"] == "size":
        env.Append(CCFLAGS=["-Os"])
    elif env["target"] == "debug":
        env.Append(CCFLAGS=["-Og"])
    else:
        env.Append(CCFLAGS=["-O3"])

    if env["use_lto"]:
        env.Append(CCFLAGS=["-flto"])
        env.Append(LINKFLAGS=["-flto=" + str(env.GetOption("num_jobs"))])
        env["AR"] = tools_path + tool_prefix + "gcc-ar"
        env["RANLIB"] = tools_path + tool_prefix + "gcc-ranlib"
    if env["target"] == "debug":
        env.Append(CPPDEFINES=["DEBUG_ENABLED"])
        env.Append(CCFLAGS=["-g"])
        env.Prepend(LIBS=["db"])

    env.Prepend(CPPPATH=["#platform/wii"])
    #TODO: Debug flags
