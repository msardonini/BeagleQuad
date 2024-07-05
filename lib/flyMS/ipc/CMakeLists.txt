cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)

if (NOT TARGET spdlog)
  add_subdirectory(external)
endif()


# check if FLYMS_LIBRARY_ROOT is set
if(NOT DEFINED FLYMS_LIBRARY_ROOT)
  set(FLYMS_LIBRARY_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
endif()

add_subdirectory(uart)
add_subdirectory(mavlink)
add_subdirectory(redis)
add_subdirectory(mavlink_redis)

add_library(flyMS_ipc INTERFACE)

target_link_libraries(flyMS_ipc
  INTERFACE
    flyMS_mavlink
    flyMS_uart
    flyMS_redis
    # redis++_static
    flyMS_mavlink_redis
)

target_include_directories(flyMS_ipc
  INTERFACE
    ${FLYMS_LIBRARY_ROOT}
)

# Build with DEBUG defined if we are in Debug mode
if (CMAKE_BUILD_TYPE MATCHES Debug)
  target_compile_definitions(flyMS_ipc
    INTERFACE
      -DDEBUG_FLYMS
    )
endif()

if (FLYMS_BUILD_TESTS)
  add_subdirectory(test)
endif()
