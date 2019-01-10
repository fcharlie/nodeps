#

add_executable(nodeps
    main.cc
    pe.cc
)

target_link_libraries(nodeps
    Pathcch
    Kernel32
    Advapi32
)