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

std::string NormalizeTitle(std::string_view text) {
    std::string lower = AsciiLower(text);
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

std::string BuildReleaseGroupSearchUrl(std::string_view artist, std::string_view album) {
    const std::string query = "releasegroup:\"" + EscapeLucene(album) + "\" AND artist:\"" + EscapeLucene(artist) + "\"";
    return "https://musicbrainz.org/ws/2/release-group/?fmt=json&limit=10&query=" + UrlEncode(query);
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
