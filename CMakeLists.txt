cmake_minimum_required(VERSION 3.25)
project(metroid_prime2_port LANGUAGES C CXX)

# Native-PC port scaffold for Metroid Prime 2: Echoes (`G2ME01`), a fork of the
# PrimeDecomp/echoes matching decompilation, built the same way as the Metroid
# Prime port: the decompiled game and engine run directly on the GPU through
# Aurora (MIT) instead of an emulator or a static recompilation.
#
# The decompilation is still early (see PORT_NOTES.md), so today this builds the
# game sources against Aurora's SDK headers, plus the port's REL module runtime
# and its tests. `MP_SDK_HEADERS_ONLY` is the only verified configuration.
#
# Reference implementation for every piece of the port layer:
# ../MetroidPrimePort (same engine lineage, complete and working).

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)
include(CTest)

option(MP_SDK_HEADERS_ONLY
    "Compile the decompiled game sources against Aurora's SDK headers without \
linking Aurora. This is the only configuration that works while the Echoes \
decompilation is incomplete, and it is what the shim queue is tracked against."
    ON)

# The REL module tests need modules extracted from a disc the user owns (see
# tools/extract_disc_file.py) plus the symbol list of the executable they were
# linked against. Both are optional: without them the tests report themselves
# skipped. Point MP_REL_FIXTURE at a directory of .rel files to validate the
# whole module set in dependency order.
set(MP_REL_FIXTURE "" CACHE PATH
    "REL module file, or directory of .rel files, extracted from an owned disc")
set(MP_DOL_SYMBOLS "${CMAKE_CURRENT_SOURCE_DIR}/config/G2ME01/symbols.txt" CACHE FILEPATH
    "DOL symbol list matching MP_REL_FIXTURE")

# The decompiled sources are written for a *signed* plain `char`, as PowerPC/MWCC
# and every host the Metroid Prime port has been verified on agree on that. ARM
# defaults `char` to unsigned, which silently changes sentinels such as
# CDSPStreamManager's -1 companion index into 255. Force the verified semantics
# everywhere that compiles game-facing code. MSVC/clang-cl already default to
# signed char.
function(mp_signed_char target)
    if(NOT MSVC)
        target_compile_options(${target} PRIVATE -fsigned-char)
    endif()
endfunction()

# Decompiled game + engine sources. The file list is explicit: every translation
# unit that links must be listed, including registration-only ones.
include(files.cmake)

# Aurora provides the GameCube SDK compatibility layer (GX, OS, PAD, DVD, CARD,
# VI, SI, THP, mtx) plus the SDL3/WebGPU application layer. Only its headers are
# used until the decompilation can link.
add_library(mp_game OBJECT ${MP_GAME_SOURCES})
# The same version macros configure.py hands mwcceppc (upstream moved from -DVERSION_G2ME01 to
# -DVERSION=<n> against include/GameVersions.h). Without them `#if VERSION != VERSION_G2ME01`
# only reads right because an undefined macro is 0 in #if.
target_compile_definitions(mp_game PUBLIC BUILD_VERSION=0 VERSION=0)
# The host musyx-port target defines MUSY_TARGET_PC; headers that reach <musyx/musyx.h>
# (CAudioSys.hpp, CAuxEffect.hpp) must agree, or its Dolphin typedefs clash with Aurora's.
target_compile_definitions(mp_game PUBLIC MUSY_TARGET=MUSY_TARGET_PC)

# DIAGNOSTIC ONLY, and off by default. Adds src/MetroidPrime/PortReachStubs.cpp, which
# defines the undefined symbols that the boot path *does* reach, so a diagnostic build can
# run far enough to print which ones it asks for and in what order. Only
# tools/boot_probe.sh passes -DMP_BOOT_STUBS=ON.
#
# **It must never be on for a real build, and the reason is not caution, it is arithmetic:**
# with these stubs the link *succeeds*, so `link_check.sh` would report 0 undefined and be
# wrong by 318. `link_check.sh` never passes this option, which is the only thing keeping the
# honest number honest - so if anything ever adds it there, the gate becomes a liar with no
# symptom. `tools/check_boot_stubs.py` fails if the option is on while the target is the
# real executable.
option(MP_BOOT_STUBS
    "Diagnostic only: stub the REACHABLE undefined symbols so the boot path's order becomes \
observable. NEVER for a real build - it makes the link succeed and lie."
    OFF)
