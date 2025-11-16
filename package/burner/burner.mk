################################################################################
#
# burner
#
################################################################################

BURNER_VERSION = 0.1
BURNER_SITE = $(BR2_EXTERNAL_BEAGLEBURNER_PATH)/package/burner/src
BURNER_SITE_METHOD = local
BURNER_LICENSE = MIT
BURNER_LICENSE_FILES = LICENSE

BURNER_DEPENDENCIES = \
	libevdev

$(eval $(meson-package))
