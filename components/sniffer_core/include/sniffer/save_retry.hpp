// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>
namespace sniffer {
// Tracks queue acceptance, not persistence. Retry the latest UI-owned state;
// do not allocate another saved-state buffer when the storage worker is busy.
struct SaveRetry {
    bool pending{}, backup_requested{};
    uint64_t attempted{}, accepted{};
    bool due(uint64_t now) const {
        const auto previous = pending ? attempted : accepted;
        return now >= previous && now - previous >= (pending ? 1000U : 60000U);
    }
    void result(uint64_t now, bool queued) {
        attempted = now;
        pending = !queued;
        if (queued) {
            accepted = now;
            backup_requested = false;
        }
    }
};
} // namespace sniffer
