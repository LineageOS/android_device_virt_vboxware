#
# Copyright (C) 2024 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

# Inherit from common
$(call inherit-product, device/virt/virt-common/virt-common.mk)

# Graphics (Composer)
PRODUCT_PACKAGES += \
    android.hardware.graphics.composer@2.1-service

# Graphics (Gralloc)
PRODUCT_PACKAGES += \
    android.hardware.graphics.allocator@2.0-impl \
    android.hardware.graphics.allocator@2.0-service \
    android.hardware.graphics.mapper@2.0-impl-2.1 \
    gralloc.drm

PRODUCT_VENDOR_PROPERTIES += \
    ro.hardware.gralloc=drm

# Graphics (Mesa)
PRODUCT_PACKAGES += \
    libGLES_mesa

PRODUCT_SOONG_NAMESPACES += \
    external/mesa3d

# Init
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/init/init.vboxware.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.vboxware.rc

PRODUCT_PACKAGES += \
    fstab.vboxware

# Input
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/.emptyfile:$(TARGET_COPY_OUT_VENDOR)/usr/keylayout/VirtualBox_mouse_integration.kl \
    $(LOCAL_PATH)/configs/.emptyfile:$(TARGET_COPY_OUT_VENDOR)/usr/keylayout/VirtualBox_USB_Tablet.kl

# Kernel
TARGET_KERNEL_SOURCE := kernel/virt/virtio-old
ifneq ($(wildcard $(TARGET_KERNEL_SOURCE)/Makefile),)
    $(warning Using source built kernel)
endif

# Recovery
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/init/init.recovery.vboxware.rc:$(TARGET_COPY_OUT_RECOVERY)/root/init.recovery.vboxware.rc

# Soong namespaces
PRODUCT_SOONG_NAMESPACES += \
    $(LOCAL_PATH)

# Vendor ramdisk
PRODUCT_PACKAGES += \
    fstab.vboxware.vendor_ramdisk

# Vendor service manager
PRODUCT_PACKAGES += \
    vndservicemanager
