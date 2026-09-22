#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
from compile_signatures import validate

if __name__ == "__main__":
    print(f"Validated provenance for {len(validate())} rules")
