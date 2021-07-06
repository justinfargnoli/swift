add_swift_host_library(swiftSILAlive STATIC
  SILAlive.cpp)

# FIXME: Make this independent of my system
target_link_directories(swiftSILAlive PRIVATE
  /Users/justinfargnoli/Documents/projects/sil-alive/swift-project/build/Ninja-ReleaseAssert/alive-macosx-x86_64/
  /Users/justinfargnoli/Documents/projects/sil-alive/swift-project/build/Ninja-ReleaseAssert/z3-macosx-x86_64/)
