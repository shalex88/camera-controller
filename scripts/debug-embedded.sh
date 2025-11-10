#!/bin/bash
# Debug script for embedded target with gdbserver
# Usage: ./debug-embedded.sh <target-host> [port] [config-file]

TARGET_HOST=${1:-root@172.25.125.142}
DEBUG_PORT=${2:-1234}
CONFIG_FILE=${3:-config-adimec-gentl.yaml}

REMOTE_DIR="/home/root/camera-service"

echo "=== Starting remote debug session ==="
echo "Target: $TARGET_HOST"
echo "Debug port: $DEBUG_PORT"
echo "Config: $CONFIG_FILE"
echo ""
echo "Connecting to gdbserver on $TARGET_HOST:$DEBUG_PORT..."
echo "In your IDE, set remote target to: $TARGET_HOST:$DEBUG_PORT"
echo ""

# Start gdbserver with correct library path
ssh "$TARGET_HOST" << EOF
cd $REMOTE_DIR
export LD_LIBRARY_PATH=$REMOTE_DIR/lib/genicam:$REMOTE_DIR/lib:\$LD_LIBRARY_PATH
/usr/bin/gdbserver :$DEBUG_PORT $REMOTE_DIR/app/camera-service -c config/$CONFIG_FILE
EOF
