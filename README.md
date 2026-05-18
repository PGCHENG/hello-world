# Linux Character Device Driver - Hello Device

A complete implementation of a Linux character device driver with kernel module, Makefile, test program, and documentation.

## 📋 Table of Contents

- [Features](#features)
- [Project Structure](#project-structure)
- [Requirements](#requirements)
- [Building](#building)
- [Installation](#installation)
- [Usage](#usage)
- [Testing](#testing)
- [Implementation Details](#implementation-details)
- [Troubleshooting](#troubleshooting)
- [Performance Notes](#performance-notes)

## ✨ Features

✅ **Dynamic Device Number Allocation**
- Uses `alloc_chrdev_region()` to dynamically allocate major device number
- No hardcoded device number needed

✅ **Complete File Operations**
- `open()` - Opens the device and allocates kernel buffer
- `read()` - Returns "Hello {user_string}\n" formatted message
- `write()` - Accepts user input and stores in kernel buffer
- `release()` - Closes the device cleanly

✅ **Safe User-Kernel Communication**
- `copy_from_user()` - Safely copy user data to kernel space
- `copy_to_user()` - Safely copy kernel data to user space
- Full error handling for memory access violations

✅ **Proper Memory Management**
- `kmalloc()` for kernel buffer allocation
- `kfree()` for proper cleanup
- No memory leaks

✅ **Automatic Device Node Creation**
- Uses `device_create()` for automatic `/dev/hello` creation
- Automatic cleanup in exit function

✅ **Comprehensive Error Handling**
- Checks all kernel API return values
- Proper error unwinding and resource cleanup
- Detailed error messages via `pr_err()` and `pr_info()`

✅ **Complete Module Information**
- MODULE_LICENSE
- MODULE_AUTHOR
- MODULE_DESCRIPTION
- MODULE_VERSION

## 📁 Project Structure

```
.
├── hello_driver.c    # Main driver source code
├── Makefile          # Build configuration
├── test.c            # User-space test program
└── README.md         # This file
```

## 📦 Requirements

- Linux kernel with headers (>= 2.6)
- GCC compiler
- Build tools (`make`, `gcc`)
- Root/sudo access for driver installation

### Installing Requirements (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install build-essential linux-headers-$(uname -r)
```

### Installing Requirements (Fedora/RHEL)

```bash
sudo dnf install gcc kernel-devel kernel-headers
```

## 🔨 Building

### Compile the Driver

```bash
make
```

This will generate:
- `hello_driver.ko` - The kernel module
- `hello_driver.o` - Object file
- Other build artifacts

### Clean Build Artifacts

```bash
make clean
```

## 📥 Installation

### Load the Driver

```bash
sudo insmod hello_driver.ko
```

### Verify Driver Installation

```bash
lsmod | grep hello
```

You should see output like:
```
hello_driver           xxxx  0
```

### Check Kernel Messages

```bash
dmesg | tail
```

You should see:
```
[xxx.xxx] hello: Initializing character device driver
[xxx.xxx] hello: Allocated device number major=XYZ, minor=0
[xxx.xxx] hello: Character device driver loaded successfully
[xxx.xxx] hello: Device node created at /dev/hello
```

### Verify Device Node

```bash
ls -l /dev/hello
```

### Unload the Driver

```bash
sudo rmmod hello_driver
```

## 🚀 Usage

### Basic Command-Line Operations

#### Read (without prior write)

```bash
cat /dev/hello
# Output: Hello World
```

#### Write and Read

```bash
echo "Alice" > /dev/hello
cat /dev/hello
# Output: Hello Alice
```

#### Write with newline handling

```bash
echo -n "Bob" > /dev/hello
cat /dev/hello
# Output: Hello Bob
```

### Programmatic Usage (C)

```c
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main() {
    // Write data
    int fd = open("/dev/hello", O_WRONLY);
    write(fd, "John", 4);
    close(fd);
    
    // Read data
    char buffer[256];
    fd = open("/dev/hello", O_RDONLY);
    ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
    buffer[n] = '\0';
    printf("%s", buffer);  // Output: Hello John
    close(fd);
    
    return 0;
}
```

## 🧪 Testing

### Compile the Test Program

```bash
gcc -o test test.c
```

### Run the Test Suite

```bash
# Make sure the driver is loaded
sudo insmod hello_driver.ko

# Run tests (may need sudo for device access)
sudo ./test
```

### Test Cases

The test program includes 6 comprehensive test cases:

1. **Basic Read (No Write)**
   - Expected: "Hello World\n"
   - Tests default behavior

2. **Write Then Read**
   - Writes "Alice", reads "Hello Alice\n"
   - Tests basic write-read cycle

3. **Multiple Writes (Overwrite)**
   - Tests that second write overwrites first
   - Writes "Bob", then "Charlie", reads "Hello Charlie\n"

4. **Write With Newline**
   - Tests newline stripping
   - Writes "Dave\n", reads "Hello Dave\n"

5. **Large Write (Truncation)**
   - Tests buffer size handling
   - Verifies data is truncated to buffer size

6. **Empty Write**
   - Tests default value after empty write
   - Expects "Hello World\n"

## 🔍 Implementation Details

### Architecture Overview

```
User Space
    |
    | system calls (open, read, write, close)
    v
Kernel Space
    |
    +---> File Operations Handler
    |     - hello_open()
    |     - hello_read()
    |     - hello_write()
    |     - hello_release()
    |
    +---> Character Device Driver
    |     - cdev structure
    |     - Device number management
    |
    +---> Kernel Buffer (256 bytes)
    |     - Allocated with kmalloc
    |     - Stores user-written data
    |
    +---> Device Class & Node
          - /dev/hello (created automatically)
```

### Key Functions

#### `hello_open()`
- Called when user opens `/dev/hello`
- Allocates kernel buffer (256 bytes) using `kmalloc()`
- Returns 0 on success, -ENOMEM if allocation fails

#### `hello_write()`
- Called when user writes to device
- Uses `copy_from_user()` to safely copy data from user space
- Limits write size to BUFFER_SIZE - 1
- Strips trailing newline for clean string storage
- Returns number of bytes written

#### `hello_read()`
- Called when user reads from device
- Formats output as "Hello {buffer}\n" using snprintf
- Uses `copy_to_user()` to safely copy formatted data to user space
- Returns number of bytes copied

#### `hello_init()`
- Module initialization
- Steps:
  1. Allocate device number dynamically
  2. Initialize cdev structure
  3. Register character device
  4. Create device class
  5. Create device node

#### `hello_exit()`
- Module cleanup
- Steps:
  1. Free allocated buffer
  2. Destroy device node
  3. Destroy device class
  4. Unregister character device
  5. Release device number

### Memory Layout

```
Kernel Memory
┌─────────────────────────┐
│  hello_device struct    │
│  ┌───────────────────┐  │
│  │ *buffer (ptr)     │──┼──> ┌─────────────────┐
│  │ buffer_len        │  │    │  Kernel Buffer  │
│  └───────────────────┘  │    │  (256 bytes)    │
│                         │    │  kmalloc'd mem  │
│  cdev structure         │    └─────────────────┘
│  device_number          │
│  device class/node      │
└─────────────────────────┘
```

## 🔧 Troubleshooting

### Permission Denied

```bash
# Make sure to use sudo
sudo cat /dev/hello
sudo echo "test" > /dev/hello
```

### Device Not Found

```bash
# Check if driver is loaded
lsmod | grep hello

# Load driver if not present
sudo insmod hello_driver.ko

# Verify device node
ls -l /dev/hello
```

### Module Fails to Load

```bash
# Check kernel compatibility
uname -r  # Check kernel version

# View detailed error messages
dmesg | tail -20

# Try rebuilding
make clean
make
sudo insmod hello_driver.ko
```

### Read Returns Garbage

```bash
# Make sure to write data first
echo "test" > /dev/hello
cat /dev/hello
```

## 📊 Performance Notes

### Buffer Size

- Current buffer size: 256 bytes
- Can be changed by modifying `BUFFER_SIZE` in `hello_driver.c`
- Larger buffers consume more kernel memory
- Recommended range: 64 - 4096 bytes

### Copy Operations

- `copy_from_user()` and `copy_to_user()` are safe but slightly slower
- They perform security checks to prevent kernel memory corruption
- Acceptable overhead for device driver operations

### Multiple Processes

- Current implementation shares a single buffer across all opens
- For concurrent access, consider:
  - Adding mutex locking
  - Per-process buffers
  - Ring buffers for asynchronous operations

## 🛡️ Security Considerations

1. **Memory Safety**: All user-kernel data transfers use safe copy functions
2. **Buffer Overflow**: Write size is limited to buffer size
3. **Null Termination**: String data is properly null-terminated
4. **Resource Cleanup**: All allocated resources are properly freed
5. **Error Handling**: All kernel API calls are checked for errors

## 📝 License

GNU Public License v2 (GPL v2)

## 👤 Author

PGCHENG

## 🔗 References

- Linux Device Drivers, 3rd Edition by Jonathan Corbet, Alessandro Rubini, and Greg Kroah-Hartman
- [Linux Kernel Documentation](https://www.kernel.org/doc/)
- [Character Device Drivers](https://tldp.org/LDP/lkmpg/2.4/html/c577.html)

## 📞 Support

For issues or questions:
1. Check the Troubleshooting section
2. Review kernel messages: `dmesg`
3. Verify build requirements are installed
4. Check file permissions on `/dev/hello`
