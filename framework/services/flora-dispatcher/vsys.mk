local-path := $(call my-dir)

include $(clear_vars)
local.module := flora-dispatcher
local.ndk-modules := flora-dispatcher.ndk
include $(build-executable)

include $(clear-vars)
local.module := flora-dispatcher.ndk
local.ndk-script := $(local-path)/ndk.mk
local.ndk-modules := flora-svc
include $(build-ndk-module)
