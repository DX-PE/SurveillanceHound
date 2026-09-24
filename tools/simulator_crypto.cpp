// SPDX-License-Identifier: Apache-2.0
#include "simulator_crypto.hpp"
#include <emscripten.h>

// Copy inputs before yielding; Asyncify resumes C++ only after the digest is ready.
EM_ASYNC_JS(int, browser_hmac,
            (const uint8_t *key, size_t key_size, const uint8_t *input, size_t input_size,
             uint8_t *output),
            {
                try {
                    const bytes = key_size ? HEAPU8.slice(key, key + key_size) : new Uint8Array(64);
                    const data = HEAPU8.slice(input, input + input_size);
                    const handle = await crypto.subtle.importKey(
                        'raw', bytes, {name : 'HMAC', hash : 'SHA-256'}, false, ['sign']);
                    const digest = await crypto.subtle.sign('HMAC', handle, data);
                    HEAPU8.set(new Uint8Array(digest), output);
                    return 1;
                } catch (error) {
                    return 0;
                }
            });
EM_ASYNC_JS(int, browser_sha256, (const uint8_t *input, size_t input_size, uint8_t *output), {
    try {
        const data = HEAPU8.slice(input, input + input_size);
        const digest = await crypto.subtle.digest('SHA-256', data);
        HEAPU8.set(new Uint8Array(digest), output);
        return 1;
    } catch (error) {
        return 0;
    }
});
