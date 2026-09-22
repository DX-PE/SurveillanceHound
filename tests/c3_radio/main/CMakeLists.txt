# SPDX-License-Identifier: Apache-2.0
idf_component_register(SRCS "main.cpp" INCLUDE_DIRS "." REQUIRES nvs_flash sniffer_core radio esp_timer)
target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_20)
