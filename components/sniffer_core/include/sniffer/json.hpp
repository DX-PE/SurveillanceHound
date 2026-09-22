// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <string_view>
namespace sniffer {
// Bounded JSON reader shared by SD recovery and structured Wi-Fi advertisements.
class Json {
  public:
    enum Type : uint8_t { Object, Array, String, Number, Boolean, Null };
    struct Token {
        uint16_t start{}, size{}, after{};
        Type type{};
    };
    std::array<Token, 160> tokens{};
    uint16_t count{};
    bool parse(std::string_view input);
    int member(int object, std::string_view key) const;
    bool string(int token, std::span<char> out) const;
    bool integer(int token, int64_t &out) const;
    std::string_view raw(int token) const;

  private:
    std::string_view input_;
    size_t pos_{};
    void space();
    int value(unsigned depth);
};
} // namespace sniffer
