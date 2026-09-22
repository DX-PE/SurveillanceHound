# SPDX-License-Identifier: Apache-2.0
idf_component_register(SRCS "radio.cpp" INCLUDE_DIRS "include"
    REQUIRES sniffer_core esp_wifi esp_event esp_timer bt esp_system)
target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_20)
