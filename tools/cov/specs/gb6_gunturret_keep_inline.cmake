# Coverage-only: retain native inline SLdr constructors/destructors for direct
# heap-lifetime/default-value fixtures; ordinary host/matching builds unchanged.
function(gb6_keep_inline)
    if(NOT MP_PORT_COVERAGE)
        message(FATAL_ERROR "gb6 inline retention requires MP_PORT_COVERAGE")
    endif()
    set_property(SOURCE
        "${CMAKE_SOURCE_DIR}/src/MetroidPrime/Enemies/CGunTurretBase.cpp"
        "${CMAKE_SOURCE_DIR}/src/MetroidPrime/Enemies/CGunTurretTop.cpp"
        APPEND PROPERTY COMPILE_OPTIONS -fkeep-inline-functions)
endfunction()
cmake_language(DEFER CALL gb6_keep_inline)