if(MP_BOOT_STUBS)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/MetroidPrime/PortReachStubs.cpp")
        message(FATAL_ERROR
            "MP_BOOT_STUBS=ON but src/MetroidPrime/PortReachStubs.cpp is missing. "
            "Generate it with: python3 tools/gen_link_stubs.py --reachable")
    endif()
    message(WARNING
        "MP_BOOT_STUBS=ON: the REACHABLE undefined symbols are being stubbed. This build "
        "is a diagnostic. It is not the port, it must not be run as one, and the undefined "
        "count it reports is 0 by construction.")
    target_sources(mp_game PRIVATE src/MetroidPrime/PortReachStubs.cpp)
endif()

mp_signed_char(mp_game)
# Include order is deliberate: `include/` (the decompilation's own headers) is
# searched *after* Aurora, because Echoes' `include/dolphin/` tree is the partial
# SDK decompilation and must not shadow Aurora's complete, PC-aware SDK view.
# `platform/include/dolphin` fills the SDK headers Aurora omits. `include/LZO`
# is on the path project-wide because the bundled codec is included both as
# `<lzo1x.h>` (upstream's own build does the same).
target_include_directories(mp_game PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/include
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/aurora/include
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/musyx-port/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/LZO
    # config.yml `extract:` output (DefaultFontData.inc etc.), written by the upstream build.
    ${CMAKE_CURRENT_SOURCE_DIR}/build/G2ME01/include)
# compat.h restores the SDK macros Aurora omits; it is force-included ahead of
# every C++ game translation unit. It is C++-only (it pulls in the game's
# CMemory/IAllocator headers), so it must not reach the few C translation units
# in the list - the bundled LZO codec - which are compiled as C, exactly as the
# matching build does.
if(MSVC)
    target_compile_options(mp_game PRIVATE /W3
        "$<$<COMPILE_LANGUAGE:CXX>:/FI${CMAKE_CURRENT_SOURCE_DIR}/platform/compat.h>")
else()
    # -Werror=return-type is what the Metroid Prime port uses, but the Echoes
    # decompilation is only ~7% done: several not-yet-finished functions fall off
    # the end of a non-void function, exactly as MWCC accepted (undefined
    # behaviour). Inventing return values would be decompilation work, not port
    # work, so the diagnostic stays a warning until upstream completes them; see
    # PORT_NOTES.md. Tighten this back to -Werror=return-type when the
    # decompilation is complete.
    target_compile_options(mp_game PRIVATE
        -Wformat -Wno-error=return-type -Wno-narrowing -Wno-multichar -Wno-write-strings
        # The game's allocator API carries the literal "??(??)" call-stack tags
        # from the PowerPC build; they are trigraphs to the host compiler and
        # are intentional, so keep them out of the log.
        -Wno-trigraphs
        "$<$<COMPILE_LANGUAGE:CXX>:-include${CMAKE_CURRENT_SOURCE_DIR}/platform/compat.h>")
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)
# REL modules. On the console each REL is its own symbol namespace, so their RELMain/RELExit and
# module-local Load* functions collide in a flat host link. Each module is compiled with the game's
# flags as its own object library, then tools/scope_module.py partially links it and makes every
# symbol local except the ones the rest of the program needs (see PORT_NOTES.md).
set(MP_MODULE_OBJECT_TARGETS)
foreach(m ${MP_MODULES})
    add_library(mp_mod_${m} OBJECT ${MP_MODULE_${m}_SOURCES})
    target_compile_definitions(mp_mod_${m} PUBLIC BUILD_VERSION=0 VERSION=0 MUSY_TARGET=MUSY_TARGET_PC)
    mp_signed_char(mp_mod_${m})
    target_include_directories(mp_mod_${m} PUBLIC
        $<TARGET_PROPERTY:mp_game,INTERFACE_INCLUDE_DIRECTORIES>)
    if(NOT MSVC)
        target_compile_options(mp_mod_${m} PRIVATE
            -Wformat -Wno-error=return-type -Wno-narrowing -Wno-multichar -Wno-write-strings
            -Wno-trigraphs
            "$<$<COMPILE_LANGUAGE:CXX>:-include${CMAKE_CURRENT_SOURCE_DIR}/platform/compat.h>")
    endif()
    list(APPEND MP_MODULE_OBJECT_TARGETS mp_mod_${m})
endforeach()
# Compile the modules whenever mp_game is built, so a broken module source is caught by the
# header-only build too.
add_dependencies(mp_game ${MP_MODULE_OBJECT_TARGETS})

