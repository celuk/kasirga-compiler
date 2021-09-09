cmake_minimum_required(VERSION 3.13.4)
project(alp)

set(LT_LLVM_INSTALL_DIR "" CACHE PATH "LLVM installation directory")


set(LT_LLVM_INCLUDE_DIR "${LT_LLVM_INSTALL_DIR}/include/llvm")
if(NOT EXISTS "${LT_LLVM_INCLUDE_DIR}")
message(FATAL_ERROR
  " LT_LLVM_INSTALL_DIR (${LT_LLVM_INCLUDE_DIR}) is invalid.")
endif()

set(LT_LLVM_CMAKE_FILE "${LT_LLVM_INSTALL_DIR}/lib/cmake/llvm/LLVMConfig.cmake")

## aslinda build diri de export etmek istenmesi lazim, o da optional olmali
if(NOT EXISTS "${LT_LLVM_CMAKE_FILE}")
## ~/llvm/llvm-project/llvm den ~/llvm/llvm-project/
## ~/llvm/llvm-project/build/lib/cmake/llvm
get_filename_component(UPPER_DIR ${LT_LLVM_INSTALL_DIR} DIRECTORY)
set(LT_LLVM_CMAKE_FILE "${UPPER_DIR}/build/lib/cmake/llvm/LLVMConfig.cmake")

endif()
##

if(NOT EXISTS "${LT_LLVM_CMAKE_FILE}")
message(FATAL_ERROR
    " LT_LLVM_CMAKE_FILE (${LT_LLVM_CMAKE_FILE}) is invalid.")
endif()



list(APPEND CMAKE_PREFIX_PATH "${LT_LLVM_INSTALL_DIR}/lib/cmake/llvm/")

find_package(LLVM REQUIRED CONFIG)


#if(NOT "11" VERSION_EQUAL "${LLVM_VERSION_MAJOR}")
#  message(FATAL_ERROR "Found LLVM ${LLVM_VERSION_MAJOR}, but need LLVM 11")
#endif()

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LT_LLVM_INSTALL_DIR}")

message("LLVM STATUS:
  Definitions ${LLVM_DEFINITIONS}
  Includes    ${LLVM_INCLUDE_DIRS}
  Libraries   ${LLVM_LIBRARY_DIRS}
  Targets     ${LLVM_TARGETS_TO_BUILD}"
)


#####################
get_filename_component(LT_LLVM_PROJECT_DIR ${LT_LLVM_INSTALL_DIR} DIRECTORY)
#get_filename_component(UPPER_DIR ${LT_LLVM_INSTALL_DIR} DIRECTORY)
#set(LT_LLVM_PROJECT_DIR "${UPPER_DIR}")
#####################

# Set the LLVM header and library paths
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
##
include_directories(SYSTEM "${LT_LLVM_PROJECT_DIR}/llvm/lib/Target/RISCV")
include_directories(SYSTEM "${LT_LLVM_PROJECT_DIR}/build/lib/Target/RISCV")
#include_directories(SYSTEM "~/llvm/llvm-project/build/lib/cmake/llvm")
#include_directories(SYSTEM "/home/shc/llvm-new/llvm-project/llvm/lib/Target/")
##
link_directories(${LLVM_LIBRARY_DIRS})
add_definitions(${LLVM_DEFINITIONS})



set(CMAKE_CXX_STANDARD 14 CACHE STRING "")


if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE
      STRING "Build type (default Debug):" FORCE)
endif()


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall\
    -fdiagnostics-color=always")



if(NOT LLVM_ENABLE_RTTI)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()



include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-fvisibility-inlines-hidden" SUPPORTS_FVISIBILITY_INLINES_HIDDEN_FLAG)
#if (${SUPPORTS_FVISIBILITY_INLINES_HIDDEN_FLAG} EQUAL "1")
#  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility-inlines-hidden")
#endif()


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")

##
#set(LLVM_TARGET_DEFINITIONS RISCV.td)
#
#tablegen(LLVM RISCVGenAsmMatcher.inc -gen-asm-matcher)
#tablegen(LLVM RISCVGenAsmWriter.inc -gen-asm-writer)
#tablegen(LLVM RISCVGenCompressInstEmitter.inc -gen-compress-inst-emitter)
#tablegen(LLVM RISCVGenDAGISel.inc -gen-dag-isel)
#tablegen(LLVM RISCVGenDisassemblerTables.inc -gen-disassembler)
#tablegen(LLVM RISCVGenGlobalISel.inc -gen-global-isel)
#tablegen(LLVM RISCVGenInstrInfo.inc -gen-instr-info)
#tablegen(LLVM RISCVGenMCCodeEmitter.inc -gen-emitter)
#tablegen(LLVM RISCVGenMCPseudoLowering.inc -gen-pseudo-lowering)
#tablegen(LLVM RISCVGenRegisterBank.inc -gen-register-bank)
#tablegen(LLVM RISCVGenRegisterInfo.inc -gen-register-info)
#tablegen(LLVM RISCVGenSubtargetInfo.inc -gen-subtarget)
#tablegen(LLVM RISCVGenSystemOperands.inc -gen-searchable-tables)
#
#add_public_tablegen_target(RISCVCommonTableGen)
##

## config.h icin aradigimiz sey buna benzer olabilir
#/home/shc/llvm/llvm-project/clang -----> cmake current source bura olabilir cmakelist.txtnin olduğu yer buraydı.
#set(CLANG_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
#set(CLANG_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
#configure_file(
#  ${CLANG_SOURCE_DIR}/include/clang/Config/config.h.cmake
#  ${CLANG_BINARY_DIR}/include/clang/Config/config.h)
##




add_subdirectory(tools)


  #LINK_COMPONENTS
  #Analysis
  #AsmPrinter
  #Core
  #CodeGen
  #MC
  #RISCVDesc
  #RISCVInfo
  #SelectionDAG
  #Support
  #Target
  #GlobalISel

  #ADD_TO_COMPONENT
  #RISCV
  #)

