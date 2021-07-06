add_swift_host_library(swiftSILAlive STATIC
  SILAlive.cpp)

# FIXME: Make this independent of my system
# target_link_directories(swiftSILAlive PRIVATE
#   /Users/justinfargnoli/Documents/projects/sil-alive/swift-project/build/Ninja-ReleaseAssert/alive-macosx-x86_64
#   /Users/justinfargnoli/Documents/projects/sil-alive/swift-project/build/Ninja-ReleaseAssert/z3-macosx-x86_64)

target_link_libraries(swiftSILAlive PRIVATE
  /Users/justinfargnoli/Documents/projects/sil-alive/swift-project/build/Ninja-ReleaseAssert/alive-macosx-x86_64/libir.a
  /Users/justinfargnoli/Documents/projects/sil-alive/swift-project/build/Ninja-ReleaseAssert/alive-macosx-x86_64/libsmt.a
  /Users/justinfargnoli/Documents/projects/sil-alive/swift-project/build/Ninja-ReleaseAssert/alive-macosx-x86_64/libutil.a
  # tools
  /Users/justinfargnoli/Documents/projects/sil-alive/swift-project/build/Ninja-ReleaseAssert/alive-macosx-x86_64/libllvm_util.a
  /Users/justinfargnoli/Documents/projects/sil-alive/swift-project/build/Ninja-ReleaseAssert/z3-macosx-x86_64/libz3.dylib)
