set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

if(NOT DEFINED CMAKE_C_COMPILER)
    find_program(VE301_AARCH64_GCC aarch64-linux-gnu-gcc)
    if(NOT VE301_AARCH64_GCC)
        message(FATAL_ERROR
            "aarch64-linux-gnu-gcc was not found. On Ubuntu install gcc-aarch64-linux-gnu and g++-aarch64-linux-gnu.")
    endif()
    set(CMAKE_C_COMPILER "${VE301_AARCH64_GCC}" CACHE FILEPATH "C compiler")
endif()

if(NOT DEFINED CMAKE_CXX_COMPILER)
    find_program(VE301_AARCH64_GXX aarch64-linux-gnu-g++)
    if(NOT VE301_AARCH64_GXX)
        message(FATAL_ERROR
            "aarch64-linux-gnu-g++ was not found. On Ubuntu install gcc-aarch64-linux-gnu and g++-aarch64-linux-gnu.")
    endif()
    set(CMAKE_CXX_COMPILER "${VE301_AARCH64_GXX}" CACHE FILEPATH "C++ compiler")
endif()

set(VE301_RASPBERRY ON CACHE BOOL "Enable Raspberry Pi specific sources and flags" FORCE)
