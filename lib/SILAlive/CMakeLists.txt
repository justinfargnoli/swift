add_swift_host_library(swiftSILAlive STATIC
  SILAlive.cpp)
target_link_libraries(swiftSILAlive PRIVATE
  libir
  libsmt
  libtools
  libutil
  swiftSILOptimizer)
