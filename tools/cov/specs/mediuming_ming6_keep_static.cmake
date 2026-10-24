# Coverage-only: expose the module-local jump-route and audio helpers to gdb.
# Ordinary port/matching builds keep their compiler options unchanged.
function(ming6_keep_static)
    if(NOT MP_PORT_COVERAGE)
        message(FATAL_ERROR "ming6 helper retention requires MP_PORT_COVERAGE")
    endif()
    set_property(SOURCE
        "${CMAKE_SOURCE_DIR}/src/MetroidPrime/Enemies/CMediumIng.cpp"
        "${CMAKE_SOURCE_DIR}/src/MetroidPrime/Enemies/PortCMediumIng.cpp"
        APPEND PROPERTY COMPILE_OPTIONS -fkeep-static-functions)
endfunction()
cmake_language(DEFER CALL ming6_keep_static)
