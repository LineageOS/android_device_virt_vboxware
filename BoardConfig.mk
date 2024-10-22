#
# Copyright (C) 2024 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

# Boot manager
TARGET_BOOT_MANAGER := grub

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
TARGET_GRUB_BOOT_CONFIGS += $(DEVICE_PATH)/bootmgr/grub/grub-boot.cfg
TARGET_REFIND_BOOT_CONFIG := $(DEVICE_PATH)/bootmgr/rEFInd/refind-boot.conf
TARGET_REFIND_INSTALL_CONFIG := $(DEVICE_PATH)/bootmgr/rEFInd/refind-install.conf

# Fstab
ifeq ($(AB_OTA_UPDATER),true)
$(call soong_config_set,VBOXWARE_FSTAB,PARTITION_SCHEME,ab)
else
$(call soong_config_set,VBOXWARE_FSTAB,PARTITION_SCHEME,a)
endif

# Graphics (Mesa)
BOARD_GPU_DRIVERS := vmwgfx
BOARD_USES_DRM_GRALLOC := true

# GRUB
TARGET_GRUB_ARCH := x86_64-efi

# Kernel
BOARD_KERNEL_CMDLINE += \
    8250.nr_uarts=1 \
    console=tty0 \
    androidboot.console=ttyS0 \
    androidboot.hardware=vboxware \
    androidboot.partition_map=sdb,userdata \
    audit=0 androidboot.selinux=permissive androidboot.insecure_adb=1

BOARD_KERNEL_IMAGE_NAME := bzImage

ifneq ($(wildcard $(TARGET_KERNEL_SOURCE)/Makefile),)
TARGET_KERNEL_ARCH := x86

KERNEL_VERSION := $(shell grep -s "^VERSION = " $(TARGET_KERNEL_SOURCE)/Makefile | awk '{ print $$3 }')
KERNEL_PATCHLEVEL := $(shell grep -s "^PATCHLEVEL = " $(TARGET_KERNEL_SOURCE)/Makefile | awk '{ print $$3 }')
ifeq ($(KERNEL_VERSION),4)
    ifeq ($(KERNEL_PATCHLEVEL),19)
        TARGET_KERNEL_CONFIG := gki_defconfig
    else
        TARGET_KERNEL_CONFIG := x86_64_cuttlefish_defconfig
    endif
else ifeq ($(KERNEL_VERSION),5)
    TARGET_KERNEL_CONFIG := gki_defconfig
endif
TARGET_KERNEL_CONFIG += \
    lineageos/vboxware.config \
    lineageos/feature/fbcon.config
endif

# Recovery
TARGET_RECOVERY_FSTAB_GENRULE := gen_fstab_vboxware
TARGET_RECOVERY_PIXEL_FORMAT := BGRX_8888

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
