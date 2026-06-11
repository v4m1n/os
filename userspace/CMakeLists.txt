# Clear kernel flags inherited from parent directory
set(CMAKE_CXX_FLAGS "")
set(CMAKE_C_FLAGS "")
set(CMAKE_ASM_FLAGS "")

# Compile/link options for userspace stdlib and applications
set(USERSPACE_COMPILE_FLAGS "-m64" "-O2" "-gdwarf-2" "-Wall" "-Wextra" "-Werror" "-Wno-nonnull-compare" "-nostdinc" "-fno-builtin" "-fno-exceptions" "-fno-stack-protector" "-fcf-protection=none" "-fno-pic" "-fno-pie" "-fcheck-new")
set(USERSPACE_LINK_FLAGS "-nostdlib" "-nostartfiles" "-nodefaultlibs" "-static" "-no-pie" "-Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/linker.ld")
set(USERSPACE_ASM_FLAGS "-m64" "-gdwarf-2" "-nostdlib" "-fcf-protection=none" "-fno-pic" "-fno-pie")

# Userspace Standard Library
set(STDLIB_SOURCES
  stdlib/crt0.S
  stdlib/src/syscall.S
  stdlib/src/stdio.cpp
  stdlib/src/stdlib.cpp
  stdlib/src/cpp_support.cpp
)

add_library(userspace_stdlib STATIC ${STDLIB_SOURCES})

# Disable standard include paths and add compiler freestanding include paths + stdlib headers
target_compile_options(userspace_stdlib PRIVATE
  $<$<COMPILE_LANGUAGE:C>:${USERSPACE_COMPILE_FLAGS}>
  $<$<COMPILE_LANGUAGE:CXX>:${USERSPACE_COMPILE_FLAGS};-fno-rtti>
  $<$<COMPILE_LANGUAGE:ASM>:${USERSPACE_ASM_FLAGS}>
)

target_include_directories(userspace_stdlib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/stdlib/include
)

foreach(PATH ${COMPILER_INCLUDE_DIRS})
  target_include_directories(userspace_stdlib SYSTEM BEFORE PRIVATE "${PATH}")
endforeach()

# Automatically detect all C and C++ applications in userspace/ directory
file(GLOB UserspaceApps CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

# Create the bin output directory in build directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(USERSPACE_APP_TARGETS_LOCAL "")

foreach(APP_SOURCE ${UserspaceApps})
  get_filename_component(APP_NAME "${APP_SOURCE}" NAME_WE)
  
  message(STATUS "Found userspace application: ${APP_NAME} (${APP_SOURCE})")
  
  # Create the executable target
  add_executable(${APP_NAME} ${APP_SOURCE} stdlib/crt0.S)
  
  # Compile options for C and C++
  target_compile_options(${APP_NAME} PRIVATE
    $<$<COMPILE_LANGUAGE:C>:${USERSPACE_COMPILE_FLAGS}>
    $<$<COMPILE_LANGUAGE:CXX>:${USERSPACE_COMPILE_FLAGS};-fno-rtti>
    $<$<COMPILE_LANGUAGE:ASM>:${USERSPACE_ASM_FLAGS}>
  )
  
  # Link options
  target_link_options(${APP_NAME} PRIVATE ${USERSPACE_LINK_FLAGS})
  
  # Include directories
  target_include_directories(${APP_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/stdlib/include)
  foreach(PATH ${COMPILER_INCLUDE_DIRS})
    target_include_directories(${APP_NAME} SYSTEM BEFORE PRIVATE "${PATH}")
  endforeach()
  
  # Link standard library
  target_link_libraries(${APP_NAME} PRIVATE userspace_stdlib)
  
  # Set output directory to CMAKE_BINARY_DIR/bin
  set_target_properties(${APP_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  )
  
  # Generate flat binary after build
  if(NOT CMAKE_OBJCOPY)
    find_program(CMAKE_OBJCOPY objcopy)
  endif()
  
  if(CMAKE_OBJCOPY)
    add_custom_command(
      TARGET ${APP_NAME} POST_BUILD
      COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${APP_NAME}> $<TARGET_FILE:${APP_NAME}>.bin
      COMMENT "Generating flat binary for ${APP_NAME}"
    )
  endif()
  
  list(APPEND USERSPACE_APP_TARGETS_LOCAL ${APP_NAME})
endforeach()

# Share the detected targets list with parent scope
set(USERSPACE_APP_TARGETS ${USERSPACE_APP_TARGETS_LOCAL} PARENT_SCOPE)
