#!/bin/bash

set -e

# --- Automated Device Permissions ---
# Fix permissions for hardware devices on every container startup.
# Requires: privileged: true and /dev:/dev volume mount in docker-compose.
# Errors are suppressed per-glob so missing device types don't abort the script.
sudo chmod a+rw /dev/i2c-*    2>/dev/null || true
sudo chmod a+rw /dev/ttyUSB*  2>/dev/null || true
sudo chmod a+rw /dev/ttyACM*  2>/dev/null || true

source /opt/ros/humble/setup.bash

# If you built your workspace, you might want to source it automatically too:
# if [ -f /home/ros/ros2_ws/install/setup.bash ]; then
#   source /home/ros/ros2_ws/install/setup.bash
# fi

echo "Provided arguments: $@"

exec $@