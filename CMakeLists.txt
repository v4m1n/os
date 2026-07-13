cmake_minimum_required(VERSION 3.20)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24")
  set(CMAKE_COLOR_DIAGNOSTICS ON)
else()
  add_compile_options(-fdiagnostics-color=always)
endif()

set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_ASM_COMPILER "g++")
set(CMAKE_C_COMPILER "gcc")

project(OS)

set(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES "")
set(CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES "")

enable_language(C ASM)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP true)

FILE(GLOB CPPSource CONFIGURE_DEPENDS *.cpp driver/*.cpp)
FILE(GLOB_RECURSE ModuleSources CONFIGURE_DEPENDS include/*.cppm)
FILE(GLOB SSource CONFIGURE_DEPENDS *.S)
#enable_language(ASM_NASM)
#set(CMAKE_ASM_NASM_COMPILER "nasm")
#set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> <INCLUDES> <FLAGS> -f elf64 -o <OBJECT> <SOURCE>")
#set(CMAKE_ASM_NASM_LINK_EXECUTABLE "ld <CMAKE_ASM_NASM_LINK_FLAGS> <LINK_FLAGS> <OBJECTS>  -o <TARGET> <LINK_LIBRARIES>")
#set_source_files_properties(${SSource} PROPERTIES LANGUAGE ASM_NASM)
option(ENABLE_ANALYZER "Enable GCC static analyzer (-fanalyzer)" ON)

set(CMAKE_KERNEL_FLAGS "-m64 -masm=intel -O0 -gdwarf-2 -Wall -Wextra -Werror -Wno-nonnull-compare -nostdinc -nostdlib -nostartfiles -nodefaultlibs -fno-builtin -fno-exceptions -fno-stack-protector -ffreestanding -mcmodel=kernel -mno-red-zone -mgeneral-regs-only -mno-mmx -mno-sse2 -mno-sse3 -mno-3dnow -fcf-protection=none -fno-pic -static")

if(ENABLE_ANALYZER)
  set(CMAKE_KERNEL_FLAGS "${CMAKE_KERNEL_FLAGS} -fanalyzer")
endif()

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CMAKE_KERNEL_FLAGS} -nostdinc++ -fno-rtti -fmodules-ts -g -D_GLIBCXX_OS_DEFINES -D__glibcxx_format=1 -D__glibcxx_format_ranges=202207L")
set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} ${CMAKE_KERNEL_FLAGS}")
set(CMAKE_ASM_FLAGS  "${CMAKE_ASM_FLAGS} ${CMAKE_KERNEL_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T ${CMAKE_SOURCE_DIR}/linker.ld")

ADD_EXECUTABLE(kernel.elf ${CPPSource} ${SSource})
target_sources(kernel.elf PUBLIC FILE_SET CXX_MODULES FILES
  ${ModuleSources}
)
set_target_properties(kernel.elf PROPERTIES CXX_SCAN_FOR_MODULES ON)

# Automatically detect compiler's standard C++ and C freestanding include paths
set(COMPILER_INCLUDE_DIRS "")
execute_process(
  COMMAND ${CMAKE_CXX_COMPILER} -E -x c++ -v /dev/null
  ERROR_VARIABLE GCC_OUTPUT
  OUTPUT_QUIET
)
string(REPLACE "\n" ";" GCC_LINES "${GCC_OUTPUT}")
foreach(LINE ${GCC_LINES})
  string(STRIP "${LINE}" LINE_STRIPPED)
  if(LINE_STRIPPED MATCHES "c\\+\\+" OR (LINE_STRIPPED MATCHES "lib/gcc" AND LINE_STRIPPED MATCHES "include$"))
    get_filename_component(ABS_PATH "${LINE_STRIPPED}" ABSOLUTE)
    if(EXISTS "${ABS_PATH}")
      message(STATUS "Found compiler include path: ${ABS_PATH}")
      list(APPEND COMPILER_INCLUDE_DIRS "${ABS_PATH}")
    endif()
  endif()
endforeach()

foreach(PATH ${COMPILER_INCLUDE_DIRS})
  target_include_directories(kernel.elf SYSTEM BEFORE PRIVATE "${PATH}")
endforeach()

target_include_directories(kernel.elf PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(kernel.elf PRIVATE ${PROJECT_SOURCE_DIR}/include/containers)
target_include_directories(kernel.elf PRIVATE ${PROJECT_SOURCE_DIR}/include/driver)

add_custom_command(OUTPUT disk.img
  COMMAND xz -d -c "${CMAKE_CURRENT_SOURCE_DIR}/disk/disk.img.xz" > ${CMAKE_CURRENT_BINARY_DIR}/disk.img
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Unpacking disk..."
)

add_custom_target(image
  DEPENDS disk.img
)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/disk/grub.cfg ${CMAKE_CURRENT_BINARY_DIR}/grub.cfg COPYONLY)
add_subdirectory(userspace)

add_custom_command(OUTPUT dsk
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/disk/setup.sh
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS disk.img kernel.elf ${USERSPACE_APP_TARGETS}
  COMMENT "Copying files to disk..."
)

add_custom_target(qemu
  COMMAND qemu-system-x86_64 -m 8G -smp 4 -no-reboot -debugcon stdio -drive format=raw,file=disk.img,id=NVME1,if=none -device nvme,drive=NVME1,serial=nvme1 -bios /usr/share/ovmf/x64/OVMF.4m.fd -display none
  COMMENT "launch qemu `qemu-system-x86_64 -m 8G -smp 4 -no-reboot -debugcon stdio -drive format=raw,file=disk.img,id=NVME1,if=none -device nvme,drive=NVME1,serial=nvme1 -bios /usr/share/ovmf/x64/OVMF.4m.fd -display none`"
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  USES_TERMINAL
)
add_custom_target(kvm
  COMMAND qemu-system-x86_64 -m 8G -smp 4 -no-reboot -debugcon stdio -drive format=raw,file=disk.img,id=NVME1,if=none -device nvme,drive=NVME1,serial=nvme1 -enable-kvm -cpu host -bios /usr/share/ovmf/x64/OVMF.4m.fd -display none
  COMMENT "launch qemu `qemu-system-x86_64 -m 8G -smp 4 -no-reboot -debugcon stdio -drive format=raw,file=disk.img,id=NVME1,if=none -device nvme,drive=NVME1,serial=nvme1 -enable-kvm -cpu host -bios /usr/share/ovmf/x64/OVMF.4m.fd -display none`"
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  USES_TERMINAL
)


add_custom_target(
  disk ALL
  DEPENDS dsk
)

add_dependencies(disk image)
add_dependencies(disk kernel.elf)  

add_dependencies(qemu disk)
add_dependencies(kvm disk)