find_program(MP_LD_BFD NAMES ld.bfd ld REQUIRED)
find_program(MP_OBJCOPY NAMES ${CMAKE_OBJCOPY} objcopy REQUIRED)
find_program(MP_NM NAMES ${CMAKE_NM} nm REQUIRED)
set(MP_MODULE_DIR "${CMAKE_BINARY_DIR}/modules")
set(MP_MODULE_REGISTRY "${CMAKE_BINARY_DIR}/generated/compiled_modules.inc")

# modules.txt: one "Name;obj;obj" line per module.
set(_mp_modlist "")
foreach(m ${MP_MODULES})
    string(APPEND _mp_modlist "${m};$<JOIN:$<TARGET_OBJECTS:mp_mod_${m}>,;>\n")
endforeach()
file(GENERATE OUTPUT "${MP_MODULE_DIR}/modules.txt" CONTENT "${_mp_modlist}")

# The registry only needs the raw module objects (it names who defines RELMain/RELExit), so it
# can be an input of platform/compiled_modules.cpp without a cycle through the scoping step.
set(_mp_mod_objs)
foreach(m ${MP_MODULES})
    list(APPEND _mp_mod_objs $<TARGET_OBJECTS:mp_mod_${m}>)
endforeach()
add_custom_command(OUTPUT "${MP_MODULE_REGISTRY}"
    COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tools/scope_module.py" registry
            --modules "${MP_MODULE_DIR}/modules.txt" --out "${MP_MODULE_REGISTRY}" --nm "${MP_NM}"
    DEPENDS ${_mp_mod_objs} "${CMAKE_CURRENT_SOURCE_DIR}/tools/scope_module.py"
    COMMENT "Generating compiled-module registry" VERBATIM)
add_custom_target(mp_module_registry DEPENDS "${MP_MODULE_REGISTRY}")

# Called once the other object libraries exist. Produces build/modules/<Name>.o for every module
# and returns the list in MP_SCOPED_MODULE_OBJECTS.
function(mp_scope_modules)
    set(others)
    foreach(t ${ARGN})
        list(APPEND others $<TARGET_OBJECTS:${t}>)
    endforeach()
    set(content "")
    foreach(t ${ARGN})
        string(APPEND content "$<JOIN:$<TARGET_OBJECTS:${t}>,\n>\n")
    endforeach()
    file(GENERATE OUTPUT "${MP_MODULE_DIR}/others.txt" CONTENT "${content}")
    set(outs)
    foreach(m ${MP_MODULES})
        list(APPEND outs "${MP_MODULE_DIR}/${m}.o")
    endforeach()
    add_custom_command(OUTPUT ${outs}
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tools/scope_module.py" scope
                --modules "${MP_MODULE_DIR}/modules.txt" --others "${MP_MODULE_DIR}/others.txt"
                --outdir "${MP_MODULE_DIR}" --ld "${MP_LD_BFD}" --objcopy "${MP_OBJCOPY}" --nm "${MP_NM}"
        DEPENDS ${_mp_mod_objs} ${others} "${CMAKE_CURRENT_SOURCE_DIR}/tools/scope_module.py"
        COMMENT "Scoping REL modules" VERBATIM)
    add_custom_target(mp_modules DEPENDS ${outs})
    set(MP_SCOPED_MODULE_OBJECTS ${outs} PARENT_SCOPE)
endfunction()

# The port's own engine-independent sources: SDK shims and stubs, and the
# glibc floor. They compile against Aurora's headers alone, which is what
# keeps them verified while the game cannot link. The engine-facing platform
# files (entry point, debug UI, textures, prompts, randomizer, smoke, disc
# resources) need Echoes' game types and are still Metroid-Prime-shaped; see
# platform/README.md.
add_library(mp_platform OBJECT
    platform/entry.cpp
    platform/compiled_modules.cpp
    platform/rel.cpp
    platform/shims.cpp
    platform/sdk_stubs.cpp
    platform/port_reset.cpp
    platform/glibc_compat.c)
set_source_files_properties(platform/compiled_modules.cpp PROPERTIES
    OBJECT_DEPENDS "${MP_MODULE_REGISTRY}")
target_include_directories(mp_platform PRIVATE
    "${CMAKE_BINARY_DIR}/generated"
    platform/include
    extern/aurora/include
    extern/musyx-port/include
    include)
target_compile_definitions(mp_platform PRIVATE TARGET_PC AURORA MUSY_TARGET=MUSY_TARGET_PC
    MUSY_VERSION_MAJOR=2 MUSY_VERSION_MINOR=0 MUSY_VERSION_PATCH=3)

