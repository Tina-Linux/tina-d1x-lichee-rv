$(call inherit-product-if-exists, target/allwinner/d1-h-common/d1-h-common.mk)

PRODUCT_PACKAGES +=

PRODUCT_COPY_FILES +=

PRODUCT_AAPT_CONFIG := large xlarge hdpi xhdpi
PRODUCT_AAPT_PERF_CONFIG := xhdpi
PRODUCT_CHARACTERISTICS := musicbox

PRODUCT_BRAND := allwinner
PRODUCT_NAME := d1-h_lichee_rv
PRODUCT_DEVICE := d1-h-lichee_rv
PRODUCT_MODEL := Allwinner d1 dev board
