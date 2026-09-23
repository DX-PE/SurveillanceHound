// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "state.hpp"
#include <atomic>
#include <type_traits>
#include <variant>

namespace sniffer::storage::detail {
enum class Op : uint8_t {
    Record,
    Save,
    Export,
    Eject,
    Diagnostics,
    Sound,
    Clear,
    Sleep,
    BackupIgnores
};
struct Record {
    Detection detection{};
    std::array<char, 21> token{};
    bool research{};
    uint8_t region{};
};
struct Message {
    Op op{};
    uint8_t value{}; // Snapshot slot for Save/Sleep; effect number for Sound.
    std::variant<std::monostate, Record, std::array<char, 256>> payload{};
};
// FreeRTOS copies queue entries as bytes. Never add ownership-bearing payloads here.
static_assert(std::is_trivially_copyable_v<Message>);
static_assert(sizeof(Message) <= 288);

class Snapshots {
    struct Slot {
        std::atomic<bool> busy{};
        State state{};
    };
    std::array<Slot, 2> slots_{};

  public:
    // Claim before publishing the queue message; retain until persistence finishes.
    int claim(const State &state) {
        for (size_t i = 0; i < slots_.size(); ++i) {
            bool expected = false;
            if (slots_[i].busy.compare_exchange_strong(expected, true, std::memory_order_acquire)) {
                slots_[i].state = state;
                return int(i);
            }
        }
        return -1; // Backpressure, never overwrite an in-flight snapshot.
    }
    const State &get(size_t slot) const {
        return slots_[slot].state;
    }
    void release(size_t slot) {
        slots_[slot].busy.store(false, std::memory_order_release);
    }
};
// Reuse one of the two existing immutable slots while SD history is checked.
// A newer successful NVS save replaces the deferred snapshot without growing RAM.
struct DeferredBackup {
    int slot{-1};
    bool manual{};
    void retain(Snapshots &snapshots, unsigned next, bool requested) {
        if (slot >= 0)
            snapshots.release(size_t(slot));
        slot = int(next);
        manual |= requested;
    }
    void release(Snapshots &snapshots) {
        if (slot >= 0)
            snapshots.release(size_t(slot));
        slot = -1;
        manual = false;
    }
};
} // namespace sniffer::storage::detail