# The process entry point. It is compiled but not linked: it builds the OS
# context and memory system the game's entry takes, and those constructors
# are still upstream work (see PORT_NOTES.md).
add_library(mp_port_entry OBJECT platform/main.cpp)
target_include_directories(mp_port_entry PRIVATE
    platform/include
    extern/aurora/include
    extern/musyx-port/include
    include)
target_compile_definitions(mp_port_entry PRIVATE TARGET_PC AURORA MUSY_TARGET=MUSY_TARGET_PC
    MUSY_VERSION_MAJOR=2 MUSY_VERSION_MINOR=0 MUSY_VERSION_PATCH=3)

if(MP_SDK_HEADERS_ONLY)
    # Aurora sets these on its own targets; the header-only build has to state them.
    target_compile_definitions(mp_game PUBLIC TARGET_PC AURORA)
    foreach(t ${MP_MODULE_OBJECT_TARGETS})
        target_compile_definitions(${t} PUBLIC TARGET_PC AURORA)
    endforeach()
    target_compile_definitions(mp_platform PRIVATE TARGET_PC AURORA MUSY_TARGET=MUSY_TARGET_PC
    MUSY_VERSION_MAJOR=2 MUSY_VERSION_MINOR=0 MUSY_VERSION_PATCH=3)
    target_compile_definitions(mp_port_entry PRIVATE TARGET_PC AURORA MUSY_TARGET=MUSY_TARGET_PC
    MUSY_VERSION_MAJOR=2 MUSY_VERSION_MINOR=0 MUSY_VERSION_PATCH=3)

    mp_scope_modules(mp_game mp_platform mp_port_entry)

    # REL module runtime tests. The runtime is port code (see PORT_NOTES.md), so
    # these need neither Aurora's libraries nor a GPU device.
    set(MP_REL_TEST_SOURCES platform/rel.cpp)

    add_executable(port_rel_tests tests/port_rel_synthetic.cpp ${MP_REL_TEST_SOURCES})
    target_include_directories(port_rel_tests PRIVATE platform/include)
    add_test(NAME port_rel_tests COMMAND port_rel_tests)
    set_tests_properties(port_rel_tests PROPERTIES TIMEOUT 30 LABELS port)

    add_executable(port_rel_real_tests tests/port_rel_real.cpp ${MP_REL_TEST_SOURCES})
    target_include_directories(port_rel_real_tests PRIVATE platform/include)
    if(MP_REL_FIXTURE AND IS_DIRECTORY "${MP_REL_FIXTURE}")
        add_test(NAME port_rel_real_tests
            COMMAND port_rel_real_tests --dir "${MP_REL_FIXTURE}" "${MP_DOL_SYMBOLS}")
    else()
        add_test(NAME port_rel_real_tests
            COMMAND port_rel_real_tests "${MP_REL_FIXTURE}" "${MP_DOL_SYMBOLS}")
    endif()
    set_tests_properties(port_rel_real_tests PROPERTIES
        TIMEOUT 30 LABELS port SKIP_RETURN_CODE 77)

    add_executable(port_entry_tests tests/port_entry_tests.cpp platform/entry.cpp)
    target_include_directories(port_entry_tests PRIVATE platform/include extern/aurora/include)
    target_compile_definitions(port_entry_tests PRIVATE TARGET_PC)
    add_test(NAME port_entry_tests COMMAND port_entry_tests)
    set_tests_properties(port_entry_tests PROPERTIES TIMEOUT 30 LABELS port)

    message(STATUS
        "MP_SDK_HEADERS_ONLY=ON: building the game sources against Aurora's headers "
        "only (no Aurora, no executable). Set -DMP_SDK_HEADERS_ONLY=OFF once the "
        "decompilation is complete enough to link. See PORT_NOTES.md.")
    return()
endif()

add_subdirectory(extern/aurora EXCLUDE_FROM_ALL)
if(TARGET aurora_mtx)
    target_compile_definitions(aurora_mtx PRIVATE MTX_USE_PS=1)
endif()

