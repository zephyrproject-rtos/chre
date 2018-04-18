#
# IMU Cal Nanoapp Makefile
#

# Environment Checks ###########################################################

ifneq ($(ANDROID_BUILD_TOP),)
CHRE_PREFIX = $(ANDROID_BUILD_TOP)/system/chre
else
$(error "You must run 'lunch' to setup ANDROID_BUILD_TOP, or explicitly define \
         the CHRE_PREFIX environment variable to point to the CHRE root \
         directory.")
endif

# Nanoapp Configuration ########################################################

NANOAPP_NAME = imu_cal
NANOAPP_ID = 0x476f6f676c000016
NANOAPP_VERSION = 0x01000001 # Version scheme: 0xMMmmpppp
NANOAPP_IS_SYSTEM_NANOAPP = 1

NANOAPP_NAME_STRING = \"IMU\ Cal\"

ALGOS_DIR = $(ANDROID_BUILD_TOP)/device/google/contexthub/firmware/os/algos

# Common Compiler Flags ########################################################

# Include paths.
COMMON_CFLAGS += -I.
COMMON_CFLAGS += -I$(ALGOS_DIR)
COMMON_CFLAGS += -I$(CHRE_PREFIX)/ash/include/ash_api

# Defines.
COMMON_CFLAGS += -DNANOAPP_MINIMUM_LOG_LEVEL=CHRE_LOG_LEVEL_DEBUG

# Calibration algorithm enable flags.
COMMON_CFLAGS += -DACCEL_CAL_ENABLED
COMMON_CFLAGS += -DDIVERSITY_CHECK_ENABLED
COMMON_CFLAGS += -DGYRO_CAL_ENABLED
COMMON_CFLAGS += -DMAG_CAL_ENABLED

# Debug testing flags.
#COMMON_CFLAGS += -DACCEL_CAL_DBG_ENABLED
#COMMON_CFLAGS += -DDIVERSE_DEBUG_ENABLE
#COMMON_CFLAGS += -DGYRO_CAL_DBG_ENABLED
#COMMON_CFLAGS += -DMAG_CAL_DEBUG_ENABLE
COMMON_CFLAGS += -DNANO_SENSOR_CAL_DBG_ENABLED
#COMMON_CFLAGS += -DOVERTEMPCAL_DBG_ENABLED
#COMMON_CFLAGS += -DSYNCHRONIZER_DBG_ENABLED

# Debug options.
#COMMON_CFLAGS += -DIMU_TEMP_DBG_ENABLED
#COMMON_CFLAGS += -DOVERTEMPCAL_DBG_LOG_TEMP

# Common Source Files ##########################################################

# C++ Source Code
COMMON_SRCS += imu_cal.cc
COMMON_SRCS += $(ALGOS_DIR)/calibration/nano_calibration/nano_calibration.cc
COMMON_SRCS += $(ALGOS_DIR)/calibration/online_calibration/accelerometer/accel_offset_cal/accel_offset_cal.cc
COMMON_SRCS += $(ALGOS_DIR)/calibration/online_calibration/common_data/calibration_data.cc
COMMON_SRCS += $(ALGOS_DIR)/calibration/online_calibration/gyroscope/gyro_offset_over_temp_cal/gyro_offset_over_temp_cal.cc
COMMON_SRCS += $(ALGOS_DIR)/calibration/online_calibration/magnetometer/mag_diverse_cal/mag_diverse_cal.cc

# C Source Code
COMMON_SRCS += $(ALGOS_DIR)/calibration/accelerometer/accel_cal.c
COMMON_SRCS += $(ALGOS_DIR)/calibration/diversity_checker/diversity_checker.c
COMMON_SRCS += $(ALGOS_DIR)/calibration/gyroscope/gyro_cal.c
COMMON_SRCS += $(ALGOS_DIR)/calibration/gyroscope/gyro_stillness_detect.c
COMMON_SRCS += $(ALGOS_DIR)/calibration/magnetometer/mag_cal.c
COMMON_SRCS += $(ALGOS_DIR)/calibration/over_temp/over_temp_cal.c
COMMON_SRCS += $(ALGOS_DIR)/calibration/sample_rate_estimator/sample_rate_estimator.c
COMMON_SRCS += $(ALGOS_DIR)/common/math/kasa.c
COMMON_SRCS += $(ALGOS_DIR)/common/math/levenberg_marquardt.c
COMMON_SRCS += $(ALGOS_DIR)/common/math/mat.c
COMMON_SRCS += $(ALGOS_DIR)/common/math/quat.c
COMMON_SRCS += $(ALGOS_DIR)/common/math/vec.c

# Makefile Includes ############################################################

include $(CHRE_PREFIX)/build/nanoapp/app.mk
