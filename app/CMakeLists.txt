
# Build test webserver program
add_executable(test_webserver
  test_webserver.cc
)
target_link_libraries(test_webserver
  HTTPRequest
  yaml-cpp
)

# Build flyMS
add_executable(flyMS
  flyMS.cc
)
target_link_libraries(flyMS
  PRIVATE
    flyMS_lib
)

add_executable(mission_interface
  mission_interface.cc
)
target_link_libraries(mission_interface
  PRIVATE
    flyMS_lib
    flyMS_ipc
)

set_target_properties(flyMS test_webserver mission_interface
  PROPERTIES
  LINK_FLAGS "-static-libstdc++ -static-libgcc"
)
