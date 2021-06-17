add_swift_host_library(swiftSILAlive STATIC
  SILAlive.cpp)
target_link_libraries(swiftSILAlive PRIVATE
  swiftSILOptimizer)
# set_swift_llvm_is_available(swiftSILAlive)

# set(Z3_INCLUDE_DIR "/usr/local/Cellar/z3/4.8.10/include" CACHE PATH FORCE)
# set(LLVM_ENABLE_Z3_SOLVER "YES" CACHE BOOL "" FORCE)

# message(STATUS "Z3_INCLUDE_DIR: ${Z3_INCLUDE_DIR}")
# message(STATUS "LLVM_ENABLE_Z3_SOLVER: ${LLVM_ENABLE_Z3_SOLVER}")

add_subdirectory(alive2)
