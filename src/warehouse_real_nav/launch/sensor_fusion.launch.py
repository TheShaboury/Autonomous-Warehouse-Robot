import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    
    package_name = 'warehouse_real_nav'
    imu_pkg = 'mpu6050driver'

    # 1. IMU Driver Node
    # Load the full mpu6050.yaml for runtime control over calibration,
    # DLPF bandwidth, gyro/accel ranges, and offset parameters.
    imu_params_path = os.path.join(
        get_package_share_directory(imu_pkg),
        'params',
        'mpu6050.yaml'
    )

    mpu6050_driver = Node(
        package=imu_pkg,
        executable='mpu6050driver',
        name='mpu6050driver_node',
        output='screen',
        emulate_tty=True,
        parameters=[imu_params_path]
    )

    # 2. EKF Node (Robot Localization)
    ekf_config_path = os.path.join(
        get_package_share_directory(package_name),
        'config',
        'ekf.yaml'
    )

    robot_localization_node = Node(
        package='robot_localization',
        executable='ekf_node',
        name='ekf_filter_node',
        output='screen',
        parameters=[ekf_config_path]
    )

    return LaunchDescription([
        mpu6050_driver,
        robot_localization_node
    ])