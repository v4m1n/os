cmake_minimum_required(VERSION 3.20)

# set the project name and version
project(OS)

enable_language(C ASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP true)

FILE(GLOB CPPSource *.cpp)
FILE(GLOB SSource *.S)
#enable_language(ASM_NASM)
#set(CMAKE_ASM_NASM_COMPILER "nasm")
#set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> <INCLUDES> <FLAGS> -f elf64 -o <OBJECT> <SOURCE>")
#set(CMAKE_ASM_NASM_LINK_EXECUTABLE "ld <CMAKE_ASM_NASM_LINK_FLAGS> <LINK_FLAGS> <OBJECTS>  -o <TARGET> <LINK_LIBRARIES>")
#set_source_files_properties(${SSource} PROPERTIES LANGUAGE ASM_NASM)

ADD_EXECUTABLE(kernel.elf ${CPPSource} ${SSource})

set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_ASM_COMPILER "g++")
set(CMAKE_C_COMPILER "gcc")

set(CMAKE_KERNEL_FLAGS "-m64 -O0 -masm=intel -gdwarf-2 -Wall -Wextra -Werror -Wno-nonnull-compare -nostdinc -nostdlib -nostartfiles -nodefaultlibs -fno-builtin -fno-exceptions -fno-stack-protector -ffreestanding -mcmodel=kernel -mno-red-zone -mgeneral-regs-only -mno-mmx -mno-sse2 -mno-sse3 -mno-3dnow -fcf-protection=none -fno-pic -static")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CMAKE_KERNEL_FLAGS} -nostdinc++ -fno-rtti -g")
set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} ${CMAKE_KERNEL_FLAGS}")
set(CMAKE_ASM_FLAGS  "${CMAKE_ASM_FLAGS} ${CMAKE_KERNEL_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T ${CMAKE_SOURCE_DIR}/linker.ld")



include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/containers)

add_custom_command(OUTPUT disk.img
  COMMAND gunzip "${CMAKE_CURRENT_SOURCE_DIR}/disk/disk.img.gz" -c > ${CMAKE_CURRENT_BINARY_DIR}/disk.img
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Unpacking disk..."
)

add_custom_target(image
  DEPENDS disk.img
)
add_library(OBJS OBJECT disk.img)

add_custom_command(OUTPUT dsk
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/disk/setup.sh
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS disk.img
  COMMENT "Copying kernel to disk..."
)
  

add_custom_target(qemu
    COMMAND qemu-system-x86_64 -m 8G -smp 4 -no-reboot -debugcon stdio -drive format=raw,file=disk.img -enable-kvm -cpu host
    COMMENT "launch qemu `qemu-system-x86_64 -m 8G -smp 4 -no-reboot -debugcon stdio -drive format=raw,file=disk.img -enable-kvm -cpu host`"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)


add_custom_target(
  disk ALL
  DEPENDS dsk
)

add_dependencies(disk image)
add_dependencies(disk kernel.elf)  

