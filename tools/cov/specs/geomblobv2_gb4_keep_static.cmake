# Coverage-only: retain callable copies of the two otherwise-inlined gathering
# helpers so gdb can supply isolated geometry fixtures. Their ordinary callers
# and semantics are unchanged. Use only with build-port-cov:
# -DCMAKE_PROJECT_metroid_prime2_port_INCLUDE=$PWD/tools/cov/specs/geomblobv2_gb4_keep_static.cmake
function(gb4_keep_static)
    if(NOT MP_PORT_COVERAGE)
        message(FATAL_ERROR "gb4 helper retention requires MP_PORT_COVERAGE")
    endif()
    set_property(SOURCE
        "${CMAKE_SOURCE_DIR}/src/MetroidPrime/Enemies/CSurfaceParticleEffect.cpp"
        APPEND PROPERTY COMPILE_OPTIONS -fkeep-static-functions)
endfunction()
cmake_language(DEFER CALL gb4_keep_static)
