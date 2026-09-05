import os
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():
    pkg_name = 'warehouse_real_bringup'

    lidar_config_file = os.path.join(
        get_package_share_directory(pkg_name),
        'config',
        'laser_filter_config.yaml'
    )
    
    serial_port = LaunchConfiguration('serial_port')

    serial_port_arg = DeclareLaunchArgument(
        'serial_port',
        default_value='/dev/ttyUSB0',
        description='Serial port for RPLidar'
    )

    return LaunchDescription([
        serial_port_arg,

        # Driver Node
        Node(
            package='rplidar_ros',
            executable='rplidar_composition',
            output='screen',
            parameters=[{
                'serial_port': serial_port,
                'serial_baudrate': 115200,
                'frame_id': 'laser_frame',
                'angle_compensate': True,
                'scan_mode': 'Standard'
            }],
            remappings=[
                ('scan', 'scan_raw')
            ]
        ),

        # Filter Node
        Node(
            package='laser_filters',
            executable='scan_to_scan_filter_chain',
            parameters=[lidar_config_file],
            remappings=[
                ('scan', 'scan_raw'),
                ('scan_filtered', 'scan')
            ]
        )
    ])