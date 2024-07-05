

add_library(flyMS_mavlink STATIC
  MavlinkUart.cc
)

target_link_libraries(flyMS_mavlink
  PUBLIC
    mavlink_v2
    spdlog
    flyMS_uart
)

target_include_directories(flyMS_mavlink
  PUBLIC
    ${FLYMS_LIBRARY_ROOT}
)

# Build with DEBUG defined if we are in Debug mode
if (CMAKE_BUILD_TYPE MATCHES Debug)
  target_compile_definitions(flyMS_mavlink
    PUBLIC
      -DDEBUG_FLYMS
    )
endif()

