cmake_minimum_required(VERSION 3.10)
project(YourProject LANGUAGES C CXX)  # Add both C and CXX

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find bpftool
find_program(BPFTOOL bpftool REQUIRED)

# Generate vmlinux.h
add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/vmlinux.h
    COMMAND ${BPFTOOL} btf dump file /sys/kernel/btf/vmlinux format c > ${CMAKE_SOURCE_DIR}/vmlinux.h
    COMMENT "Generating vmlinux.h"
    VERBATIM
)

# Add custom target for vmlinux.h
add_custom_target(vmlinux_header
    DEPENDS ${CMAKE_SOURCE_DIR}/vmlinux.h
)

# Find libelf directly
find_path(LIBELF_INCLUDE_DIRS
    NAMES libelf.h
    PATHS /usr/include
    /usr/include/libelf
    /usr/local/include)

find_library(LIBELF_LIBRARIES
    NAMES elf
    PATHS /usr/lib
    /usr/lib64
    /usr/local/lib
    /usr/local/lib64)

# Find libbpf directly
find_path(LIBBPF_INCLUDE_DIRS
    NAMES bpf/libbpf.h
    PATHS /usr/include
    /usr/local/include)

find_library(LIBBPF_LIBRARIES
    NAMES bpf libbpf
    PATHS /usr/lib/x86_64-linux-gnu
          /usr/lib
          /usr/lib64
          /usr/local/lib
          /usr/local/lib64
)

# Find required tools
find_program(CLANG clang REQUIRED)

# Get kernel release version for headers
execute_process(
    COMMAND uname -r
    OUTPUT_VARIABLE KERNEL_RELEASE
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(STATUS "Building BPF object to: ${CMAKE_CURRENT_BINARY_DIR}/probe.bpf.o")
message(STATUS "KERNEL_RELEASE: ${KERNEL_RELEASE}")

# Add custom command to build BPF program
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/probe.bpf.o
    COMMAND ${CLANG}
    -g -O2 -target bpf
    -D__TARGET_ARCH_x86
    -D__BPF_TRACING__
    -D__KERNEL__
    -D__ASM_SYSREG_H
    -Wno-unused-value
    -Wno-pointer-sign
    -Wno-compare-distinct-pointer-types
    -Wno-gnu-variable-sized-type-not-at-end
    -Wno-address-of-packed-member
    -Wno-tautological-compare
    -Wno-unknown-warning-option
    -I/usr/src/linux-headers-${KERNEL_RELEASE}/arch/x86/include/generated
    -I/usr/src/linux-headers-${KERNEL_RELEASE}/arch/x86/include/generated/uapi
    -I/usr/src/linux-headers-${KERNEL_RELEASE}/arch/x86/include/uapi
    -I/usr/src/linux-headers-${KERNEL_RELEASE}/include/uapi
    -I/usr/src/linux-headers-${KERNEL_RELEASE}/include
    -I/usr/src/linux-headers-${KERNEL_RELEASE}/arch/x86/include
    -I/usr/src/linux-headers-${KERNEL_RELEASE}/include/generated/uapi
    -I/usr/include
    -c ${CMAKE_SOURCE_DIR}/probe.bpf.c
    -o ${CMAKE_CURRENT_BINARY_DIR}/probe.bpf.o
    DEPENDS ${CMAKE_SOURCE_DIR}/probe.bpf.c
            ${CMAKE_SOURCE_DIR}/vmlinux.h
    COMMENT "Building BPF program"
    VERBATIM
)

# Add custom target for BPF program
add_custom_target(bpf_program ALL    # Add ALL to make it build by default
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/probe.bpf.o
)

# Make bpf_program depend on vmlinux header
add_dependencies(bpf_program vmlinux_header)

if(NOT LIBELF_INCLUDE_DIRS OR NOT LIBELF_LIBRARIES)
    message(FATAL_ERROR "libelf not found")
endif()

add_executable(MainApp
    main.cpp
    ../Interface.h
)

target_include_directories(MainApp
    PRIVATE
    ${LIBBPF_INCLUDE_DIRS}
    ${LIBELF_INCLUDE_DIRS}
)

target_link_libraries(MainApp
    PRIVATE
    ${LIBBPF_LIBRARIES}
    ${LIBELF_LIBRARIES})

# Make MainApp depend on bpf_program
add_dependencies(MainApp bpf_program)

# Copy BPF object to the same directory as MainApp
add_custom_command(
    TARGET MainApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying BPF object to: $<TARGET_FILE_DIR:MainApp>/probe.bpf.o"
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_BINARY_DIR}/probe.bpf.o
            $<TARGET_FILE_DIR:MainApp>/probe.bpf.o
    VERBATIM
)

target_compile_features(MainApp PRIVATE cxx_std_17)

# Add this to ensure proper C/C++ flags
target_compile_options(MainApp PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-fno-strict-aliasing>
)