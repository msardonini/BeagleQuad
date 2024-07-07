add_library(flyMS_redis STATIC
  RedisSubscriber.cc
)

target_link_libraries(flyMS_redis
  PUBLIC
    spdlog
    redis++_static
    yaml-cpp
    flyMS_mavlink
)

target_include_directories(flyMS_redis
  PUBLIC
    ${FLYMS_LIBRARY_ROOT}
)

# Build with DEBUG defined if we are in Debug mode
if (CMAKE_BUILD_TYPE MATCHES Debug)
  target_compile_definitions(flyMS_redis
    PUBLIC
      -DDEBUG_FLYMS
    )
endif()
