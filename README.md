# camera-service

[![Test](https://github.com/shalex88/camera-service/actions/workflows/test.yml/badge.svg)](https://github.com/shalex88/camera-service/actions/workflows/test.yml)
[![Coverage](https://img.shields.io/codecov/c/github/shalex88/camera-service)](https://codecov.io/github/shalex88/camera-service)
[![Release](https://img.shields.io/github/v/release/shalex88/camera-service.svg)](https://github.com/shalex88/camera-service/releases/latest)

## Usage

```bash
A camera control service
camera-service [OPTIONS]
OPTIONS:
-h,     --help              Print this help message and exit
-v,     --version           Show version information
-c,     --config TEXT:FILE  Configuration file path
```

## Run

```bash
./camera-service ../config/config-wfov.yaml

# Run client
grpcui -plaintext 0.0.0.0:50051
```

## Test

### Unit tests

```bash
./camera-service-unit-tests
```

### Integration tests

```bash
./camera-service-integration-tests
```

### System tests

```bash
./camera-service-system-tests
```

## Add new functionality

### Camera

1. Add new functionality in `src/common/types/CameraCapabilities.h`
2. Extend ICameraHal with the new capability
3. Implement the capability in CameraHal class
4. Implement the new capability in the concrete camera class

### Core

1. Add new function in ICore interface
2. Implement new function in Core class

### API

1. Add new function in IRequestHandler interface
2. Implement new function in RequestHandler class
3. Define new RPC in `proto/camera_service.proto`
4. Create new RPC in GrpcCallbackHandler class
