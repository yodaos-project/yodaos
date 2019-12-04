# OpenWrt

We use `openwrt` to build YodaOS image, which includes linux, drivers, uboot and other softwares.

## Compile

Enter the `./openwrt` directory:

```sh
$ cd openwrt
```

Copy your defconfig file to `.config`:

```sh
$ cp configs/<boardconfig>_defconfig .config
```

Then generate the final `.config` and do make:

```sh
$ make defconfig
$ make -j<jobs> V=s
```

## FAQ

__Q: Where is the generated files and image?__

See the directory `openwrt/bin`.

__Q: How to generate a default config?__

There are 2 choices:

1. use `make menuconfig`
2. use `scripts/diffconfig.sh` as `./scripts/diffconfig.sh > configs/<boardconfig>_defconfig`.

__Q: How to compile a specific package?__

To clean/compile/install, run the following commands:

```sh
$ make package/<pkg-name>/{clean,compile,install}
```

And toolchain related:

```sh
$ make toolchain/{clean,compile,install}
```

__Q: How to compile linux?__

- compile the module: `make target/linux/compile`
- compile the zImage, dts, then install to openwrt/bin: `make target/linux/install`

__Q: How to modify the kernel config?__

Modify all:

```sh
$ make kernel_menuconfig
```

Modify a specific board:

```sh
$ make kernel_menuconfig CONFIG_TARGET=<subtarget>
```
