#!/bin/bash

bash Third_Party/download.bash

mkdir build

cd build

# https://github.com/bytecodealliance/wasm-micro-runtime/blob/WAMR-1.1.1/doc/pthread_library.md
/opt/wasi-sdk-21/bin/llvm-ar -d /opt/wasi-sdk-21/share/wasi-sysroot/lib/wasm32-wasi/libc.a dlmalloc.o

cmake -DWASI_SDK_PREFIX=/opt/wasi-sdk-21 -DCMAKE_TOOLCHAIN_FILE=/opt/wasi-sdk-21/share/cmake/wasi-sdk-pthread.cmake -DCMAKE_SYSROOT=/opt/wasi-sdk-21/share/wasi-sysroot .. 

make

make install
