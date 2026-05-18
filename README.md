# hello-world

Linux Character Device Driver

## 项目描述

实现一个Linux字符设备驱动，具有以下特性：

- **设备名称**: hello
- **主设备号**: 动态分配
- **支持的操作**: open、read、write、release

## 功能说明

### write 操作
- 接收用户空间传来的数据
- 使用 `copy_from_user()` 从用户空间复制数据到内核空间
- 存储输入的字符串

### read 操作
- 返回格式为 "Hello {string}" 的数据
- 例如：如果 write 输入 "world"，read 返回 "Hello world"
- 使用 `copy_to_user()` 从内核空间复制数据到用户空间

## 编译和使用

### 编译驱动模块

```bash
make module
```

### 编译测试程序

```bash
make test
```

### 加载驱动

```bash
sudo insmod src/hello.ko
```

### 运行测试程序

```bash
sudo ./test_hello
```

### 卸载驱动

```bash
sudo rmmod hello
```

### 查看内核日志

```bash
make dmesg
# 或
dmesg | tail -20
```

## 文件说明

- `src/hello.c` - 字符设备驱动源代码
- `src/test.c` - 用户态测试程序
- `Makefile` - 编译配置文件

## 驱动实现细节

### 动态分配主设备号

```c
alloc_chrdev_region(&device_number, 0, 1, DEVICE_NAME);
```

### 字符设备操作

- `device_open()` - 设备打开
- `device_read()` - 读取操作，返回 "Hello {string}"
- `device_write()` - 写入操作，接收用户数据
- `device_release()` - 设备关闭

### 用户空间数据交互

- `copy_from_user()` - 从用户空间读取数据
- `copy_to_user()` - 向用户空间写入数据
