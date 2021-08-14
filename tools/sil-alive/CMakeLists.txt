add_swift_host_tool(sil-alive
  SILAlive.cpp
  SWIFT_COMPONENT tools
)
target_link_libraries(sil-alive
                      PRIVATE
                        swiftClangImporter
                        swiftDemangling
                        swiftFrontend
                        swiftSerialization
                        swiftSILGen
                        swiftSILOptimizer
                        swiftSILAlive)
