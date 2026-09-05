# 🤖 AMR Jetson Core System

The core ROS 2 software stack for an **Autonomous Mobile Robot (AMR)**, running on an **NVIDIA Jetson** and controlled through an **Arduino Uno**. It handles robot description, low-level motor control, sensor fusion, SLAM, and Nav2-based autonomous navigation, and ships fully Dockerized for both desktop development and Jetson deployment.

This is the real-hardware counterpart of a Mechatronics Engineering graduation project on warehouse AMRs — the packages here (`warehouse_real_*`) target the physical robot rather than simulation.

---

## 🚀 What It Does

- Describes the robot (URDF/xacro) and exposes it to `ros2_control`
- Drives the physical robot through a custom **Arduino hardware interface** talking to `ros_arduino_bridge` firmware over serial
- Fuses **IMU (MPU6050)** and wheel odometry with an **Extended Kalman Filter** (`robot_localization`) for a more reliable state estimate
- Builds a map of the environment with **SLAM Toolbox**
- Localizes on a saved map with **AMCL**
- Plans and executes paths with **Nav2** (global + local planning, recovery behaviors)
- Reads the environment with an **RPLIDAR A1**
- Runs identically in a dev container on a normal PC or in a Jetson-specific container on the robot itself

---

## 🏗️ System Architecture

```text
                    ┌─────────────────────────┐
                    │        Joystick /        │
                    │      Nav2 Goal / Teleop   │
                    └────────────┬─────────────┘
                                 │
                    ┌────────────▼────────────┐
                    │           ROS 2          │
                    │                          │
                    │ SLAM Toolbox │ AMCL │    │
                    │ Nav2 │ EKF (robot_       │
                    │ localization)            │
                    └────────────┬─────────────┘
                                 │
                    ┌────────────▼────────────┐
                    │       ros2_control       │
                    │ arduino_hardware_interface│
                    └────────────┬─────────────┘
                                 │  Serial (ros_arduino_bridge)
                    ┌────────────▼────────────┐
                    │        Arduino Uno       │
                    │     Low-Level Control    │
                    └────────────┬─────────────┘
                                 │
                    ┌────────────▼────────────┐
                    │ Motors / Encoders / I/O  │
                    └──────────────────────────┘
```

### High-Level Computing — NVIDIA Jetson

Runs the full ROS 2 stack: robot state publishing, sensor fusion, SLAM, localization, navigation, and planning.

### Low-Level Control — Arduino Uno

Handles motor control and encoder feedback. The custom `arduino_hardware_interface` package plugs into `ros2_control` as a `SystemInterface` and talks to the Arduino over serial via `ros_arduino_bridge`-style firmware, so the navigation stack never talks to hardware directly.

---

## 📡 Sensor Fusion

```text
       Wheel Encoders ──────┐
                            │
       MPU6050 (IMU) ───────┼──► EKF (robot_localization) ──► /odometry/filtered
                            │
       (RPLIDAR feeds SLAM/AMCL directly, not the EKF)
```

- `mpu6050driver` reads the IMU over I2C and calibrates on startup
- `robot_localization`'s `ekf_node` fuses IMU + wheel odometry using `warehouse_real_nav/config/ekf.yaml`
- RPLIDAR A1 scans feed SLAM Toolbox (mapping) and AMCL/Nav2 (localization + costmaps) directly

---

## 🧰 Technology Stack

| Category                   | Technologies                                  |
| --------------------------- | ---------------------------------------------- |
| Robotics Middleware          | ROS 2 Humble                                   |
| Navigation                   | Navigation2 (Nav2), AMCL                       |
| SLAM                         | SLAM Toolbox                                   |
| State Estimation             | Extended Kalman Filter (`robot_localization`)  |
| Motor Control Framework      | `ros2_control` + custom Arduino hardware interface |
| High-Level Computer          | NVIDIA Jetson                                  |
| Low-Level Controller         | Arduino Uno (`ros_arduino_bridge`-style firmware) |
| LiDAR                        | RPLIDAR A1                                     |
| IMU                          | MPU6050                                        |
| Velocity Arbitration         | `twist_mux`                                    |
| Teleop                       | Joystick (`joystick.launch.py`)                |
| Simulation (robot model)     | Gazebo (`launch_sim.launch.py`, xacro/URDF)    |
| Programming                  | C++, Python                                    |
| Deployment                   | Docker / Docker Compose                        |
| Version Control              | Git                                             |

