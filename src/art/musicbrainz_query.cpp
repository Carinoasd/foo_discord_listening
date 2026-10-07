// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

// MusicBrainz 查詢的純邏輯部分：不碰網路與 Windows API，可在 Linux 上單元測試。

#include "stdafx.h"

#include "art/http.h"
#include "art/musicbrainz.h"
#include "json_util.h"

#include <cctype>

namespace fdl::art {
namespace {

constexpr int kMinScore = 90;
constexpr std::string_view kSize = "front-500";

} // namespace

std::string AsciiLower(std::string_view text) {
    std::string out(text);
    for (auto& c : out) {
        if (static_cast<unsigned char>(c) < 0x80) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }
    return out;
}

bool IsMbid(std::string_view text) {
    if (text.size() != 36) {
        return false;
    }
    for (size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (c != '-') {
                return false;
            }
        } else if (!std::isxdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return true;
}

std::string EscapeLucene(std::string_view text) {
    static constexpr std::string_view kSpecial = R"(+-&|!(){}[]^"~*?:\/)";
    std::string out;
    for (char c : text) {
        if (kSpecial.find(c) != std::string_view::npos) {
            out += '\\';
        }
        out += c;
    }
    return out;
}

/// 全形英數符號（U+FF01–FF5E）轉半形、全形空白轉空白、波浪號「〜」轉「~」。
/// 日本的發行資料常混用全形與半形，例如「～」與「〜」、「ＡＢＣ」與「ABC」。
std::string FoldWidth(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (size_t i = 0; i < text.size();) {
        const auto c0 = static_cast<unsigned char>(text[i]);
        if (c0 == 0xE3 && i + 2 < text.size()) {
            const uint32_t cp = ((c0 & 0x0F) << 12) | ((static_cast<unsigned char>(text[i + 1]) & 0x3F) << 6) | (static_cast<unsigned char>(text[i + 2]) & 0x3F);
            if (cp == 0x3000) { out += ' '; i += 3; continue; }
            if (cp == 0x301C) { out += '~'; i += 3; continue; }
        }
        if (c0 == 0xEF && i + 2 < text.size()) {
            const uint32_t cp = ((c0 & 0x0F) << 12) | ((static_cast<unsigned char>(text[i + 1]) & 0x3F) << 6) | (static_cast<unsigned char>(text[i + 2]) & 0x3F);
            if (cp >= 0xFF01 && cp <= 0xFF5E) {
                out += static_cast<char>(cp - 0xFEE0);
                i += 3;
                continue;
            }
        }
        out += text[i];
        ++i;
    }
    return out;
}

std::string NormalizeTitle(std::string_view text) {
    std::string lower = AsciiLower(FoldWidth(text));
    // 把常見的全形或排版用符號換成 ASCII，讓「Don’t」和「Don't」視為相同。
    static const std::pair<std::string_view, std::string_view> kReplace[] = {
        { "\xE2\x80\x98", "'" }, { "\xE2\x80\x99", "'" }, { "\xE2\x80\x9C", "\"" }, { "\xE2\x80\x9D", "\"" },
        { "\xE2\x80\x90", "-" }, { "\xE2\x80\x93", "-" }, { "\xE2\x80\x94", "-" }, { "\xE2\x80\xA6", "..." },
    };
    for (const auto& [from, to] : kReplace) {
        for (size_t pos; (pos = lower.find(from)) != std::string::npos;) {
            lower.replace(pos, from.size(), to);
        }
    }
    std::string out;
    bool space = false;
    for (char c : lower) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            space = !out.empty();
            continue;
        }
        if (space) {
            out += ' ';
            space = false;
        }
        out += c;
    }
    return out;
}

std::string BuildReleaseGroupSearchUrl(std::string_view server, std::string_view artist, std::string_view album) {
    std::string base(server.empty() ? std::string_view("https://musicbrainz.org") : server);
    while (!base.empty() && base.back() == '/') {
        base.pop_back();
    }
    const std::string query = "releasegroup:\"" + EscapeLucene(album) + "\" AND artist:\"" + EscapeLucene(artist) + "\"";
    return base + "/ws/2/release-group/?fmt=json&limit=10&query=" + UrlEncode(query);
}

std::optional<std::string> PickReleaseGroup(const nlohmann::json& search, std::string_view album) {
    const auto groups = search.find("release-groups");
    if (groups == search.end() || !groups->is_array()) {
        return std::nullopt;
    }
    const auto wanted = NormalizeTitle(album);
    for (const auto& group : *groups) {
        if (!group.is_object()) {
            continue;
        }
        const auto score = json::GetInt(group, "score");
        const auto id = json::GetString(group, "id");
        const auto title = json::GetString(group, "title");
        // 只接受標題完全相符的結果，避免「SAILORWAVE III」被當成「SAILORWAVE」（上游 #95 一類的錯誤封面）。
        if (score >= kMinScore && IsMbid(id) && NormalizeTitle(title) == wanted) {
            return AsciiLower(id);
        }
    }
    return std::nullopt;
}

std::string CoverArtUrl(std::string_view entity, std::string_view mbid) {
    return "https://coverartarchive.org/" + std::string(entity) + "/" + AsciiLower(mbid) + "/" + std::string(kSize);
}

std::string UrlEncode(std::string_view text) {
    static constexpr char kHex[] = "0123456789ABCDEF";
    std::string out;
    out.reserve(text.size() * 3);
    for (unsigned char c : text) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out += static_cast<char>(c);
        } else {
            out += '%';
            out += kHex[c >> 4];
            out += kHex[c & 0xF];
        }
    }
    return out;
}

} // namespace fdl::art
