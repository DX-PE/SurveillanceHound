# SPDX-License-Identifier: Apache-2.0
idf_component_register(SRCS "ui.cpp" INCLUDE_DIRS "include" REQUIRES sniffer_core)
target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_20)
