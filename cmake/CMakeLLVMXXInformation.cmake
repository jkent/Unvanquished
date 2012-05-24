find_program( CLANGXX clang++ )
find_program( LLVM_LINK llvm-link )
if( NOT CLANGXX OR NOT LLVM_LINK )
  message( FATAL_ERROR "clang is not installed" )
endif()

set( CMAKE_LLVMXX_COMPILER_ENV_VAR LLVMXXCC )

set( CMAKE_LLVMXX_COMPILER "${CLANGXX}" CACHE STRING "" )
set( CMAKE_LLVMXX_LINKER "${LLVM_LINK}" CACHE STRING "" )
set( CMAKE_LLVMXX_FLAGS "" CACHE STRING "" )
set( CMAKE_INCLUDE_FLAG_LLVMXX -I )
set( CMAKE_LLVMXX_SOURCE_FILE_EXTENSIONS llvm )
set( CMAKE_LLVMXX_OUTPUT_EXTENSION .bc )
set( CMAKE_LLVMXX_COMPILE_OBJECT "<CMAKE_LLVMXX_COMPILER> <DEFINES> <FLAGS> -o <OBJECT> -S <SOURCE>" )
set( CMAKE_LLVMXX_CREATE_SHARED_LIBRARY "${CMAKE_LLVM_LINKER} -o <TARGET> <OBJECTS>" )
set( CMAKE_LLVMXX_LINK_EXECUTABLE "<CMAKE_LLVMXX_COMPILER> <FLAGS> <CMAKE_LLVMXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET>")

set( CMAKE_LLVMXX_FLAGS_INIT "$ENV{CFLAGS} ${CMAKE_C_FLAGS_INIT}" )
if( CMAKE_LLVMXX_FLAGS_INIT STREQUAL " ")
  set( CMAKE_LLVMXX_FLAGS_INIT)
endif()

set( CMAKE_LLVMXX_FLAGS "${CMAKE_LLVMXX_FLAGS_INIT}" CACHE STRING
     "Flags used by the compiler during all build types.")
set( CMAKE_LLVMXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG_INIT}" CACHE STRING
     "Flags used by the compiler during debug builds.")
set( CMAKE_LLVMXX_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL_INIT}" CACHE STRING
    "Flags used by the compiler during release minsize builds.")
set( CMAKE_LLVMXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE_INIT}" CACHE STRING
    "Flags used by the compiler during release builds (/MD /Ob1 /Oi /Ot /Oy /Gs will produce slightly less optimized but smaller files).")
set( CMAKE_LLVMXX_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO_INIT}" CACHE STRING
    "Flags used by the compiler during Release with Debug Info builds.")

mark_as_advanced(
  CMAKE_LLVMXX_FLAGS
  CMAKE_LLVMXX_FLAGS_DEBUG
  CMAKE_LLVMXX_FLAGS_MINSIZEREL
  CMAKE_LLVMXX_FLAGS_RELEASE
  CMAKE_LLVMXX_FLAGS_RELWITHDEBINFO
)

set( CMAKE_LLVMXX_INFORMATION_LOADED 1 )
