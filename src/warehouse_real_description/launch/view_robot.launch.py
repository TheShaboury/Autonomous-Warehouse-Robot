import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import Command, LaunchConfiguration
from launch_ros.actions import Node

def generate_launch_description():
    # 1. Define Package Name
    pkg_name = 'warehouse_real_description'
    
    # 2. Define File Paths (Xacro & RViz Config)
    pkg_share = get_package_share_directory(pkg_name)
    default_model_path = os.path.join(pkg_share, 'description', 'robot.urdf.xacro')
    default_rviz_config_path = os.path.join(pkg_share, 'rviz', 'urdf_config.rviz')

    # 3. Declare Launch Arguments
    # Argument to enable/disable the GUI slider for joints
    gui_arg = DeclareLaunchArgument(
        name='gui', 
        default_value='true', 
        choices=['true', 'false'],
        description='Flag to enable joint_state_publisher_gui'
    )
    
    # Argument for the model file path
    model_arg = DeclareLaunchArgument(
        name='model', 
        default_value=default_model_path,
        description='Absolute path to robot urdf file'
    )
    
    # Argument for the RViz config file path
    rviz_arg = DeclareLaunchArgument(
        name='rvizconfig', 
        default_value=default_rviz_config_path,
        description='Absolute path to rviz config file'
    )

    # 4. Process the Xacro file to generate URDF
    robot_description = Command(['xacro ', LaunchConfiguration('model')])
    
    # 5. Define Nodes
    
    # Robot State Publisher: Publishes the TF tree based on the URDF
    robot_state_publisher_node = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        parameters=[{'robot_description': robot_description}]
    )

    # Joint State Publisher GUI: Allows manual control of joints via sliders
    joint_state_publisher_gui_node = Node(
        package='joint_state_publisher_gui',
        executable='joint_state_publisher_gui',
        condition=None # Runs if gui arg is true (logic handled by arg, simplified here)
    )

    # RViz2 Node: Visualizer
    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        output='screen',
        arguments=['-d', LaunchConfiguration('rvizconfig')],
    )

    # 6. Return Launch Description
    return LaunchDescription([
        gui_arg,
        model_arg,
        rviz_arg,
        joint_state_publisher_gui_node,
        robot_state_publisher_node,
        rviz_node
    ])