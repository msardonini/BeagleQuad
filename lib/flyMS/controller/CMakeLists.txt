
add_library(flyMS_controller STATIC
  DigitalFilter.cc
  position_controller.cc
  Setpoint.cc
  AttitudeController.cc
)

target_link_libraries(flyMS_controller
PUBLIC
  spdlog
  eigen
  yaml-cpp
  flyMS_ipc
)

target_include_directories(flyMS_controller
PUBLIC
  ${FLYMS_LIBRARY_ROOT}
)

# Build with DEBUG defined if we are in Debug mode
if (CMAKE_BUILD_TYPE MATCHES Debug)
target_compile_definitions(flyMS_controller
  PUBLIC
    -DDEBUG_FLYMS
  )
endif()


if(FLYMS_BUILD_TESTS)
  add_subdirectory(test)
endif()
