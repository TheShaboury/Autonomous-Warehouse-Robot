cmake_minimum_required(VERSION 3.8)
project(arduino_hardware_interface)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(hardware_interface REQUIRED)
find_package(pluginlib REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)

# LibSerial is required for communication
find_path(LIBSERIAL_INCLUDE_DIR libserial/SerialPort.h)
find_library(LIBSERIAL_LIBRARIES serial)

add_library(
  ${PROJECT_NAME}
  SHARED
  src/arduino_system.cpp
)

target_include_directories(
  ${PROJECT_NAME}
  PRIVATE
  include
  ${LIBSERIAL_INCLUDE_DIR}
)

target_link_libraries(
  ${PROJECT_NAME}
  ${LIBSERIAL_LIBRARIES}
)

ament_target_dependencies(
  ${PROJECT_NAME}
  hardware_interface
  pluginlib
  rclcpp
  rclcpp_lifecycle
)

# Export the plugin XML
pluginlib_export_plugin_description_file(
  hardware_interface
  arduino_hardware_plugins.xml
)

install(
  TARGETS ${PROJECT_NAME}
  DESTINATION lib/${PROJECT_NAME}
)

install(
  DIRECTORY include/
  DESTINATION include
)

install(
  FILES arduino_hardware_plugins.xml
  DESTINATION share/${PROJECT_NAME}
)

ament_export_include_directories(
  include
)

ament_export_libraries(
  ${PROJECT_NAME}
)

ament_export_dependencies(
  hardware_interface
  pluginlib
  rclcpp
  rclcpp_lifecycle
)

ament_package()