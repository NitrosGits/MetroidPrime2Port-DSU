# Coverage-only: retain callable loader-data constructor/destructor fixtures.
function(spw1_keep_inline)
    if(NOT MP_PORT_COVERAGE)
        message(FATAL_ERROR "spw1 inline retention requires MP_PORT_COVERAGE")
    endif()
    set_property(SOURCE
        "${CMAKE_SOURCE_DIR}/src/MetroidPrime/Enemies/CSpankWeed.cpp"
        APPEND PROPERTY COMPILE_OPTIONS -fkeep-inline-functions)
endfunction()
cmake_language(DEFER CALL spw1_keep_inline)
