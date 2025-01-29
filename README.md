# How to Build

## Prerequisites

Ensure the following environment variables are set:

**Windows (MinGW)**: 

- `VULKAN_SDK = C:\VulkanSDK\1.4.304.0\Bin`
- `PATH = C:\Qt\6.8.1\mingw_64\bin` and `C:\Qt\6.8.1\mingw_64\lib`

**Linux/macOS**: Adjust according to your Qt installation directory.

Ensure you use appropriate version of the compiler, in my case **GCC 13.x**, as Qt 6.8.1 does not support GCC 14.x.

## Build Instructions
### 1. Clone the Repository
```sh
git clone https://github.com/decastyle/discovering-path-tracer
cd discovering-path-tracer
```
### 2. Set Up the Build Directory

```sh
mkdir build
```
### 3. Run CMake for Your Specific Platform
### 4. Build the Project

```sh
cmake --build .
```

