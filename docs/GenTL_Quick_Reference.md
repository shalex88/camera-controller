# GenTL Producer Quick Reference

## Build Commands

```bash
# Configure project
cmake -B build -S .

# Build everything (including GenTL producer)
cmake --build build

# Build only GenTL producer
cmake --build build --target FpgaCXPProducer

# Build example
cmake --build build --target gentl_example

# Install (requires sudo for /usr/lib/genicam/gentl/)
sudo cmake --install build
```

## Files Generated

- `build/src/infrastructure/camera/protocol/gentl/FpgaCXP.cti` - GenTL Producer library
- `build/src/infrastructure/camera/protocol/gentl/FpgaCXP.cti.info` - Producer metadata
- `build/examples/gentl_example` - Example application

## Running the Example

```bash
# Without device access (enumeration only)
./build/examples/gentl_example

# With device access (requires root for /dev/mem)
sudo ./build/examples/gentl_example
```

## GenTL API Cheat Sheet

### Initialization
```c
GCInitLib();                          // Initialize library
TLOpen(&tl);                          // Open system/TL
```

### Interface Discovery
```c
TLGetNumInterfaces(tl, &numIfaces);   // Count interfaces
TLGetInterfaceID(tl, 0, id, &size);   // Get interface ID
TLOpenInterface(tl, id, &iface);      // Open interface
```

### Device Discovery
```c
IFUpdateDeviceList(iface, &changed, timeout);  // Refresh device list
IFGetNumDevices(iface, &numDevices);           // Count devices
IFGetDeviceID(iface, 0, id, &size);            // Get device ID
IFOpenDevice(iface, id, access, &dev);         // Open device
```

### Register Access (GenApi Port)
```c
DevGetPort(dev, &port);               // Get IPort handle
GCReadPort(port, addr, buf, &size);   // Read register
GCWritePort(port, addr, buf, &size);  // Write register
GCGetPortURL(port, url, &size);       // Get XML location
```

### Stream & Acquisition
```c
DevOpenDataStream(dev, id, &stream);           // Open stream
DSAllocAndAnnounceBuffer(stream, size, &buf);  // Create buffer
DSQueueBuffer(stream, buf);                    // Queue for acquisition
DSStartAcquisition(stream, flags, numFrames);  // Start
// ... wait for frames via events ...
DSStopAcquisition(stream, flags);              // Stop
```

### Info Queries
```c
TLGetInfo(tl, TL_INFO_VENDOR, &type, buf, &size);
IFGetInfo(iface, INTERFACE_INFO_DISPLAYNAME, &type, buf, &size);
DevGetInfo(dev, DEVICE_INFO_MODEL, &type, buf, &size);
DSGetInfo(stream, STREAM_INFO_PAYLOAD_SIZE, &type, buf, &size);
```

### Cleanup
```c
DSClose(stream);
DevClose(dev);
IFClose(iface);
TLClose(tl);
GCCloseLib();
```

## Error Handling

```c
GC_ERROR err = TLOpen(&tl);
if (err != GC_ERR_SUCCESS) {
    char errMsg[256];
    size_t size = sizeof(errMsg);
    GCGetLastError(&err, errMsg, &size);
    fprintf(stderr, "Error: %s\n", errMsg);
}
```

## Common Info Commands

### TL Info
- `TL_INFO_ID` - Producer ID ("FpgaCXP")
- `TL_INFO_VENDOR` - Vendor name
- `TL_INFO_MODEL` - Model name
- `TL_INFO_VERSION` - Version string
- `TL_INFO_TLTYPE` - Transport type ("CXP")

### Device Info
- `DEVICE_INFO_ID` - Device ID
- `DEVICE_INFO_VENDOR` - Camera vendor
- `DEVICE_INFO_MODEL` - Camera model
- `DEVICE_INFO_SERIAL_NUMBER` - Serial number
- `DEVICE_INFO_VERSION` - Firmware version
- `DEVICE_INFO_DISPLAYNAME` - User-friendly name

### Stream Info
- `STREAM_INFO_PAYLOAD_SIZE` - Frame size in bytes
- `STREAM_INFO_NUM_DELIVERED` - Frames delivered
- `STREAM_INFO_NUM_UNDERRUN` - Buffer underruns
- `STREAM_INFO_IS_GRABBING` - Acquisition active?

## Integration with Harvester (Python)

```python
from harvesters.core import Harvester

# Load producer
h = Harvester()
h.add_file('/usr/lib/genicam/gentl/FpgaCXP.cti')
h.update()

# List devices
print(h.device_info_list)

# Acquire
with h.create_image_acquirer(0) as ia:
    ia.start()
    for i in range(10):
        with ia.fetch() as buffer:
            print(f"Frame {i}: {buffer.payload.components[0].data.shape}")
    ia.stop()
```

## Environment Variables

```bash
# Add producer path for auto-discovery
export GENICAM_GENTL64_PATH=/usr/lib/genicam/gentl

# Enable GenTL logging
export GENICAM_LOG_CONFIG=/path/to/log.ini
```

## Troubleshooting

### "Failed to open device"
- Ensure you have root access for `/dev/mem`
- Run with `sudo` or add appropriate udev rules

### "No interfaces found"
- Check that FPGA is accessible
- Verify `/dev/mem` exists and is readable

### "Buffer too small"
- Call info functions twice: first with NULL buffer to get required size

### "Invalid handle"
- Ensure handles are opened in correct order: TL → Interface → Device → Stream
- Don't use handles after closing

## Standards Reference

- **GenTL Spec**: https://www.emva.org/standards-technology/genicam/genicam-downloads/
- **GenICam Standard**: https://www.emva.org/standards-technology/genicam/
- **SFNC**: Standard Features Naming Convention

## See Also

- `/docs/GenTL_Producer_Implementation.md` - Full implementation details
- `/src/infrastructure/camera/protocol/gentl/README.md` - Module documentation
- `/examples/gentl_example.cpp` - Working example code
