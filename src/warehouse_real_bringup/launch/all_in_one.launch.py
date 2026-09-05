import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource

def generate_launch_description():
    # 1. Get package directories
    bringup_pkg = get_package_share_directory('warehouse_real_bringup')
    nav_pkg = get_package_share_directory('warehouse_real_nav')

    # 2. Define launch file paths
    launch_robot_path = os.path.join(bringup_pkg, 'launch', 'launch_robot.launch.py')
    rplidar_path = os.path.join(bringup_pkg, 'launch', 'rplidar.launch.py')
    sensor_fusion_path = os.path.join(nav_pkg, 'launch', 'sensor_fusion.launch.py')

    # 3. Create IncludeLaunchDescription actions
    launch_robot_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(launch_robot_path)
    )

    rplidar_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(rplidar_path)
    )

    sensor_fusion_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(sensor_fusion_path)
    )

    # 4. Return the LaunchDescription
    return LaunchDescription([
        launch_robot_cmd,
        rplidar_cmd,
        sensor_fusion_cmd
    ])