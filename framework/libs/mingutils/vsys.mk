local-path := $(call my-dir)

include $(clear-vars)
local.module := mutils
local.ndk-script := $(local-path)/ndk.mk
include $(build-ndk-module)
