
add_library(Core INTERFACE)
target_include_directories(Core INTERFACE Inc)
target_link_libraries(Core INTERFACE PY32F071_Driver)
target_compile_definitions(Core INTERFACE 
    PY32F071x8 
    USE_FULL_LL_DRIVER
)

target_sources(Core INTERFACE
    Src/main.c
    Src/py32f071_it.c
    Src/system_py32f071.c
    Src/syscalls.c
    startup_py32f071xx.s
)

target_link_options(Core INTERFACE "-T" "${CMAKE_CURRENT_SOURCE_DIR}/py32f071xb.ld")
