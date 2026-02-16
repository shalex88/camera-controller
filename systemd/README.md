# Systemd Service for Camera Controller

## Overview

This directory contains the systemd service unit file for the camera-controller application. The service is configured to run the camera controller with the WFOV (Wide Field of View) configuration by default.

## Service Configuration

The service file (`camera-controller.service.in`) is a template that gets configured during the CMake build process. The following variables are substituted:

- `@PROJECT_NAME@` - The project name (camera-controller)
- `@PROJECT_INSTALL_BINDIR@` - The installation directory for the binary
- `@PROJECT_INSTALL_CONFIGDIR@` - The installation directory for configuration files

## Installation

After building and installing the camera-controller, the systemd service file will be available at:
```
${INSTALL_ROOT}/camera-controller/systemd/camera-controller.service
```

### Manual Installation Steps

1. Copy the service file to the systemd system directory:
   ```bash
   sudo cp ${INSTALL_ROOT}/camera-controller/systemd/camera-controller.service /etc/systemd/system/
   ```

2. Reload systemd to recognize the new service:
   ```bash
   sudo systemctl daemon-reload
   ```

3. Enable the service to start on boot (optional):
   ```bash
   sudo systemctl enable camera-controller.service
   ```

4. Start the service:
   ```bash
   sudo systemctl start camera-controller.service
   ```

## Service Management

### Check service status:
```bash
sudo systemctl status camera-controller.service
```

### View service logs:
```bash
# Real-time logs
sudo journalctl -u camera-controller.service -f

# All logs
sudo journalctl -u camera-controller.service

# Logs since boot
sudo journalctl -u camera-controller.service -b
```

### Stop the service:
```bash
sudo systemctl stop camera-controller.service
```

### Restart the service:
```bash
sudo systemctl restart camera-controller.service
```

### Disable auto-start on boot:
```bash
sudo systemctl disable camera-controller.service
```

## Configuration

The service uses the `config-wfov.yaml` configuration file by default. To use a different configuration:

1. Edit the service file at `/etc/systemd/system/camera-controller.service`
2. Modify the `ExecStart` line to point to a different config file
3. Reload the systemd configuration: `sudo systemctl daemon-reload`
4. Restart the service: `sudo systemctl restart camera-controller.service`

### Available configurations:
- `config-wfov.yaml` - Wide Field of View
- `config-nfov.yaml` - Narrow Field of View
- `config-mwir.yaml` - Mid-Wave Infrared
- `config-simulator.yaml` - Simulator mode
- `config.yaml` - Default configuration

## Security Features

The service includes several security hardening features:

- **NoNewPrivileges**: Prevents the process from gaining new privileges
- **PrivateTmp**: Provides a private /tmp directory
- **ProtectSystem=strict**: Makes most of the file system read-only
- **ProtectHome**: Prevents access to home directories
- **ReadWritePaths**: Explicitly allows writing to /tmp

## Resource Limits

- **LimitNOFILE**: 65536 (maximum number of open files)
- **LimitNPROC**: 4096 (maximum number of processes)

## Running as a Specific User

By default, the service runs as root. To run as a specific user:

1. Create a dedicated user (if not already exists):
   ```bash
   sudo useradd -r -s /bin/false camera
   ```

2. Edit the service file and uncomment/modify:
   ```ini
   User=camera
   Group=camera
   ```

3. Ensure the user has appropriate permissions:
   ```bash
   sudo chown -R camera:camera ${INSTALL_ROOT}/camera-controller
   ```

4. Reload and restart the service:
   ```bash
   sudo systemctl daemon-reload
   sudo systemctl restart camera-controller.service
   ```

## Troubleshooting

### Service fails to start

1. Check the service status for error messages:
   ```bash
   sudo systemctl status camera-controller.service
   ```

2. View detailed logs:
   ```bash
   sudo journalctl -u camera-controller.service -n 50 --no-pager
   ```

3. Verify the binary exists and is executable:
   ```bash
   ls -la ${INSTALL_ROOT}/camera-controller/camera-controller
   ```

4. Verify the configuration file exists:
   ```bash
   ls -la ${INSTALL_ROOT}/camera-controller/config/config-wfov.yaml
   ```

### Permission Issues

If you encounter permission errors:
- Check file ownership and permissions
- Verify SELinux/AppArmor policies (if applicable)
- Review the ReadWritePaths directive in the service file

### Camera Access Issues

If the service can't access camera devices:
- Add the service user to the `video` group
- Adjust udev rules for camera device permissions
- Modify the ProtectSystem and ProtectHome directives if needed