# platform/ai_dma.cpp is the GameCube AI DMA path on the host, and it is the port's
# implementation of all five AI DMA entry points (Aurora's dolphin/ai.h declares
# them and implements none). It is a separate target rather than part of mp_platform
# because it is the one port source that is not engine-independent: it includes
# SDL3, which only exists once Aurora has been added below and fetched it. That
# also keeps tools/probe_sources.sh honest - it covers mp_platform and
# mp_port_entry, which compile against Aurora's headers alone, and this file is
# syntax-checked by the full build instead.
if(AURORA_SDL3_TARGET)
    add_library(mp_port_audio OBJECT platform/ai_dma.cpp platform/port_events.cpp
        platform/port_mouse.cpp platform/port_warp.cpp platform/port_overlay.cpp)
    target_include_directories(mp_port_audio PRIVATE
        platform/include
        extern/aurora/include
        extern/musyx-port/include
        include
        $<TARGET_PROPERTY:${AURORA_SDL3_TARGET},INTERFACE_INCLUDE_DIRECTORIES>)
    target_compile_definitions(mp_port_audio PRIVATE TARGET_PC AURORA)
    mp_signed_char(mp_port_audio)
    # aurora::core carries ImGui's include directory, which the F1 overlay builds against.
    target_link_libraries(mp_port_audio PRIVATE ${AURORA_SDL3_TARGET} aurora::core)
else()
    message(FATAL_ERROR
        "platform/ai_dma.cpp needs SDL3, but Aurora provided no AURORA_SDL3_TARGET. "
        "The game links AIInitDMA/AIStartDMA/AIGetDMAStartAddr/AIRegisterDMACallback "
        "from it, so without SDL the port cannot link.")
endif()

# MusyX is the game's audio engine; the vendored fork is portable. Echoes uses 2.0.3 (as in
# configure.py): its group-stack API (sndStack*) and per-stack ARAM only exist from 2.0.2.
set(MUSYX_VERSION_PATCH 3 CACHE STRING "" FORCE)
add_subdirectory(extern/musyx-port EXCLUDE_FROM_ALL)
mp_signed_char(musyx)

target_link_libraries(mp_game PUBLIC
    aurora::core aurora::gx aurora::gd aurora::si aurora::vi aurora::pad
    aurora::mtx aurora::os aurora::dvd aurora::thp aurora::card
    musyx)
foreach(t ${MP_MODULE_OBJECT_TARGETS})
    target_link_libraries(${t} PRIVATE
        aurora::core aurora::gx aurora::gd aurora::si aurora::vi aurora::pad
        aurora::mtx aurora::os aurora::dvd aurora::thp aurora::card musyx)
endforeach()

# ---------------------------------------------------------------------------
# The executable. `mp_port_entry` is platform/main.cpp, which is the process entry:
# it brings Aurora up (which is what creates the window), mounts the disc, and calls
# InvokeCMain - see PORT_NOTES.md. So the Echoes binary is those three object
# libraries and Aurora, in the same shape ../MetroidPrimePort uses.
# ---------------------------------------------------------------------------
mp_scope_modules(mp_game mp_platform mp_port_entry mp_port_audio)
add_executable(metroid_prime2_port)
set_source_files_properties(${MP_SCOPED_MODULE_OBJECTS} PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)
target_sources(metroid_prime2_port PRIVATE
    ${MP_SCOPED_MODULE_OBJECTS}
    $<TARGET_OBJECTS:mp_port_entry>
    $<TARGET_OBJECTS:mp_platform>
    $<TARGET_OBJECTS:mp_port_audio>)
target_include_directories(metroid_prime2_port PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_definitions(metroid_prime2_port PRIVATE TARGET_PC AURORA)
mp_signed_char(metroid_prime2_port)

# aurora_core (webgpu/gpu.cpp) and aurora_gx reference each other; group them so the
# linker rescans and resolves the cycle. Same construct, same reason, as the Prime 1 port.
if(MSVC)
    target_link_libraries(metroid_prime2_port PRIVATE aurora::main aurora::core aurora::gx)
    target_link_options(metroid_prime2_port PRIVATE /INCREMENTAL:NO)
elseif(CMAKE_LINK_GROUP_USING_RESCAN_SUPPORTED OR CMAKE_CXX_LINK_GROUP_USING_RESCAN_SUPPORTED)
    target_link_libraries(metroid_prime2_port PRIVATE
        mp_game
        aurora::main
        "$<LINK_GROUP:RESCAN,aurora::core,aurora::gx>")
    # `platform/main.cpp` installs a SIGSEGV/SIGBUS/SIGILL/SIGFPE handler that calls
    # `backtrace_symbols_fd`, and **without -rdynamic the frames come back as bare
    # addresses** - which is the one form of this output that is not worth having, because
    # the whole point is naming the function that faulted. This is the only place it is
    # needed: the decomp build links no platform code and the gate's port build inherits
    # it from here.
    target_link_options(metroid_prime2_port PRIVATE -rdynamic)
else()
    message(FATAL_ERROR
        "This port needs an ELF linker with RESCAN support or an MSVC-compatible COFF linker")
endif()
