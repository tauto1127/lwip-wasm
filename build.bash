#!/bin/bash

bash Third_Party/download.bash

mkdir -p build

cd build

# WAMR pthread library: https://github.com/bytecodealliance/wasm-micro-runtime/blob/WAMR-1.1.1/doc/pthread_library.md
# Copy wasi-sysroot to local home directory if not already done, and modify the local libc.a instead of global
LOCAL_SYSROOT="${HOME}/wasi-sysroot"
if [ ! -d "${LOCAL_SYSROOT}" ]; then
  echo "Creating local copy of wasi-sysroot at ${LOCAL_SYSROOT}..."
  cp -r /opt/wasi-sdk-21/share/wasi-sysroot "${LOCAL_SYSROOT}"
fi

/opt/wasi-sdk-21/bin/llvm-ar -d "${LOCAL_SYSROOT}/lib/wasm32-wasi/libc.a" dlmalloc.o

cmake -DWASI_SDK_PREFIX=/opt/wasi-sdk-21 -DCMAKE_TOOLCHAIN_FILE=/opt/wasi-sdk-21/share/cmake/wasi-sdk-pthread.cmake -DCMAKE_SYSROOT="${LOCAL_SYSROOT}" .. 

make

make install
