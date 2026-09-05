FROM osrf/ros:humble-desktop-full

# 1. Install Basic Tools AND Robot Dependencies
RUN apt-get update && apt-get install -y \
    nano \
    vim \
    git \
    python3-pip \
    libserial-dev \
    # Robot Description & Control
    ros-humble-xacro \
    ros-humble-ros2-control \
    ros-humble-ros2-controllers \
    ros-humble-gazebo-ros2-control \
    # Hardware Interface Dependencies
    ros-humble-hardware-interface \
    ros-humble-controller-manager \
    ros-humble-pluginlib \
    # Navigation & Slam
    ros-humble-slam-toolbox \
    ros-humble-navigation2 \
    ros-humble-nav2-bringup \
    ros-humble-twist-mux \
    && rm -rf /var/lib/apt/lists/*

RUN apt-get update && apt-get install -y \
    ros-humble-rplidar-ros \
    ros-humble-gazebo-ros-pkgs \
    ros-humble-gazebo-plugins \
    && rm -rf /var/lib/apt/lists/*

# Install Joint State Publisher GUI
RUN apt-get update && apt-get install -y \
    ros-humble-joint-state-publisher-gui \
    && rm -rf /var/lib/apt/lists/*

# Install Python packages
RUN pip3 install pyserial

# Install I2C tools and dependencies for MPU6050 (Python & C++)
RUN apt-get update && apt-get install -y \
    python3-smbus \
    i2c-tools \
    libi2c-dev \
    && rm -rf /var/lib/apt/lists/*

# 2. Create a non-root user and add to dialout (CRITICAL for Arduino)
ARG USERNAME=ros
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN groupadd --gid $USER_GID $USERNAME \
  && useradd -s /bin/bash --uid $USER_UID --gid $USER_GID -m $USERNAME \
  && usermod -aG dialout $USERNAME \
  && mkdir /home/$USERNAME/.config && chown $USER_UID:$USER_GID /home/$USERNAME/.config

# 3. Set up sudo
RUN apt-get update \
  && apt-get install -y sudo \
  && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME\
  && chmod 0440 /etc/sudoers.d/$USERNAME \
  && rm -rf /var/lib/apt/lists/*

# 4. Copy scripts
COPY entrypoint.sh /entrypoint.sh
COPY bashrc /tmp/bashrc_append
RUN chmod +x /entrypoint.sh

# 5. Configure User Environment
# Append custom bashrc to the real .bashrc
RUN cat /tmp/bashrc_append >> /home/$USERNAME/.bashrc && rm /tmp/bashrc_append

# Create Workspace folder with correct permissions
RUN mkdir -p /home/$USERNAME/ros2_ws/src \
    && chown -R $USER_UID:$USER_GID /home/$USERNAME/ros2_ws

# 6. Switch User
USER $USERNAME
WORKDIR /home/$USERNAME/ros2_ws

ENTRYPOINT ["/entrypoint.sh"]
CMD ["bash"]