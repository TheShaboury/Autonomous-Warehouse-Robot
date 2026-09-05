#include "mpu6050driver/mpu6050driver.h"

#include <chrono>
#include <memory>

using namespace std::chrono_literals;

MPU6050Driver::MPU6050Driver()
    : Node("mpu6050publisher"), mpu6050_{std::make_unique<MPU6050Sensor>()}
{
  // Declare parameters
  declareParameters();
  // Set parameters
  mpu6050_->setGyroscopeRange(
      static_cast<MPU6050Sensor::GyroRange>(this->get_parameter("gyro_range").as_int()));
  mpu6050_->setAccelerometerRange(
      static_cast<MPU6050Sensor::AccelRange>(this->get_parameter("accel_range").as_int()));
  mpu6050_->setDlpfBandwidth(
      static_cast<MPU6050Sensor::DlpfBandwidth>(this->get_parameter("dlpf_bandwidth").as_int()));
  mpu6050_->setGyroscopeOffset(this->get_parameter("gyro_x_offset").as_double(),
                               this->get_parameter("gyro_y_offset").as_double(),
                               this->get_parameter("gyro_z_offset").as_double());
  mpu6050_->setAccelerometerOffset(this->get_parameter("accel_x_offset").as_double(),
                                   this->get_parameter("accel_y_offset").as_double(),
                                   this->get_parameter("accel_z_offset").as_double());
  // Check if we want to calibrate the sensor
  if (this->get_parameter("calibrate").as_bool()) {
    RCLCPP_INFO(this->get_logger(), "Calibrating...");
    mpu6050_->calibrate();
  }
  mpu6050_->printConfig();
  mpu6050_->printOffsets();
  // Create publisher
  publisher_ = this->create_publisher<sensor_msgs::msg::Imu>("imu", 10);
  int freq_hz = this->get_parameter("frequency").as_int();
  if (freq_hz <= 0) freq_hz = 100;  // Safe default
  auto period = std::chrono::milliseconds(1000 / freq_hz);
  timer_ = this->create_wall_timer(period, std::bind(&MPU6050Driver::handleInput, this));
}

void MPU6050Driver::handleInput()
{
  auto message = sensor_msgs::msg::Imu();
  message.header.stamp = this->get_clock()->now();
  message.header.frame_id = "imu_link";
  // MPU6050 ±2g range: noise density ~400 µg/√Hz, at 94Hz BW ≈ 0.038 m/s²
  // Variance ≈ (0.038)² ≈ 0.0014, use 0.01 for margin
  message.linear_acceleration_covariance = {
    0.01, 0.0, 0.0,
    0.0, 0.01, 0.0,
    0.0, 0.0, 0.01};
  message.linear_acceleration.x = mpu6050_->getAccelerationX();
  message.linear_acceleration.y = mpu6050_->getAccelerationY();
  message.linear_acceleration.z = mpu6050_->getAccelerationZ();
  // MPU6050 ±250°/s range: noise density ~0.005 °/s/√Hz at 94Hz BW
  // ≈ 8.5e-4 rad/s → variance ≈ 7.2e-7, use 0.001 for margin
  message.angular_velocity_covariance = {
    0.001, 0.0, 0.0,
    0.0, 0.001, 0.0,
    0.0, 0.0, 0.001};
  message.angular_velocity.x = mpu6050_->getAngularVelocityX();
  message.angular_velocity.y = mpu6050_->getAngularVelocityY();
  message.angular_velocity.z = mpu6050_->getAngularVelocityZ();
  // Invalidate quaternion (MPU6050 has no magnetometer, cannot provide orientation)
  message.orientation_covariance[0] = -1;
  message.orientation.x = 0;
  message.orientation.y = 0;
  message.orientation.z = 0;
  message.orientation.w = 0;
  publisher_->publish(message);
}

void MPU6050Driver::declareParameters()
{
  this->declare_parameter<bool>("calibrate", true);
  this->declare_parameter<int>("gyro_range", MPU6050Sensor::GyroRange::GYR_250_DEG_S);
  this->declare_parameter<int>("accel_range", MPU6050Sensor::AccelRange::ACC_2_G);
  this->declare_parameter<int>("dlpf_bandwidth", MPU6050Sensor::DlpfBandwidth::DLPF_260_HZ);
  this->declare_parameter<double>("gyro_x_offset", 0.0);
  this->declare_parameter<double>("gyro_y_offset", 0.0);
  this->declare_parameter<double>("gyro_z_offset", 0.0);
  this->declare_parameter<double>("accel_x_offset", 0.0);
  this->declare_parameter<double>("accel_y_offset", 0.0);
  this->declare_parameter<double>("accel_z_offset", 0.0);
  this->declare_parameter<int>("frequency", 100);
}

int main(int argc, char* argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<MPU6050Driver>());
  rclcpp::shutdown();
  return 0;
}