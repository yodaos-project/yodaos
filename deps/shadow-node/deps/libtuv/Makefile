# Copyright 2015-2017 Samsung Electronics Co., Ltd.
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

UNAME_M := $(shell uname -m)
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	UNAME_S := linux
endif
ifeq ($(UNAME_S),Darwin)
	UNAME_S := darwin
endif


TUV_PLATFORM    ?= $(UNAME_M)-$(UNAME_S)
TUV_BUILD_TYPE  ?= debug
TUV_BOARD       ?= unknown
TUV_SYSTEMROOT  ?= default
TUV_BUILDTESTER ?= yes
TUV_TEST_FORK   ?= no
TUV_BUILDAPIEMULTESTER ?= no
TUV_BUILDHOSTHELPER ?= no

OUTPUT_ROOT     := build
BUILD_FOLDER    := ./$(OUTPUT_ROOT)/$(TUV_PLATFORM)/$(TUV_BUILD_TYPE)
CMAKE_FOLDER    := $(BUILD_FOLDER)/cmake
CMAKE_DEFINES   := \
	-DCMAKE_TOOLCHAIN_FILE=./cmake/config/config_$(TUV_PLATFORM).cmake \
	-DCMAKE_BUILD_TYPE=$(TUV_BUILD_TYPE) \
	-DTARGET_PLATFORM=$(TUV_PLATFORM) \
	-DBUILDTESTER=${TUV_BUILDTESTER} \
	-DBUILD_HOST_HELPER=${TUV_BUILDHOSTHELPER}

ifneq ($(TUV_BOARD),unknown)
	CMAKE_DEFINES += -DTARGET_BOARD=${TUV_BOARD}
endif

ifneq ($(TUV_SYSTEMROOT),default)
	CMAKE_DEFINES += -DTARGET_SYSTEMROOT=${TUV_SYSTEMROOT}
endif

ifneq ($(TUV_BUILDAPIEMULTESTER),no)
	CMAKE_DEFINES += -DBUILDAPIEMULTESTER=${TUV_BUILDAPIEMULTESTER}
endif

ifneq ($(TUV_TEST_FORK),no)
  CMAKE_DEFINES += -DTUV_TEST_FORK=ON
endif

.phony: all

all:
	mkdir -p $(CMAKE_FOLDER)
	cmake -B$(CMAKE_FOLDER) -H./ $(CMAKE_DEFINES)
	make -C $(CMAKE_FOLDER)
ifeq ($(TUV_PLATFORM), $(filter $(TUV_PLATFORM), i686-linux x86_64-linux x86_64-darwin))
	@echo '=============================================================='
	@echo 'to run test,'
	@echo '$(BUILD_FOLDER)/bin/tuvtester'
	@echo '=============================================================='
endif
ifeq ("$(TUV_BUILDHOSTHELPER)","yes")
	@echo '=============================================================='
	@echo 'to run helper in host,'
	@echo '$(BUILD_FOLDER)/bin/tuvhelper'
	@echo '=============================================================='
endif

run_test:
	make
ifeq ($(TUV_PLATFORM), $(filter $(TUV_PLATFORM), i686-linux x86_64-linux x86_64-darwin))
	$(BUILD_FOLDER)/bin/tuvtester
else
	@echo 'Cannot run test for $(TUV_PLATFORM)'
endif

call:
	make -C $(CMAKE_FOLDER)

clean:
	rm -rf $(OUTPUT_ROOT)
