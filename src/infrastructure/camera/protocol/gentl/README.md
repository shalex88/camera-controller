# FPGA CoaXPress GenTL Producer

This directory contains a complete **GenTL (Generic Transport Layer) Producer** implementation following the EMVA GenICam GenTL Standard v1.6.

## Overview

The GenTL standard defines a modular architecture for camera transport layers:

```
System (TL) Module
  └── Interface Module (FPGA CoaXPress)
       └── Device Module (Camera)
            └── Stream Module (Acquisition)
                 └── Buffer Module
```

## Architecture

### Modules

1. **System Module (`SystemModule.cpp`)**
   - Transport layer discovery and initialization
   - Interface enumeration
   - Global library info queries

2. **Interface Module (`SystemModule.cpp`)**
   - FPGA CoaXPress interface management
   - Device discovery and enumeration
   - Device connection handling

3. **Device Module (`DeviceModule.cpp`)**
   - Camera device abstraction
   - GenApi port access (via `FpgaTransport`)
   - Stream creation and management

4. **Stream Module (`StreamModule.cpp`)**
   - Acquisition control (start/stop)
   - Buffer queue management
   - Frame delivery

5. **Buffer Module (`StreamModule.cpp`)**
   - Memory management (user-provided or allocated)
   - Buffer metadata (timestamp, frame ID, etc.)
   - Queue state tracking

### C API (`GenTL.cpp`)

Standard GenTL C functions exported for consumer applications:

- **System**: `GCInitLib`, `GCCloseLib`, `GCGetInfo`
- **TL**: `TLOpen`, `TLClose`, `TLGetNumInterfaces`, `TLOpenInterface`
- **Interface**: `IFClose`, `IFGetNumDevices`, `IFOpenDevice`, `IFUpdateDeviceList`
- **Device**: `DevClose`, `DevGetPort`, `DevOpenDataStream`
- **Stream**: `DSStartAcquisition`, `DSStopAcquisition`, `DSAnnounceBuffer`, `DSQueueBuffer`
- **Port**: `GCReadPort`, `GCWritePort`, `GCGetPortURL`

## Building

The producer builds as a shared library with `.cti` extension (standard for GenTL):

```bash
cmake -B build -S .
cmake --build build
sudo cmake --install build  # Installs to /usr/lib/genicam/gentl/
```

## Usage

### With GenTL-compatible Consumer

Any GenICam-aware application (Harvester, Vimba, etc.) can load this producer:

```python
from harvesters.core import Harvester

h = Harvester()
h.add_file('/usr/lib/genicam/gentl/FpgaCXP.cti')
h.update()

# Enumerate devices
for device in h.device_info_list:
    print(device)

# Acquire
ia = h.create_image_acquirer(0)
ia.start()
with ia.fetch() as buffer:
    print(f"Frame {buffer.payload.components[0].data}")
ia.stop()
ia.destroy()
h.reset()
```

### Direct C API

```c
#include "gentl/GenTL.h"

TL_HANDLE tl;
IF_HANDLE iface;
DEV_HANDLE dev;
DS_HANDLE stream;
PORT_HANDLE port;

// Initialize
GCInitLib();
TLOpen(&tl);

// Enumerate interfaces
uint32_t numIfaces;
TLGetNumInterfaces(tl, &numIfaces);

char ifaceID[256];
size_t size = sizeof(ifaceID);
TLGetInterfaceID(tl, 0, ifaceID, &size);

// Open interface
TLOpenInterface(tl, ifaceID, &iface);

// Enumerate devices
uint32_t numDevices;
IFGetNumDevices(iface, &numDevices);

char devID[256];
size = sizeof(devID);
IFGetDeviceID(iface, 0, devID, &size);

// Open device
IFOpenDevice(iface, devID, DEVICE_ACCESS_READWRITE, &dev);

// Get GenApi port for register access
DevGetPort(dev, &port);

// Read/write registers via GenApi
uint8_t data[4];
size = 4;
GCReadPort(port, 0x1000, data, &size);

// Open stream
char streamID[256];
size = sizeof(streamID);
DevGetDataStreamID(dev, 0, streamID, &size);
DevOpenDataStream(dev, streamID, &stream);

// Announce buffers
BUFFER_HANDLE buffers[4];
for (int i = 0; i < 4; i++) {
    DSAllocAndAnnounceBuffer(stream, payloadSize, NULL, &buffers[i]);
    DSQueueBuffer(stream, buffers[i]);
}

// Start acquisition
DSStartAcquisition(stream, ACQ_START_FLAGS_DEFAULT, GENTL_INFINITE);

// Wait and retrieve frames...
// (requires event handling - not shown)

// Stop
DSStopAcquisition(stream, ACQ_STOP_FLAGS_DEFAULT);

// Cleanup
DSClose(stream);
DevClose(dev);
IFClose(iface);
TLClose(tl);
GCCloseLib();
```

## Integration with FpgaTransport

The Device Module creates an `FpgaTransport` instance (GenApi `IPort` implementation) that provides register-level access to the camera. GenApi XML files can use this port to define high-level camera features:

```cpp
// Inside DeviceModule::initializeDevice()
auto* transport = new FpgaTransport("/dev/mem");
port_ = transport;  // Exposed via DevGetPort()
```

Consumers can then load the camera's XML and control it via GenApi:

```cpp
PORT_HANDLE port;
DevGetPort(dev, &port);

// Load GenApi XML
char xmlUrl[512];
size_t size = sizeof(xmlUrl);
GCGetPortURL(port, xmlUrl, &size);

// Parse XML and create node map
// (using GenApi library - not shown)
```

## Standards Compliance

- GenTL Standard v1.6 (EMVA)
- GenICam Standard v3.1 (EMVA)
- CoaXPress (CXP) v2.1

## Thread Safety

All modules use `std::mutex` for thread-safe operations. Multiple threads can safely:
- Query info from handles
- Queue/dequeue buffers
- Start/stop acquisition

## Error Handling

- Standard GenTL error codes (`GC_ERROR`)
- Last error retrievable via `GCGetLastError()`
- Logging via spdlog (see `common/logger/`)

## Limitations / TODO

- [ ] Event handling not implemented (polling only for now)
- [ ] Single device per interface (expand discovery for multiple cameras)
- [ ] Frame acquisition loop (integrate with DMA or shared memory)
- [ ] Chunk data parsing
- [ ] Multicast streaming
- [ ] GenTL SFNC (Standard Features Naming Convention) validation

## See Also

- [GenTL Specification](https://www.emva.org/standards-technology/genicam/)
- [GenApi Documentation](https://www.emva.org/wp-content/uploads/GenICam_Standard_v3_0.pdf)
- [Harvester (Python GenTL Consumer)](https://github.com/genicam/harvesters)
