#include "arduino_hardware_interface/arduino_system.hpp"

#include <chrono>
#include <cmath>
#include <limits>
#include <memory>
#include <vector>

#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "rclcpp/rclcpp.hpp"

namespace arduino_hardware_interface
{
hardware_interface::CallbackReturn ArduinoSystemHardware::on_init(
  const hardware_interface::HardwareInfo & info)
{
  if (
    hardware_interface::SystemInterface::on_init(info) !=
    hardware_interface::CallbackReturn::SUCCESS)
  {
    return hardware_interface::CallbackReturn::ERROR;
  }

  // Load Parameters
  cfg_.left_wheel_name = info_.hardware_parameters["left_wheel_name"];
  cfg_.right_wheel_name = info_.hardware_parameters["right_wheel_name"];
  
  if (info_.hardware_parameters.count("stepper_joint_name")){
    cfg_.lifter_name = info_.hardware_parameters["stepper_joint_name"];
  } else {
    cfg_.lifter_name = "lifter_joint"; 
  }

  cfg_.loop_rate = std::stof(info_.hardware_parameters["loop_rate"]);
  cfg_.device = info_.hardware_parameters["device"];
  cfg_.baud_rate = std::stoi(info_.hardware_parameters["baud_rate"]);
  cfg_.timeout_ms = std::stoi(info_.hardware_parameters["timeout_ms"]);
  cfg_.enc_counts_per_rev = std::stoi(info_.hardware_parameters["enc_counts_per_rev"]);
  
  if (info_.hardware_parameters.count("pid_p") > 0)
  {
    cfg_.pid_p = std::stoi(info_.hardware_parameters["pid_p"]);
    cfg_.pid_d = std::stoi(info_.hardware_parameters["pid_d"]);
    cfg_.pid_i = std::stoi(info_.hardware_parameters["pid_i"]);
    cfg_.pid_o = std::stoi(info_.hardware_parameters["pid_o"]);
  }
  else
  {
    RCLCPP_INFO(rclcpp::get_logger("ArduinoSystemHardware"), "PID values not supplied, using defaults.");
  }

  wheel_l_.setup(cfg_.left_wheel_name, cfg_.enc_counts_per_rev);
  wheel_r_.setup(cfg_.right_wheel_name, cfg_.enc_counts_per_rev);

  for (const hardware_interface::ComponentInfo & joint : info_.joints)
  {
    // Lifter Logic
    if (joint.name == cfg_.lifter_name)
    {
       if (joint.command_interfaces.size() != 1)
       {
         RCLCPP_FATAL(rclcpp::get_logger("ArduinoSystemHardware"),
                      "Joint '%s' has %zu command interfaces found. 1 expected.", joint.name.c_str(), joint.command_interfaces.size());
         return hardware_interface::CallbackReturn::ERROR;
       }
       if (joint.command_interfaces[0].name != hardware_interface::HW_IF_POSITION)
       {
         RCLCPP_FATAL(rclcpp::get_logger("ArduinoSystemHardware"),
                      "Joint '%s' have %s command interfaces found. '%s' expected.", joint.name.c_str(),
                      joint.command_interfaces[0].name.c_str(), hardware_interface::HW_IF_POSITION);
         return hardware_interface::CallbackReturn::ERROR;
       }
       if (joint.state_interfaces[0].name != hardware_interface::HW_IF_POSITION)
       {
         RCLCPP_FATAL(rclcpp::get_logger("ArduinoSystemHardware"),
                      "Joint '%s' have %s state interfaces found. '%s' expected.", joint.name.c_str(),
                      joint.state_interfaces[0].name.c_str(), hardware_interface::HW_IF_POSITION);
         return hardware_interface::CallbackReturn::ERROR;
       }
    }
    // Wheels Logic
    else 
    {
      if (joint.command_interfaces.size() != 1)
      {
        RCLCPP_FATAL(rclcpp::get_logger("ArduinoSystemHardware"),
          "Joint '%s' has %zu command interfaces found. 1 expected.", joint.name.c_str(), joint.command_interfaces.size());
        return hardware_interface::CallbackReturn::ERROR;
      }
      if (joint.command_interfaces[0].name != hardware_interface::HW_IF_VELOCITY)
      {
        RCLCPP_FATAL(rclcpp::get_logger("ArduinoSystemHardware"),
          "Joint '%s' have %s command interfaces found. '%s' expected.", joint.name.c_str(),
          joint.command_interfaces[0].name.c_str(), hardware_interface::HW_IF_VELOCITY);
        return hardware_interface::CallbackReturn::ERROR;
      }
      if (joint.state_interfaces.size() != 2)
      {
        RCLCPP_FATAL(rclcpp::get_logger("ArduinoSystemHardware"),
          "Joint '%s' has %zu state interface. 2 expected.", joint.name.c_str(), joint.state_interfaces.size());
        return hardware_interface::CallbackReturn::ERROR;
      }
    }
  }

  return hardware_interface::CallbackReturn::SUCCESS;
}

std::vector<hardware_interface::StateInterface> ArduinoSystemHardware::export_state_interfaces()
{
  std::vector<hardware_interface::StateInterface> state_interfaces;

  // Wheels state
  state_interfaces.emplace_back(hardware_interface::StateInterface(
    wheel_l_.name, hardware_interface::HW_IF_POSITION, &wheel_l_.pos));
  state_interfaces.emplace_back(hardware_interface::StateInterface(
    wheel_l_.name, hardware_interface::HW_IF_VELOCITY, &wheel_l_.vel));

  state_interfaces.emplace_back(hardware_interface::StateInterface(
    wheel_r_.name, hardware_interface::HW_IF_POSITION, &wheel_r_.pos));
  state_interfaces.emplace_back(hardware_interface::StateInterface(
    wheel_r_.name, hardware_interface::HW_IF_VELOCITY, &wheel_r_.vel));

  // Lifter State
  state_interfaces.emplace_back(hardware_interface::StateInterface(
    cfg_.lifter_name, hardware_interface::HW_IF_POSITION, &lifter_position_state_));

  return state_interfaces;
}

std::vector<hardware_interface::CommandInterface> ArduinoSystemHardware::export_command_interfaces()
{
  std::vector<hardware_interface::CommandInterface> command_interfaces;

  // Wheels command
  command_interfaces.emplace_back(hardware_interface::CommandInterface(
    wheel_l_.name, hardware_interface::HW_IF_VELOCITY, &wheel_l_.cmd));

  command_interfaces.emplace_back(hardware_interface::CommandInterface(
    wheel_r_.name, hardware_interface::HW_IF_VELOCITY, &wheel_r_.cmd));

  // Lifter Command
  command_interfaces.emplace_back(hardware_interface::CommandInterface(
    cfg_.lifter_name, hardware_interface::HW_IF_POSITION, &lifter_position_command_));

  return command_interfaces;
}

hardware_interface::CallbackReturn ArduinoSystemHardware::on_configure(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  RCLCPP_INFO(rclcpp::get_logger("ArduinoSystemHardware"), "Configuring ...please wait...");
  if (comms_.connected())
  {
    comms_.disconnect();
  }
  comms_.connect(cfg_.device, cfg_.baud_rate, cfg_.timeout_ms);
  RCLCPP_INFO(rclcpp::get_logger("ArduinoSystemHardware"), "Successfully configured!");

  return hardware_interface::CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn ArduinoSystemHardware::on_cleanup(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  RCLCPP_INFO(rclcpp::get_logger("ArduinoSystemHardware"), "Cleaning up ...please wait...");
  if (comms_.connected())
  {
    comms_.disconnect();
  }
  RCLCPP_INFO(rclcpp::get_logger("ArduinoSystemHardware"), "Successfully cleaned up!");

  return hardware_interface::CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn ArduinoSystemHardware::on_activate(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  RCLCPP_INFO(rclcpp::get_logger("ArduinoSystemHardware"), "Activating ...please wait...");
  if (!comms_.connected())
  {
    return hardware_interface::CallbackReturn::ERROR;
  }
  
  // Wait for the Arduino bootloader to finish after DTR-triggered reset.
  // This sleep is intentionally placed in on_activate() (NOT on_configure())
  // so that DDS discovery completes before we block the executor.
  RCLCPP_INFO(rclcpp::get_logger("ArduinoSystemHardware"),
    "Waiting 2s for Arduino bootloader to complete after DTR reset...");
  comms_.wait_for_bootloader();

  // Reset internal states
  wheel_l_.pos = 0.0;
  wheel_r_.pos = 0.0;
  wheel_l_.enc = 0;
  wheel_r_.enc = 0;
  lifter_position_state_ = 0.0;
  lifter_position_command_ = 0.0;

  // Reset Hardware
  comms_.flush_buffers();   // Clear any bootloader garbage before first command
  comms_.reset_encoders();
  
  if (cfg_.pid_p > 0)
  {
    comms_.set_pid_values(cfg_.pid_p,cfg_.pid_d,cfg_.pid_i,cfg_.pid_o);
  }
  RCLCPP_INFO(rclcpp::get_logger("ArduinoSystemHardware"), "Successfully activated!");

  return hardware_interface::CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn ArduinoSystemHardware::on_deactivate(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  RCLCPP_INFO(rclcpp::get_logger("ArduinoSystemHardware"), "Deactivating ...please wait...");
  RCLCPP_INFO(rclcpp::get_logger("ArduinoSystemHardware"), "Successfully deactivated!");

  return hardware_interface::CallbackReturn::SUCCESS;
}

hardware_interface::return_type ArduinoSystemHardware::read(
  const rclcpp::Time & /*time*/, const rclcpp::Duration & period)
{
  if (!comms_.connected())
  {
    return hardware_interface::return_type::ERROR;
  }

  comms_.read_encoder_values(wheel_l_.enc, wheel_r_.enc);

  double delta_seconds = period.seconds();
  double pos_prev = wheel_l_.pos;
  wheel_l_.pos = wheel_l_.calc_enc_angle();
  wheel_l_.vel = (wheel_l_.pos - pos_prev) / delta_seconds;

  pos_prev = wheel_r_.pos;
  wheel_r_.pos = wheel_r_.calc_enc_angle();
  wheel_r_.vel = (wheel_r_.pos - pos_prev) / delta_seconds;

  // Simulation of smooth lifter movement in Rviz
  double simulated_speed = 0.05; 
  double step = simulated_speed * delta_seconds;

  if (lifter_position_state_ < lifter_position_command_) {
      lifter_position_state_ += step;
      if (lifter_position_state_ > lifter_position_command_) {
          lifter_position_state_ = lifter_position_command_;
      }
  } 
  else if (lifter_position_state_ > lifter_position_command_) {
      lifter_position_state_ -= step;
      if (lifter_position_state_ < lifter_position_command_) {
          lifter_position_state_ = lifter_position_command_;
      }
  }

  return hardware_interface::return_type::OK;
}

hardware_interface::return_type ArduinoSystemHardware::write(
  const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/)
{
  if (!comms_.connected())
  {
    return hardware_interface::return_type::ERROR;
  }

  int motor_l_counts_per_loop = wheel_l_.cmd / wheel_l_.rads_per_count / cfg_.loop_rate;
  int motor_r_counts_per_loop = wheel_r_.cmd / wheel_r_.rads_per_count / cfg_.loop_rate;
  comms_.set_motor_values(motor_l_counts_per_loop, motor_r_counts_per_loop);

  int lift_cmd = 0;
  // Threshold to determine direction (0.1 as per previous configuration)
  if (lifter_position_command_ > 0.1) { 
      lift_cmd = 1;
  } else {
      lift_cmd = 0;
  }
  comms_.set_lift_val(lift_cmd);

  return hardware_interface::return_type::OK;
}

}  // namespace arduino_hardware_interface

#include "pluginlib/class_list_macros.hpp"
PLUGINLIB_EXPORT_CLASS(
  arduino_hardware_interface::ArduinoSystemHardware, hardware_interface::SystemInterface)