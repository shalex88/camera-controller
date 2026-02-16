# Systemd Service Quick Start

## After Installation

Once the camera-controller is built and installed, follow these steps to set up the systemd service:

### Quick Install (Interactive)
```bash
cd ${INSTALL_ROOT}/camera-controller/systemd
sudo ./install-service.sh
```

### Quick Install (Automatic)
```bash
cd ${INSTALL_ROOT}/camera-controller/systemd
sudo ./install-service.sh --yes
```

### Manual Install
```bash
sudo cp ${INSTALL_ROOT}/camera-controller/systemd/camera-controller.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable camera-controller.service
sudo systemctl start camera-controller.service
```

## Common Commands

| Action | Command |
|--------|---------|
| Check status | `sudo systemctl status camera-controller` |
| Start service | `sudo systemctl start camera-controller` |
| Stop service | `sudo systemctl stop camera-controller` |
| Restart service | `sudo systemctl restart camera-controller` |
| Enable on boot | `sudo systemctl enable camera-controller` |
| Disable on boot | `sudo systemctl disable camera-controller` |
| View live logs | `sudo journalctl -u camera-controller -f` |
| View all logs | `sudo journalctl -u camera-controller` |

## Service Details

- **Default Config**: `config-wfov.yaml`
- **Binary Path**: `${INSTALL_ROOT}/camera-controller/camera-controller`
- **Config Path**: `${INSTALL_ROOT}/camera-controller/config/`
- **Service File**: `/etc/systemd/system/camera-controller.service`
- **Restart Policy**: Automatic restart on failure (5 second delay)

## Changing Configuration

To use a different configuration file:

1. Edit `/etc/systemd/system/camera-controller.service`
2. Change the `-c` parameter in the `ExecStart` line
3. Run `sudo systemctl daemon-reload`
4. Run `sudo systemctl restart camera-controller`

**Available configs**: `config-wfov.yaml`, `config-nfov.yaml`, `config-mwir.yaml`, `config-simulator.yaml`, `config.yaml`

## Uninstall

```bash
cd ${INSTALL_ROOT}/camera-controller/systemd
sudo ./install-service.sh --uninstall
```

Or manually:
```bash
sudo systemctl stop camera-controller
sudo systemctl disable camera-controller
sudo rm /etc/systemd/system/camera-controller.service
sudo systemctl daemon-reload
```

For more details, see [README.md](README.md)
