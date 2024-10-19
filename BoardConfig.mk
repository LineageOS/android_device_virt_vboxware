#
# Copyright (C) 2024 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

# Boot manager
TARGET_BOOT_MANAGER ?= linux_efi_stub

# Inherit from common
include device/virt/virt-common/BoardConfigVirtCommon.mk

USES_DEVICE_VIRT_VBOXWARE := true
DEVICE_PATH := device/virt/vboxware

# Arch
TARGET_CPU_ABI := x86_64
TARGET_ARCH := x86_64
TARGET_ARCH_VARIANT := sandybridge

# Boot manager
TARGET_EFI_BOOT_SCRIPTS := $(wildcard $(DEVICE_PATH)/bootmgr/linux_efi_stub/boot/*.nsh)
TARGET_EFI_INSTALL_SCRIPTS := $(wildcard $(DEVICE_PATH)/bootmgr/linux_efi_stub/install/*.nsh)
TARGET_GRUB_BOOT_CONFIG := $(DEVICE_PATH)/bootmgr/grub/grub-boot.cfg
TARGET_GRUB_INSTALL_CONFIG := $(DEVICE_PATH)/bootmgr/grub/grub-install.cfg
TARGET_REFIND_BOOT_CONFIG := $(DEVICE_PATH)/bootmgr/rEFInd/refind-boot.conf
TARGET_REFIND_INSTALL_CONFIG := $(DEVICE_PATH)/bootmgr/rEFInd/refind-install.conf

# Fstab
ifeq ($(AB_OTA_UPDATER),true)
$(call soong_config_set,VBOXWARE_FSTAB,PARTITION_SCHEME,ab)
else
$(call soong_config_set,VBOXWARE_FSTAB,PARTITION_SCHEME,a)
endif

# GRUB
TARGET_GRUB_ARCH := x86_64-efi

# Kernel
BOARD_KERNEL_CMDLINE += \
    8250.nr_uarts=1 \
    nomodeset \
    androidboot.console=ttyS0 \
    androidboot.graphics=swiftshader \
    androidboot.hardware=vboxware

BOARD_KERNEL_IMAGE_NAME := bzImage

ifneq ($(wildcard $(TARGET_KERNEL_SOURCE)/Makefile),)
TARGET_KERNEL_ARCH := x86
TARGET_KERNEL_CONFIG += \
    lineageos/vboxware.config
else ifneq ($(wildcard $(TARGET_PREBUILT_KERNEL_DIR)/kernel),)
BOARD_VENDOR_KERNEL_MODULES := \
    $(wildcard $(TARGET_PREBUILT_KERNEL_DIR)/*.ko)
endif

# Recovery
TARGET_RECOVERY_FSTAB := $(DEVICE_PATH)/configs/fstab/fstab.vboxware

# SELinux
BOARD_VENDOR_SEPOLICY_DIRS += \
    $(DEVICE_PATH)/sepolicy/vendor

ifeq ($(AB_OTA_UPDATER),true)
BOARD_VENDOR_SEPOLICY_DIRS += \
    $(DEVICE_PATH)/sepolicy/vendor/ab
else
BOARD_VENDOR_SEPOLICY_DIRS += \
    $(DEVICE_PATH)/sepolicy/vendor/a
endif

# VINTF
DEVICE_MANIFEST_FILE += \
    $(DEVICE_PATH)/configs/vintf/manifest.xml
