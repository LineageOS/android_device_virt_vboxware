#
# Copyright (C) 2024 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

# Inherit from common
include device/virt/virt-common/BoardConfigVirtCommon.mk

USES_DEVICE_VIRT_VBOXWARE := true
DEVICE_PATH := device/virt/vboxware

# Boot manager
TARGET_GRUB_BOOT_CONFIG := $(DEVICE_PATH)/grub/grub-boot.cfg
TARGET_GRUB_INSTALL_CONFIG := $(DEVICE_PATH)/grub/grub-install.cfg
TARGET_REFIND_BOOT_CONFIG := $(DEVICE_PATH)/rEFInd/refind-boot.conf
TARGET_REFIND_INSTALL_CONFIG := $(DEVICE_PATH)/rEFInd/refind-install.conf

# Graphics (Mesa)
ifneq ($(wildcard external/mesa/android/Android.mk),)
BUILD_BROKEN_INCORRECT_PARTITION_IMAGES := true
BOARD_MESA3D_USES_MESON_BUILD := true
BOARD_MESA3D_GALLIUM_DRIVERS := virgl
BOARD_MESA3D_VULKAN_DRIVERS := virtio
else
BOARD_GPU_DRIVERS := virgl
endif

# Kernel
BOARD_KERNEL_CMDLINE += \
    androidboot.console=hvc0 \
    androidboot.hardware=vboxware

ifneq ($(wildcard $(TARGET_KERNEL_SOURCE)/Makefile),)
TARGET_KERNEL_CONFIG += \
    lineageos/virtio.config
else ifneq ($(wildcard $(TARGET_PREBUILT_KERNEL_DIR)/kernel),)
BOARD_VENDOR_KERNEL_MODULES := \
    $(wildcard $(TARGET_PREBUILT_KERNEL_DIR)/*.ko)
else
VIRTUAL_DEVICE_KERNEL_MODULES_PATH := \
    kernel/prebuilts/common-modules/virtual-device/$(TARGET_PREBUILT_KERNEL_USE)/$(TARGET_PREBUILT_KERNEL_MODULES_ARCH)

BOARD_RECOVERY_KERNEL_MODULES := \
    $(wildcard $(VIRTUAL_DEVICE_KERNEL_MODULES_PATH)/*failover.ko) \
    $(wildcard $(VIRTUAL_DEVICE_KERNEL_MODULES_PATH)/nd_virtio.ko) \
    $(wildcard $(VIRTUAL_DEVICE_KERNEL_MODULES_PATH)/virtio*.ko)

BOARD_GENERIC_RAMDISK_KERNEL_MODULES := \
    $(wildcard $(KERNEL_ARTIFACTS_PATH)/*.ko) \
    $(wildcard $(VIRTUAL_DEVICE_KERNEL_MODULES_PATH)/*.ko)
endif

# Recovery
TARGET_RECOVERY_FSTAB := $(DEVICE_PATH)/config/fstab.vboxware
TARGET_RECOVERY_PIXEL_FORMAT := ARGB_8888

# SELinux
BOARD_VENDOR_SEPOLICY_DIRS += \
    $(DEVICE_PATH)/sepolicy/vendor

SYSTEM_EXT_PRIVATE_SEPOLICY_DIRS += $(DEVICE_PATH)/sepolicy/private
SYSTEM_EXT_PUBLIC_SEPOLICY_DIRS += $(DEVICE_PATH)/sepolicy/public
