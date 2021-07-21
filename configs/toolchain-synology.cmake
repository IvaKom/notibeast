# compile boost:
# ./b2 --with-log --with-program_options --address-model=32 --stagedir=$HOME/projects/boost-syn -j4  --toolset=gcc-arm -threading=multi --reconfigure --user-config=./user-config.jam -static

# invoke cmake for synology build:
# BOOST_ROOT=$HOME/projects/boost-syn cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=../configs/toolchain-synology.cmake -DTESTS=OFF ..

message(STATUS "Building for Synology Diskstation")

set(Boost_USE_STATIC_LIBS ON)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# sudo apt install g++-9-arm-linux-gnueabihf
set(CMAKE_CXX_COMPILER "arm-linux-gnueabihf-g++-9")
set(CMAKE_C_COMPILER "arm-linux-gnueabihf-gcc-9")

# That is about what Synology uses for Marvell Armada-370
set(ARM_TUNE
    -mfloat-abi=hard
    -mhard-float
    -mfpu=vfpv3
    -mcpu=marvell-pj4
    -mtune=marvell-pj4
)

# /lib/libstdc++.so.6 is not there on the station, linking statically
# add_compile_options must come before add_executable
add_compile_options(${ARM_TUNE} -Wno-psabi)
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -static")

#argh... see https://stackoverflow.com/a/58589570
unset(CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES)
unset(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES)