---

## 📁 Repository Structure

```text
AMR-Jetson-Core-System/
│
├── src/
│   ├── arduino_hardware_interface/   # ros2_control SystemInterface for the Arduino
│   ├── mpu6050driver/                # IMU driver (I2C, calibration, publishes sensor_msgs/Imu)
│   ├── serial/                       # Serial communication library (Arduino <-> Jetson)
│   ├── warehouse_real_bringup/       # Top-level launch files, controllers, twist_mux, joystick
│   ├── warehouse_real_description/   # URDF/xacro robot model, meshes, Gazebo config
│   ├── warehouse_real_nav/           # EKF, AMCL localization, Nav2 navigation launch/config
│   └── warehouse_real_slam/          # SLAM Toolbox config + saved maps
│
├── Dockerfile                        # Desktop development image (ROS 2 desktop-full)
├── Dockerfile.jetson                 # Jetson deployment image (ROS 2 base, privileged hardware access)
├── docker-compose.yaml               # Dev container (Gazebo/RViz over X11)
├── docker-compose.jetson.yaml        # Jetson container (privileged, full /dev passthrough)
├── entrypoint.sh                     # Fixes device permissions (i2c/ttyUSB/ttyACM) on startup
└── bashrc                            # Appended shell config inside the container
```

---

## 💻 Getting Started

### Prerequisites

For development (PC):
- Ubuntu
- Docker + Docker Compose
- Git

For running on the physical robot:
- NVIDIA Jetson (flashed with a compatible Ubuntu/L4T image)
- Arduino Uno running `ros_arduino_bridge`-compatible firmware
- RPLIDAR A1
- MPU6050 IMU
- Wheel encoders + differential drive base

### Clone the repository

```bash
git clone https://github.com/TheShaboury/AMR-Jetson-Core-System.git
cd AMR-Jetson-Core-System
```

### Run the desktop development container

```bash
docker compose build
docker compose up
```

This uses `Dockerfile` (`osrf/ros:humble-desktop-full`) with X11 passthrough for Gazebo/RViz.

### Run on the Jetson

```bash
docker compose -f docker-compose.jetson.yaml build
docker compose -f docker-compose.jetson.yaml up
```

This uses `Dockerfile.jetson` (`ros:humble-ros-base`), runs `privileged: true` with `/dev` mounted so the container can access `i2c`, `ttyUSB`/`ttyACM`, and other hardware devices. `entrypoint.sh` fixes device permissions on every container start.

> Update the Arduino serial port (`ttyUSB0`/`ttyACM0`) and I2C bus in the relevant config files to match your wiring.

### Launch the stack (inside the container)

Bring up the robot description, controllers, and twist_mux:

```bash
ros2 launch warehouse_real_bringup launch_robot.launch.py
```

Bring up robot + RPLIDAR + IMU/EKF sensor fusion together:

```bash
ros2 launch warehouse_real_bringup all_in_one.launch.py
```

Build a map (SLAM):

```bash
ros2 launch warehouse_real_slam online_async_launch.py
```

Localize on a saved map and navigate (Nav2):

```bash
ros2 launch warehouse_real_nav real_nav.launch.xml
```

---

## 🗺️ Maps

Saved maps live in `src/warehouse_real_slam/maps/` (`.pgm`/`.png` + `.yaml`), generated with SLAM Toolbox and reused by AMCL for localization.

---

## 👨‍💻 Contributions

Focus areas in this repository:

- Designing the ROS 2 software architecture for the real robot
- Building the custom `ros2_control` Arduino hardware interface
- Integrating IMU + odometry sensor fusion with an EKF
- Integrating SLAM Toolbox and AMCL-based localization
- Integrating the Nav2 navigation stack
- Containerizing both the development and Jetson deployment environments

---

## 🔮 Future Development

- Warehouse task/order state machine on top of the navigation stack
- Computer vision and object detection
- Multi-robot coordination
- Learning-based navigation (simulation-to-real)

---

## 📜 License

Intended primarily for educational and research purposes. See individual package directories (e.g. `serial`, `arduino_hardware_interface`) for third-party licensing where applicable.
