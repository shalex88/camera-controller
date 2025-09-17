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