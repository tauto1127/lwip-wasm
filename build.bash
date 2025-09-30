#!/bin/bash

bash Third_Party/download.bash

mkdir build

cd build

# Build using wasi-libc threads (no WAMR sysroot hacks)

cmake -DWASI_SDK_PREFIX=/opt/wasi-sdk-21 -DCMAKE_TOOLCHAIN_FILE=/opt/wasi-sdk-21/share/cmake/wasi-sdk.cmake -DCMAKE_SYSROOT=/opt/wasi-sdk-21/share/wasi-sysroot .. 

make

make install
