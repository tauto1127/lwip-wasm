#!/bin/bash

bash Third_Party/download.bash

mkdir build

cd build

# https://github.com/bytecodealliance/wasm-micro-runtime/blob/WAMR-1.1.1/doc/pthread_library.md
/opt/wasi-sdk/bin/llvm-ar -d /opt/wasi-sdk/share/wasi-sysroot/lib/wasm32-wasi/libc.a dlmalloc.o
cp ${WAMR_ROOT}/wamr-sdk/app/libc-builtin-sysroot/include/pthread.h /opt/wasi-sdk/share/wasi-sysroot/include

cmake -DWASI_SDK_PREFIX=/opt/wasi-sdk -DCMAKE_TOOLCHAIN_FILE=/opt/wasi-sdk/share/cmake/wasi-sdk.cmake -DCMAKE_SYSROOT=/opt/wasi-sdk/share/wasi-sysroot .. 

make

make install