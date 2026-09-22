# SPDX-License-Identifier: Apache-2.0
idf_component_register(SRCS "parsers.cpp" "json.cpp" "utilities.cpp" "engine.cpp" "privacy.cpp" "records.cpp"
    INCLUDE_DIRS "include" "../../generated" REQUIRES mbedtls)
target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_20)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
