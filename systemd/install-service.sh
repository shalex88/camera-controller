#!/bin/bash
# Install systemd service for camera-controller

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Default values
INSTALL_ROOT="${INSTALL_ROOT:-/tmp}"
PROJECT_NAME="camera-controller"
SERVICE_FILE="${INSTALL_ROOT}/${PROJECT_NAME}/systemd/${PROJECT_NAME}.service"
SYSTEMD_DIR="/etc/systemd/system"

# Function to print colored messages
print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check if running as root
check_root() {
    if [ "$EUID" -ne 0 ]; then
        print_error "This script must be run as root (use sudo)"
        exit 1
    fi
}

# Function to verify service file exists
check_service_file() {
    if [ ! -f "$SERVICE_FILE" ]; then
        print_error "Service file not found: $SERVICE_FILE"
        print_info "Make sure camera-controller is built and installed first"
        exit 1
    fi
}

# Function to backup existing service file
backup_service() {
    local target="${SYSTEMD_DIR}/${PROJECT_NAME}.service"
    if [ -f "$target" ]; then
        local backup="${target}.backup.$(date +%Y%m%d_%H%M%S)"
        print_warning "Existing service file found, backing up to: $backup"
        cp "$target" "$backup"
    fi
}

# Function to install service
install_service() {
    local target="${SYSTEMD_DIR}/${PROJECT_NAME}.service"
    
    print_info "Installing service file to: $target"
    cp "$SERVICE_FILE" "$target"
    chmod 644 "$target"
    
    print_info "Reloading systemd daemon..."
    systemctl daemon-reload
    
    print_info "Service installed successfully!"
}

# Function to enable and start service
enable_start_service() {
    read -p "Do you want to enable the service to start on boot? [y/N] " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        print_info "Enabling service..."
        systemctl enable ${PROJECT_NAME}.service
    fi
    
    read -p "Do you want to start the service now? [y/N] " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        print_info "Starting service..."
        systemctl start ${PROJECT_NAME}.service
        sleep 2
        print_info "Service status:"
        systemctl status ${PROJECT_NAME}.service --no-pager
    fi
}

# Function to show usage
usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Install systemd service for camera-controller

OPTIONS:
    -h, --help              Show this help message
    -r, --root DIR          Set INSTALL_ROOT directory (default: /tmp)
    -y, --yes               Enable and start service without prompting
    -n, --no-start          Install only, don't enable or start
    --uninstall             Uninstall the service

EXAMPLES:
    # Install with default settings
    sudo $0

    # Install with custom install root
    sudo $0 --root /opt

    # Install and automatically enable/start
    sudo $0 --yes

    # Uninstall the service
    sudo $0 --uninstall

EOF
}

# Function to uninstall service
uninstall_service() {
    local target="${SYSTEMD_DIR}/${PROJECT_NAME}.service"
    
    print_info "Uninstalling camera-controller service..."
    
    # Check if service is running
    if systemctl is-active --quiet ${PROJECT_NAME}.service; then
        print_info "Stopping service..."
        systemctl stop ${PROJECT_NAME}.service
    fi
    
    # Check if service is enabled
    if systemctl is-enabled --quiet ${PROJECT_NAME}.service 2>/dev/null; then
        print_info "Disabling service..."
        systemctl disable ${PROJECT_NAME}.service
    fi
    
    # Remove service file
    if [ -f "$target" ]; then
        print_info "Removing service file..."
        rm "$target"
    else
        print_warning "Service file not found: $target"
    fi
    
    # Reload systemd
    print_info "Reloading systemd daemon..."
    systemctl daemon-reload
    
    print_info "Service uninstalled successfully!"
}

# Parse command line arguments
AUTO_ENABLE=false
NO_START=false
UNINSTALL=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            usage
            exit 0
            ;;
        -r|--root)
            INSTALL_ROOT="$2"
            SERVICE_FILE="${INSTALL_ROOT}/${PROJECT_NAME}/systemd/${PROJECT_NAME}.service"
            shift 2
            ;;
        -y|--yes)
            AUTO_ENABLE=true
            shift
            ;;
        -n|--no-start)
            NO_START=true
            shift
            ;;
        --uninstall)
            UNINSTALL=true
            shift
            ;;
        *)
            print_error "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

# Main execution
print_info "Camera Controller Systemd Service Installer"
print_info "==========================================="

check_root

if [ "$UNINSTALL" = true ]; then
    uninstall_service
    exit 0
fi

check_service_file
backup_service
install_service

if [ "$NO_START" = false ]; then
    if [ "$AUTO_ENABLE" = true ]; then
        print_info "Enabling service..."
        systemctl enable ${PROJECT_NAME}.service
        print_info "Starting service..."
        systemctl start ${PROJECT_NAME}.service
        sleep 2
        print_info "Service status:"
        systemctl status ${PROJECT_NAME}.service --no-pager || true
    else
        enable_start_service
    fi
fi

print_info ""
print_info "Installation complete!"
print_info ""
print_info "Useful commands:"
print_info "  View status:  sudo systemctl status ${PROJECT_NAME}.service"
print_info "  View logs:    sudo journalctl -u ${PROJECT_NAME}.service -f"
print_info "  Start:        sudo systemctl start ${PROJECT_NAME}.service"
print_info "  Stop:         sudo systemctl stop ${PROJECT_NAME}.service"
print_info "  Restart:      sudo systemctl restart ${PROJECT_NAME}.service"
print_info "  Enable:       sudo systemctl enable ${PROJECT_NAME}.service"
print_info "  Disable:      sudo systemctl disable ${PROJECT_NAME}.service"
