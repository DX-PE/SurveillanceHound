# SPDX-License-Identifier: Apache-2.0
idf_component_register(SRCS "storage.cpp" INCLUDE_DIRS "include"
    REQUIRES sniffer_core board_e32r40t nvs_flash esp_timer esp_system mbedtls)
target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_20)
