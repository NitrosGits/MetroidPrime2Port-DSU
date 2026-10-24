# Coverage-only callable loader and inline getter fixtures; normal builds unchanged.
function(gui1_keep_inline)
    if(NOT MP_PORT_COVERAGE)
        message(FATAL_ERROR "gui1 inline retention requires MP_PORT_COVERAGE")
    endif()
    set_property(SOURCE
        "${CMAKE_SOURCE_DIR}/src/MetroidPrime/Enemies/CShrieker.cpp"
        APPEND PROPERTY COMPILE_OPTIONS -fkeep-inline-functions)
endfunction()
cmake_language(DEFER CALL gui1_keep_inline)
