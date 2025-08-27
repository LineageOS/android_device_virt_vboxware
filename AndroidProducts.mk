#
# SPDX-FileCopyrightText: The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

PRODUCT_MAKEFILES := \
    $(LOCAL_DIR)/aosp_vboxware.mk \
    $(LOCAL_DIR)/lineage_vboxware.mk

$(foreach build_type, user userdebug eng, \
    $(eval COMMON_LUNCH_CHOICES += aosp_vboxware-$(build_type)) \
    $(eval COMMON_LUNCH_CHOICES += lineage_vboxware-$(build_type)))
