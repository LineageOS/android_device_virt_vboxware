#
# SPDX-FileCopyrightText: The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

# Inherit from common
$(call inherit-product, device/virt/virt-common/virt-common.mk)

DEVICE_PATH := device/virt/vboxware

# Graphics (Allocator)
TARGET_GRAPHICS_ALLOCATOR_HAL := minigbm-upstream
TARGET_MINIGBM_PLATFORM := vmwgfx

# Graphics (Composer)
TARGET_GRAPHICS_COMPOSER_HAL := drm_hwcomposer

# Init
PRODUCT_COPY_FILES += \
    $(DEVICE_PATH)/configs/init/init.vboxware.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.vboxware.rc

PRODUCT_PACKAGES += \
    fstab.vboxware

# Input
PRODUCT_COPY_FILES += \
    $(DEVICE_PATH)/configs/input/excluded-input-devices.xml:$(TARGET_COPY_OUT_VENDOR)/etc/excluded-input-devices.xml

PRODUCT_PACKAGES += \
    vboxware_tablet2multitouch.idc

# Kernel
TARGET_PREBUILT_KERNEL_ARCH ?= x86_64
TARGET_PREBUILT_KERNEL_USE ?= 6.18
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

# Shipping API level
PRODUCT_SHIPPING_API_LEVEL := 33

# Soong namespaces
PRODUCT_SOONG_NAMESPACES += \
    $(DEVICE_PATH)

# Tablet to multitouch
PRODUCT_PACKAGES += \
    tablet2multitouch_vboxware \
    tablet2multitouch_vboxware_recovery

# Vendor ramdisk
PRODUCT_PACKAGES += \
    fstab.vboxware.vendor_ramdisk
