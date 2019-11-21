mingutils
==========

小明的工具，包括日志、序列化、命令行参数解析、URI 解析等功能。

## 编译

### 依赖

* CMake 3.2+
* [Rokid/CMake-Modules](https://github.com/Rokid/CMake-Modules.git)

### 主机编译

```shell
$ ./config \
    --build-dir=${build目录} \  # cmake生成的makefiles目录, 编译生成的二进制文件也将在这里
    --cmake-modules=${cmake_modules目录} \  # 指定cmake-modules所在目录
    --prefix=${prefixPath}  # 安装路径
$ cd ${build目录}
$ make && make install
```

### 交叉编译

```shell
$ ./config \
    --build-dir=${build目录} \
    --cmake-modules=${cmake_modules目录} \
    --toolchain=${工具链目录} \
    --cross-prefix=${工具链命令前缀} \    # 如arm-openwrt-linux-gnueabi-
    --prefix=${prefixPath}             # 安装路径
$ cd ${build目录}
$ make
```
### 其它config选项

- `--debug` 使用调试模式编译
- `--build-demo` 编译演示/测试程序

## License

MIT
