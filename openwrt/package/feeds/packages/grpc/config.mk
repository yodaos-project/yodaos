# Extracted from OpenWRT Makefile so that the version information can be used
# for consistent ubuntu builds
PKG_NAME:=grpc
PKG_VERSION:=master
PKG_SOURCE_VERSION=$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
