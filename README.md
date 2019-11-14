<p align="center">
  <img alt="YODAOS" src="logo.png" width="400" />
</p>

<p align="center">
  <img src="https://img.shields.io/badge/base-linux-green.svg" />
  <img src="https://img.shields.io/badge/build-openwrt-blue.svg" />
</p>

This is a modern operating system for next generation interactive device, and it embraces Web community,
uses JavaScript as the application language.

## Get Started

To start with compiling [YODAOS][], a Linux is required, we recommend the followings distributions:

- Ubuntu 16.04
- Centos 7

For Ubuntu:

```sh
$ apt-get install build-essential subversion libncurses5-dev zlib1g-dev gawk gcc-multilib flex git-core gettext libssl-dev unzip texinfo device-tree-compiler dosfstools libusb-1.0-0-dev
```

For Centos 7, the install command-line is:

```sh
$ yum install -y unzip bzip2 dosfstools wget gcc gcc-c++ git ncurses-devel zlib-static openssl-devel svn patch perl-Module-Install.noarch perl-Thread-Queue
# And the `device-tree-compiler` also needs to install manually:
$ wget http://www.rpmfind.net/linux/epel/6/x86_64/Packages/d/dtc-1.4.0-1.el6.x86_64.rpm
$ rpm -i dtc-1.4.0-1.el6.x86_64.rpm
```

### Download Source

Click [http://openai-corp.rokid.com](http://openai-corp.rokid.com), and do register as Rokid Developer.

Then, go [SSH Settings](https://openai-corp.rokid.com/#/settings/ssh-keys) to config your local public key.

> https://help.github.com/articles/generating-a-new-ssh-key-and-adding-it-to-the-ssh-agent/

### Install the `repo` cli

[YODAOS][] uses the `repo` command-line to manage the source tree:

> Repo unifies Git repositories when necessary, performs uploads to the Gerrit revision control system, and automates parts of the Android development workflow. Repo is not meant to replace Git, only to make it easier to work with Git in the context of Android. The repo command is an executable Python script that you can put anywhere in your path.

Follow the below commands to install manually:

```sh
$ curl https://raw.githubusercontent.com/yodaos-project/yodaos/master/tools/repo > /usr/local/bin/repo
$ chmod 777 /usr/local/bin/repo
```

And use `repo --help` to test if installation is done.

### Compile

When the `repo` cli is ready, follow the instruments to get the complete source of [YODAOS][]:

```sh
$ repo init -u https://github.com/yodaos-project/yodaos.git -m manifest.xml --repo-url=http://openai-corp.rokid.com/tools/repo --no-repo-verify
$ .repo/manifests/tools/repo-username -u {your rokid developer username} # set your username to fetch source from gerrit
$ repo sync
```

The above takes few minutes, just be patient. The next step is to build out the OS image, let's take the example of Raspberry board:

```sh
$ cp -r products/yodaos/rbpi-3b-plus/configs/broadcom_bcm2710_rpi3b_plus_defconfig openwrt/.config
$ cd openwrt
$ make defconfig && make
```

The `cp` command is to select which board that you would build, the following are the boards config table:

| board             | product               | config  |
|-------------------|-----------------------|---------|
| Raspberry 3b plus | [raspberry][] | products/yodaos/rbpi-3b-plus/configs/broadcom_bcm2710_rpi3b_plus_defconfig |
| Kamino18          | [kamino18][]  | products/rokid/dev3.3/configs/leo_k18_dev33_defconfig |

Remember that `make defconfig` after you redo the copy on the specific board config.

Go [compile & run](https://yodaos-project.github.io/yoda-book/en-us/yodaos-source/system/compile-run.html) for more details.

## Releases

This section describes how do [YODAOS][] release and its lifecycle.

### versions

[YODAOS][] uses [Semver 2.0][] for versioning management, and we have lifecycles for `major` and `minor`.

- `major`: one release per 6 month, and all releases are stable.
- `minor`: one release per 1 month, and only the even ones are __stable__, namely `1.0.x`, `1.2.x`, `1.4.x`.

We assume the 3rd Thursday at every month as [YodaOS][] release date.

### release requirements

Every release must contain the following parts:

- Repo manifest commit with specific version, for downloading the source code.
- Multiple platform images, includes:
  - Rokid Kamino18 soc.
  - AmLogic A113.
- Changelog describes what changes this updated.

### stable and pre-versions

Stable version requires the complete tests on:

- Compatibility test suite
- Unit tests of modules and services
- Functional tests
- Integration tests

The above workload is in usual 2 weeks for the team, therefore we introduce pre-version, namely release
candidates(RC) to track the stable releasing progress.

For stable version, we preserve 2 weeks at least to take the workload of full tests, and create a pre-version
branch `v1.2.x-rc`. Once all the full tests are passed, then create a stable branch(`v1.2.x`) from release
candidate branch(`v1.2.x-rc`).

## Community

- [YouTube](https://www.youtube.com/channel/UCRvBWIaBcsfvCTC_4EKW4lw)

## Contributing

[YODAOS][] is a community-driven project that we accepts any improved proposals, pull requests and issues.

- For JavaScript development, go [yodart][] for details.
- For proposal, [yodaos-project/evolution][] is the place where someone can submit pull request to propose something.

## Documentation

- [YodaOS Book](https://github.com/yodaos-project/yoda-book)

## License

Apache 2.0

[YODAOS]: https://github.com/yodaos-project
[yoda.js]: https://github.com/yodaos-project/yoda.js
[flora]: https://github.com/yodaos-project/flora
[flora-dispatcher]: https://github.com/yodaos-project/flora-dispatcher
[yodaos-project/evolution]: https://github.com/yodaos-project/evolution
[yoda-speech-service]: https://github.com/Rokid/yoda-speech-service
[Semver 2.0]: https://semver.org/
[ShadowNode]: https://github.com/yodaos-project/ShadowNode
[Node.js]: https://github.com/nodejs/node
[raspberry]: https://github.com/yodaos-project/product-raspberry
[kamino18]: https://github.com/yodaos-project/product-kamino18
