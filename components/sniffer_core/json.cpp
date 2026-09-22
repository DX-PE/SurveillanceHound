// SPDX-License-Identifier: Apache-2.0
#include "sniffer/json.hpp"
#include <charconv>
#include <cstring>
namespace sniffer {
void Json::space() {
    while (pos_ < input_.size() && (input_[pos_] == ' ' || input_[pos_] == '\n' ||
                                    input_[pos_] == '\r' || input_[pos_] == '\t'))
        ++pos_;
}
std::string_view Json::raw(int t) const {
    return t >= 0 && t < count ? input_.substr(tokens[t].start, tokens[t].size)
                               : std::string_view{};
}
bool Json::string(int t, std::span<char> out) const {
    if (t < 0 || t >= count || tokens[t].type != String || out.empty())
        return false;
    auto s = raw(t);
    size_t j = 0;
    for (size_t i = 1; i + 1 < s.size(); ++i) {
        unsigned char c = s[i];
        if (c == '\\') {
            c = s[++i];
            switch (c) {
            case 'n':
                c = '\n';
                break;
            case 'r':
                c = '\r';
                break;
            case 't':
                c = '\t';
                break;
            case 'b':
                c = '\b';
                break;
            case 'f':
                c = '\f';
                break;
            case 'u': {
                unsigned n = 0;
                for (int k = 0; k < 4; ++k) {
                    char h = s[++i];
                    n = n * 16 + (h <= '9' ? h - '0' : (h | 32) - 'a' + 10);
                }
                if (n == 0 || n > 127)
                    return false;
                c = n;
                break;
            }
            default:
                break;
            }
        }
        if (j + 1 >= out.size() || c == 0)
            return false;
        out[j++] = char(c);
    }
    out[j] = 0;
    return true;
}
int Json::member(int object, std::string_view key) const {
    if (object < 0 || object >= count || tokens[object].type != Object)
        return -1;
    for (int i = object + 1; i < tokens[object].after;) {
        char name[80];
        int v = i + 1;
        if (string(i, name) && key == name)
            return v;
        i = tokens[v].after;
    }
    return -1;
}
bool Json::integer(int t, int64_t &out) const {
    if (t < 0 || t >= count || tokens[t].type != Number)
        return false;
    auto s = raw(t);
    auto r = std::from_chars(s.data(), s.data() + s.size(), out);
    return r.ec == std::errc{} && r.ptr == s.data() + s.size();
}
int Json::value(unsigned depth) {
    space();
    if (depth > 8 || pos_ >= input_.size() || count >= tokens.size())
        return -1;
    int t = count++;
    auto &tok = tokens[t];
    tok.start = pos_;
    char c = input_[pos_++];
    if (c == '{' || c == '[') {
        tok.type = c == '{' ? Object : Array;
        char close = c == '{' ? '}' : ']';
        space();
        if (pos_ < input_.size() && input_[pos_] == close)
            ++pos_;
        else
            for (;;) {
                if (c == '{') {
                    space();
                    if (pos_ >= input_.size() || input_[pos_] != '"')
                        return -1;
                    int k = value(depth + 1);
                    if (k < 0)
                        return -1;
                    char name[80];
                    if (!string(k, name))
                        return -1;
                    for (int i = t + 1; i < k;) {
                        char prev[80];
                        if (!string(i, prev) || std::strcmp(prev, name) == 0)
                            return -1;
                        i = tokens[i + 1].after;
                    }
                    space();
                    if (pos_ >= input_.size() || input_[pos_++] != ':')
                        return -1;
                }
                if (value(depth + 1) < 0)
                    return -1;
                space();
                if (pos_ >= input_.size())
                    return -1;
                char next = input_[pos_++];
                if (next == close)
                    break;
                if (next != ',')
                    return -1;
            }
    } else if (c == '"') {
        tok.type = String;
        bool closed = false;
        while (pos_ < input_.size()) {
            unsigned char x = input_[pos_++];
            if (x == '"') {
                closed = true;
                break;
            }
            if (x < 32 || x >= 128)
                return -1;
            if (x == '\\') {
                if (pos_ >= input_.size())
                    return -1;
                char e = input_[pos_++];
                if (e == 'u') {
                    for (int i = 0; i < 4; ++i) {
                        if (pos_ >= input_.size())
                            return -1;
                        char h = input_[pos_++];
                        if (!((h >= '0' && h <= '9') || (h >= 'a' && h <= 'f') ||
                              (h >= 'A' && h <= 'F')))
                            return -1;
                    }
                } else if (std::string_view("\"\\/bfnrt").find(e) == std::string_view::npos)
                    return -1;
            }
        }
        if (!closed)
            return -1;
    } else if (c == 't' || c == 'f' || c == 'n') {
        std::string_view word = c == 't' ? "true" : c == 'f' ? "false" : "null";
        --pos_;
        if (input_.substr(pos_, word.size()) != word)
            return -1;
        pos_ += word.size();
        tok.type = c == 'n' ? Null : Boolean;
    } else {
        --pos_;
        tok.type = Number;
        if (input_[pos_] == '-')
            ++pos_;
        if (pos_ >= input_.size())
            return -1;
        auto digit = [&] {
            return pos_ < input_.size() && input_[pos_] >= '0' && input_[pos_] <= '9';
        };
        if (input_[pos_] == '0')
            ++pos_;
        else {
            if (!digit())
                return -1;
            while (digit())
                ++pos_;
        }
        if (pos_ < input_.size() && input_[pos_] == '.') {
            ++pos_;
            if (!digit())
                return -1;
            while (digit())
                ++pos_;
        }
        if (pos_ < input_.size() && (input_[pos_] == 'e' || input_[pos_] == 'E')) {
            ++pos_;
            if (pos_ < input_.size() && (input_[pos_] == '+' || input_[pos_] == '-'))
                ++pos_;
            if (!digit())
                return -1;
            while (digit())
                ++pos_;
        }
    }
    tok.size = pos_ - tok.start;
    tok.after = count;
    return t;
}
bool Json::parse(std::string_view input) {
    input_ = input;
    pos_ = count = 0;
    if (input.empty() || input.size() > 2048)
        return false;
    if (value(0) < 0)
        return false;
    space();
    return pos_ == input_.size();
}
} // namespace sniffer
