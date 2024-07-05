
add_library(flyMS_mavlink_redis STATIC
  MavlinkRedisSub.cc
)

target_link_libraries(flyMS_mavlink_redis
  PUBLIC
    spdlog
    mavlink_v2
    flyMS_redis
)

target_include_directories(flyMS_mavlink_redis
  PUBLIC
    ${FLYMS_LIBRARY_ROOT}
)

# Build with DEBUG defined if we are in Debug mode
if (CMAKE_BUILD_TYPE MATCHES Debug)
  target_compile_definitions(flyMS_mavlink_redis
    PUBLIC
      -DDEBUG_FLYMS
    )
endif()
