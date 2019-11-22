#!/bin/bash

# Copyright 2016-present Samsung Electronics Co., Ltd. and other contributors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

sudo apt-key adv --fetch-keys http://dl.yarnpkg.com/debian/pubkey.gpg
echo "deb http://dl.yarnpkg.com/debian/ stable main" | \
  sudo tee /etc/apt/sources.list.d/yarn.list

# Fingerprint: 6084 F3CF 814B 57C1 CF12 EFD5 15CF 4D18 AF4F 7421
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
echo "
deb http://apt.llvm.org/trusty/ llvm-toolchain-trusty main
deb-src http://apt.llvm.org/trusty/ llvm-toolchain-trusty main
deb http://apt.llvm.org/trusty/ llvm-toolchain-trusty-7 main
deb-src http://apt.llvm.org/trusty/ llvm-toolchain-trusty-7 main
" | sudo tee /etc/apt/sources.list.d/llvm.list

sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test

sudo apt-get update -q
sudo apt-get install -q -y \
    cmake valgrind clang-format-7 \
    zlib1g-dev \
    dbus libdbus-1-dev dbus-x11 \
    mosquitto \
    build-essential

sudo apt-get install -y -qq yarn

sudo ln -s /usr/bin/clang-format-7 /usr/bin/clang-format-7.0
