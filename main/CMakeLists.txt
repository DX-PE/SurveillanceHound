# SPDX-License-Identifier: Apache-2.0
idf_component_register(SRCS "app_main.cpp" INCLUDE_DIRS "."
    REQUIRES sniffer_core board_e32r40t radio storage ui esp_timer esp_system bootloader_support)
target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_20)
