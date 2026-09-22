# SPDX-License-Identifier: Apache-2.0
idf_component_register(SRCS "board.cpp" INCLUDE_DIRS "include"
    REQUIRES esp_hw_support esp_driver_gpio esp_driver_spi esp_driver_ledc esp_driver_dac esp_adc fatfs sdmmc)
target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_20)
