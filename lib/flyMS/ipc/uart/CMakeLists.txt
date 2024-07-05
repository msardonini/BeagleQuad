

add_library(flyMS_uart STATIC
  Uart.cc
)

target_link_libraries(flyMS_uart
  PUBLIC
    spdlog
    stdc++fs
)

target_include_directories(flyMS_uart
  PUBLIC
    ${FLYMS_LIBRARY_ROOT}
)

# Build with DEBUG defined if we are in Debug mode
if (CMAKE_BUILD_TYPE MATCHES Debug)
  target_compile_definitions(flyMS_uart
    PUBLIC
      -DDEBUG_FLYMS
    )
endif()
