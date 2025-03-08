#
# Copyright (C) 2024 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

# Inherit from common
$(call inherit-product, device/virt/virt-common/virt-common.mk)

DEVICE_PATH := device/virt/vboxware

# Graphics (Allocator)
PRODUCT_PACKAGES += \
    android.hardware.graphics.allocator-service.minigbm_vboxware \
    android.hardware.graphics.mapper@4.0-impl.minigbm_vboxware \
    gralloc.minigbm_vboxware \
    mapper.minigbm_vboxware

PRODUCT_PACKAGES += \
    android.hardware.graphics.allocator@2.0-impl \
    android.hardware.graphics.allocator@2.0-service \
    android.hardware.graphics.mapper@2.0-impl-2.1

# Graphics (Composer)
PRODUCT_PACKAGES += \
    android.hardware.graphics.composer@2.1-service \
    android.hardware.graphics.composer@2.4-service \
    hwcomposer.drm

# Graphics (Mesa)
PRODUCT_PACKAGES += \
    mesa3d

# Init
PRODUCT_COPY_FILES += \
    $(DEVICE_PATH)/configs/init/init.vboxware.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.vboxware.rc

PRODUCT_PACKAGES += \
    fstab.vboxware

# Input
PRODUCT_COPY_FILES += \
    $(DEVICE_PATH)/../virt-common/configs/input/virt_tablet2multitouch.idc:$(TARGET_COPY_OUT_VENDOR)/usr/idc/vboxware_vboxtablet2multitouch.idc \
    $(DEVICE_PATH)/configs/input/excluded-input-devices.xml:$(TARGET_COPY_OUT_VENDOR)/etc/excluded-input-devices.xml

# Kernel
TARGET_PREBUILT_KERNEL_ARCH ?= x86_64
TARGET_PREBUILT_KERNEL_USE ?= 6.1
TARGET_PREBUILT_KERNEL_DIR := device/virt/kernel-vboxware/$(TARGET_PREBUILT_KERNEL_USE)/$(TARGET_PREBUILT_KERNEL_ARCH)
TARGET_KERNEL_SOURCE := kernel/virt/virtio
ifneq ($(wildcard $(TARGET_KERNEL_SOURCE)/Makefile),)
    $(warning Using source built kernel)
else ifneq ($(wildcard $(TARGET_PREBUILT_KERNEL_DIR)/kernel),)
    PRODUCT_COPY_FILES += $(TARGET_PREBUILT_KERNEL_DIR)/kernel:kernel
    $(warning Using prebuilt kernel from $(TARGET_PREBUILT_KERNEL_DIR)/kernel)
endif

# Recovery
PRODUCT_COPY_FILES += \
    $(DEVICE_PATH)/configs/init/init.recovery.vboxware.rc:$(TARGET_COPY_OUT_RECOVERY)/root/init.recovery.vboxware.rc

# Soong namespaces
PRODUCT_SOONG_NAMESPACES += \
    $(DEVICE_PATH)

# Tablet to multitouch
PRODUCT_PACKAGES += \
    vboxtablet2multitouch \
    vboxtablet2multitouch_recovery

# Vendor ramdisk
PRODUCT_PACKAGES += \
    fstab.vboxware.vendor_ramdisk

# Vendor service manager
PRODUCT_PACKAGES += \
    vndservicemanager
