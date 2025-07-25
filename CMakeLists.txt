set(LWIP_POSIX_CMAKE_VERSION 3.5.1)
cmake_minimum_required(VERSION ${LWIP_POSIX_CMAKE_VERSION})

project(lwip-wasm-project
    VERSION 1.0.0
    DESCRIPTION "lwip-posix"
    LANGUAGES C
)

include(default-cmake-options.cmake)

# ASSERTION CHECK 
if (NOT DEFINED LWIP_DIR)
	message(FATAL " LWIP_DIR is not defined")
endif()
if (NOT DEFINED LWIP_CORE_DIR)
	message(FATAL " LWIP_CORE_DIR is not defined")
endif()
if (NOT DEFINED cmsis_DIR)
	message(FATAL " cmsis_DIR is not defined")
endif()

include(${LWIP_DIR}/Filelists.cmake)
if (NOT DEFINED lwip_SRCS)
	message(FATAL " lwip_SRCS is not defined")
endif()
if (NOT DEFINED lwip_core_SRCS)
	message(FATAL " lwip_core_SRCS is not defined")
endif()

message(STATUS "LWIP_DIR " ${LWIP_DIR})
message(STATUS "LWIP_CORE_DIR " ${LWIP_CORE_DIR})

include(${PROJECT_SOURCE_DIR}/../../wasm-micro-runtime/core/iwasm/libraries/lib-socket/lib_socket_wasi.cmake)

add_library(
	lwip STATIC
	${lwip_SRCS}
	${lwip_core_SRCS}
)

target_link_libraries(lwip socket_wasi_ext)

target_include_directories(
	lwip
	PRIVATE ${LWIP_DIR}/Third_Party/STM32CubeF7/Middlewares/Third_Party/LwIP/system
	PRIVATE ${LWIP_DIR}/src/core
	PRIVATE ${LWIP_DIR}/src/netif
	PRIVATE ${LWIP_DIR}/src/include
	PRIVATE ${LWIP_CORE_DIR}/src
	PRIVATE ${LWIP_CORE_DIR}/src/include
	PRIVATE ${LWIP_CORE_DIR}/system
	PRIVATE ${cmsis_DIR}/include
	PRIVATE ${PROJECT_SOURCE_DIR}/../../library/include
)

set(INSTALL_CMAKE_DIR ${PROJECT_SOURCE_DIR}/public)

install(
	DIRECTORY ${LWIP_DIR}/Third_Party/STM32CubeF7/Middlewares/Third_Party/LwIP/system/arch/
	DESTINATION ${INSTALL_CMAKE_DIR}/include/system/arch
)

install(
	DIRECTORY ${LWIP_CORE_DIR}/src/include/
	DESTINATION ${INSTALL_CMAKE_DIR}/include/lwip
)
install(
	DIRECTORY ${LWIP_DIR}/src/include/
	DESTINATION ${INSTALL_CMAKE_DIR}/include/posix
)

install(
	TARGETS lwip socket_wasi_ext
	DESTINATION     ${INSTALL_CMAKE_DIR} 
	EXPORT lwip-export
    LIBRARY         DESTINATION lib
    INCLUDES        DESTINATION include
    PUBLIC_HEADER   DESTINATION include
)
install(
	EXPORT lwip-export 
    FILE    lwip-config.cmake
	DESTINATION ${INSTALL_CMAKE_DIR}
    EXPORT_LINK_INTERFACE_LIBRARIES
)
