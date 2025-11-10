#!/bin/bash
# Deployment script for embedded Linux target
# Usage: ./deploy-embedded.sh <build-dir> [<staging-dir>] [<target-host>]
#   build-dir:     CMake build directory (default: cmake-build-debug-cc)
#   staging-dir:   Local staging directory (default: /tmp/camera-service-deploy)
#   target-host:   Optional. If provided, deploy via scp (e.g., root@192.168.1.100)

set -e

BUILD_DIR=${1:-cmake-build-debug-cc}
STAGING_DIR=${2:-/tmp/camera-service-deploy}
TARGET_HOST=${3:-}

TARGET_HOST=${3:-}

echo "=== Camera Service Embedded Deployment ==="
echo "Build directory: $BUILD_DIR"
echo "Staging directory: $STAGING_DIR"
if [ -n "$TARGET_HOST" ]; then
    echo "Target host: $TARGET_HOST (will deploy via scp)"
fi
echo ""

# Create target directory structure
mkdir -p "$STAGING_DIR/app"
mkdir -p "$STAGING_DIR/lib/genicam"
mkdir -p "$STAGING_DIR/config"

# Copy main executable
echo "Copying camera-service executable..."
cp "$BUILD_DIR/camera-service" "$STAGING_DIR/app/"

# Copy GenTL Producer library
echo "Copying GenTL Producer library..."
if [ -f "$BUILD_DIR/src/infrastructure/camera/protocol/gentl/libFpgaCXP.cti.1.0.0" ]; then
    cp "$BUILD_DIR/src/infrastructure/camera/protocol/gentl/libFpgaCXP.cti.1.0.0" "$STAGING_DIR/lib/genicam/"
    ln -sf libFpgaCXP.cti.1.0.0 "$STAGING_DIR/lib/genicam/libFpgaCXP.cti.1"
    ln -sf libFpgaCXP.cti.1 "$STAGING_DIR/lib/genicam/libFpgaCXP.cti"
    echo "  -> libFpgaCXP.cti deployed with symlinks"
else
    echo "ERROR: GenTL Producer library not found!"
    exit 1
fi

# Copy GenICam libraries (if needed)
GENICAM_LIBS=$(find "$BUILD_DIR" -name "*.so*" -path "*/genicam/lib/*" 2>/dev/null || true)
if [ -n "$GENICAM_LIBS" ]; then
    echo "Copying GenICam libraries..."
    cp -P $GENICAM_LIBS "$STAGING_DIR/lib/" 2>/dev/null || true
fi

# Copy configuration files
echo "Copying configuration files..."
cp config/config*.yaml "$STAGING_DIR/config/" 2>/dev/null || true

# Create startup script
echo "Creating startup script..."
cat > "$STAGING_DIR/app/start-camera-service.sh" << 'EOF'
#!/bin/sh
# Camera Service startup script for embedded target

# Get script directory
APP_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(dirname "$APP_DIR")"

# Set library path
export LD_LIBRARY_PATH="$ROOT_DIR/lib:$ROOT_DIR/lib/genicam:$LD_LIBRARY_PATH"

# Set GenTL Producer path (optional - application will auto-discover)
export GENTL_PRODUCER_PATH="$ROOT_DIR/lib/genicam/libFpgaCXP.cti"

# Change to app directory
cd "$APP_DIR"

# Run camera service
exec ./camera-service "$@"
EOF

chmod +x "$STAGING_DIR/app/start-camera-service.sh"

# Create README
cat > "$STAGING_DIR/README.txt" << 'EOF'
Camera Service Deployment Package
==================================

Directory Structure:
  app/                  - Application binaries
    camera-service      - Main executable
    start-camera-service.sh - Startup script
  lib/                  - Shared libraries
    genicam/            - GenTL Producer library
      libFpgaCXP.cti    - CoaXPress GenTL Producer
  config/               - Configuration files
    config-*.yaml       - Camera configurations

Installation:
1. Copy entire directory to target device (e.g., /home/root/camera-service)
2. Ensure execution permissions:
   chmod +x app/camera-service app/start-camera-service.sh
3. Run using startup script:
   cd /home/root/camera-service
   ./app/start-camera-service.sh --config config/config-adimec-gentl.yaml

Environment Variables:
  GENTL_PRODUCER_PATH   - Override GenTL Producer location
  LD_LIBRARY_PATH       - Library search path (set by startup script)

Troubleshooting:
- If "libFpgaCXP.cti.1: cannot open shared object file":
  -> Check LD_LIBRARY_PATH includes lib/genicam directory
  -> Verify libFpgaCXP.cti exists and has correct permissions
  -> Use startup script which sets paths automatically

- If "GenTL producer library not found":
  -> Set GENTL_PRODUCER_PATH to full path of libFpgaCXP.cti
  -> Or ensure it's in lib/genicam/ relative to executable

For more information, see docs/Using_GenTL_Camera.md
EOF

# Show deployment summary
echo ""
echo "=== Deployment Package Ready ==="
echo "Staging directory: $STAGING_DIR"
echo ""
echo "Contents:"
ls -lh "$STAGING_DIR/app/" "$STAGING_DIR/lib/genicam/"

# Deploy via scp if target host provided
if [ -n "$TARGET_HOST" ]; then
    echo ""
    echo "=== Deploying to $TARGET_HOST via scp ==="
    
    # Create remote directory structure
    echo "Creating remote directories..."
    ssh "$TARGET_HOST" "mkdir -p /home/root/camera-service/{app,lib/genicam,config}"
    
    # Copy files
    echo "Copying files to target..."
    scp -r "$STAGING_DIR"/* "$TARGET_HOST:/home/root/camera-service/"
    
    # Set permissions on target
    echo "Setting permissions on target..."
    ssh "$TARGET_HOST" "chmod +x /home/root/camera-service/app/camera-service /home/root/camera-service/app/start-camera-service.sh"
    
    echo ""
    echo "=== Deployment Complete ==="
    echo "Files deployed to: $TARGET_HOST:/home/root/camera-service/"
    echo ""
    echo "To run on target device:"
    echo "  ssh $TARGET_HOST"
    echo "  cd /home/root/camera-service"
    echo "  ./app/start-camera-service.sh --config config/config-adimec-gentl.yaml"
else
    echo ""
    echo "=== Manual Deployment Instructions ==="
    echo "To deploy to target device via scp:"
    echo "  scp -r $STAGING_DIR/* root@<target-ip>:/home/root/camera-service/"
    echo ""
    echo "Or run this script with target host:"
    echo "  $0 $BUILD_DIR $STAGING_DIR root@<target-ip>"
    echo ""
    echo "On target device:"
    echo "  ssh root@<target-ip>"
    echo "  cd /home/root/camera-service"
    echo "  chmod +x app/camera-service app/start-camera-service.sh"
    echo "  ./app/start-camera-service.sh --config config/config-adimec-gentl.yaml"
fi
