// foo_discord_listening
// Copyright (C) 2026 Carinoasd
// SPDX-License-Identifier: MIT

#include <stdafx.h> // 角括號：測試工具會改用 tests/win_stub 的版本

#include "i18n.h"

#include <array>

namespace fdl {
namespace {

struct Entry {
    StringId id;
    const char* en;
    const char* zh_tw;
    const char* zh_cn;
    const char* ja;
};

// 每一列都標上 StringId，下方的 consteval 檢查會確保順序與 enum 一致，不會錯位。
constexpr Entry kStrings[] = {
    { StringId::type_listening, "Listening to", "正在聽（Listening to）", "正在听（Listening to）", "再生中（Listening to）" },
    { StringId::type_playing, "Playing", "正在玩（Playing）", "正在玩（Playing）", "プレイ中（Playing）" },
    { StringId::type_watching, "Watching", "正在看（Watching）", "正在看（Watching）", "視聴中（Watching）" },
    { StringId::status_line1, "Line 1 (song title)", "第一行（歌名）", "第一行（歌名）", "1 行目（曲名）" },
    { StringId::status_line2, "Line 2 (artist)", "第二行（歌手）", "第二行（歌手）", "2 行目（アーティスト）" },
    { StringId::status_app_name, "Application name", "應用程式名稱", "应用程序名称", "アプリ名" },
    { StringId::pause_keep, "Keep showing the song", "繼續顯示歌曲", "继续显示歌曲", "曲を表示し続ける" },
    { StringId::pause_clear, "Clear the status", "清除狀態", "清除状态", "ステータスを消す" },
    { StringId::stop_clear, "Clear the status", "清除狀態", "清除状态", "ステータスを消す" },
    { StringId::stop_keep, "Keep showing the last song", "保留最後一首歌", "保留最后一首歌", "最後の曲を表示し続ける" },
    { StringId::client_any, "Any Discord", "任何 Discord", "任意 Discord", "どの Discord でも" },

    { StringId::page_album_art, "Album art", "專輯封面", "专辑封面", "アルバムアート" },
    { StringId::page_links, "Links & filters", "連結與過濾", "链接与过滤", "リンクとフィルター" },
    { StringId::cache_count, "{} entries cached", "已快取 {} 筆", "已缓存 {} 条", "{} 件キャッシュ済み" },
    { StringId::preview_nothing, "Play a song to see a preview.", "播放歌曲即可在此預覽。", "播放歌曲即可在此预览。", "曲を再生するとここにプレビューが表示されます。" },
    { StringId::app_id_cue, "empty = built-in", "留空＝使用內建", "留空＝使用内置", "空欄＝内蔵" },

    { StringId::menu_show, "Show on Discord", "在 Discord 顯示", "在 Discord 显示", "Discord に表示" },
    { StringId::menu_show_desc, "Shows what you are playing as your Discord status.", "在 Discord 狀態顯示正在播放的歌曲。",
      "在 Discord 状态显示正在播放的歌曲。", "再生中の曲を Discord のステータスに表示します。" },
    { StringId::menu_settings, "Discord Listening settings", "Discord Listening 設定", "Discord Listening 设置", "Discord Listening の設定" },
    { StringId::menu_settings_desc, "Opens the Discord Listening preferences page.", "開啟 Discord Listening 的設定頁。",
      "打开 Discord Listening 的设置页。", "Discord Listening の設定ページを開きます。" },
    { StringId::ctx_refetch, "Re-fetch Discord album art", "重新抓取 Discord 專輯封面", "重新获取 Discord 专辑封面",
      "Discord のアルバムアートを再取得" },
    { StringId::ctx_refetch_desc, "Forgets the cached Discord album art of the selected tracks and looks it up again.",
      "清除所選曲目的 Discord 封面快取並重新查詢。", "清除所选曲目的 Discord 封面缓存并重新查询。",
      "選択した曲の Discord 用アルバムアートのキャッシュを消して、取得し直します。" },
    { StringId::ctx_set_art, "Set Discord album art...", "指定 Discord 專輯封面...", "指定 Discord 专辑封面...",
      "Discord のアルバムアートを指定..." },
    { StringId::ctx_set_art_desc, "Uses an image URL of your choice as the Discord album art for this album.",
      "以你指定的圖片網址作為這張專輯在 Discord 上的封面。", "以你指定的图片网址作为这张专辑在 Discord 上的封面。",
      "指定した画像 URL を、このアルバムの Discord 用アートとして使います。" },
    { StringId::ctx_no_album, "This track has no album information, so its art can't be set by album.",
      "這首曲目沒有專輯資訊，無法以專輯為單位指定封面。", "这首曲目没有专辑信息，无法以专辑为单位指定封面。",
      "この曲にはアルバム情報がないため、アルバム単位でアートを指定できません。" },

    { StringId::conn_disabled, "Disabled", "已停用", "已停用", "無効" },
    { StringId::conn_connecting, "Connecting...", "連線中...", "连接中...", "接続中..." },
    { StringId::conn_connected, "Connected", "已連線", "已连接", "接続済み" },
    { StringId::conn_disconnected, "Disconnected", "已斷線", "已断开", "切断されました" },
    { StringId::conn_not_running, "Discord is not running", "Discord 沒有在執行", "Discord 没有运行", "Discord が起動していません" },
    { StringId::icon_playing, "Playing", "播放中", "播放中", "再生中" },
    { StringId::icon_paused, "Paused", "已暫停", "已暂停", "一時停止" },
    { StringId::icon_stopped, "Stopped", "已停止", "已停止", "停止" },
    { StringId::update_available, "A new version is available: {}", "有新版本：{}", "有新版本：{}", "新しいバージョンがあります：{}" },

    { StringId::ui_enabled, "Show what I'm playing on Discord", "在 Discord 顯示正在播放的歌曲", "在 Discord 显示正在播放的歌曲",
      "再生中の曲を Discord に表示する" },
    { StringId::ui_activity, "Activity", "狀態", "状态", "アクティビティ" },
    { StringId::ui_type, "Type:", "類型：", "类型：", "種類：" },
    { StringId::ui_show_time, "Show progress bar", "顯示進度條", "显示进度条", "進行状況バーを表示" },
    { StringId::ui_status_shows, "Status shows:", "狀態列顯示：", "状态栏显示：", "ステータス表示：" },
    { StringId::ui_when_paused, "When paused:", "暫停時：", "暂停时：", "一時停止時：" },
    { StringId::ui_idle_prefix, "Clear after", "閒置", "闲置", "放置" },
    { StringId::ui_idle_suffix, "min", "分鐘後清除", "分钟后清除", "分後に消去" },
    { StringId::ui_when_stopped, "When stopped:", "停止時：", "停止时：", "停止時：" },
    { StringId::ui_text, "Text (title formatting)", "文字（title formatting）", "文字（title formatting）", "テキスト（title formatting）" },
    { StringId::ui_line1, "Line 1:", "第一行：", "第一行：", "1 行目：" },
    { StringId::ui_line2, "Line 2:", "第二行：", "第二行：", "2 行目：" },
    { StringId::ui_line3, "Line 3 / art tooltip:", "第三行／封面提示：", "第三行／封面提示：", "3 行目／アートの説明：" },
    { StringId::ui_preview, "Preview (now playing)", "預覽（目前播放）", "预览（当前播放）", "プレビュー（再生中の曲）" },
    { StringId::ui_icons, "Icons", "圖示", "图标", "アイコン" },
    { StringId::ui_small_icons, "Play / pause / stop icon", "播放／暫停／停止圖示", "播放／暂停／停止图标", "再生／一時停止／停止アイコン" },
    { StringId::ui_no_art_image, "Placeholder when there's no art", "沒有封面時顯示預設圖", "没有封面时显示默认图", "アートがない時の代替画像" },
    { StringId::ui_paused_text, "Add (Paused) / (Stopped) to line 1", "在第一行加上（已暫停）／（已停止）", "在第一行加上（已暂停）／（已停止）",
      "1 行目に（一時停止）／（停止）を付ける" },
    { StringId::ui_discord, "Discord", "Discord", "Discord", "Discord" },
    { StringId::ui_app_id, "Application ID:", "應用程式 ID：", "应用程序 ID：", "アプリ ID：" },
    { StringId::ui_status, "Status:", "連線狀態：", "连接状态：", "接続状態：" },

    { StringId::ui_art_enabled, "Show album art", "顯示專輯封面", "显示专辑封面", "アルバムアートを表示" },
    { StringId::ui_sources, "Sources (tried in this order)", "封面來源（依序嘗試）", "封面来源（依次尝试）", "取得元（上から順に試します）" },
    { StringId::ui_server, "Server:", "伺服器：", "服务器：", "サーバー：" },
    { StringId::ui_country, "Store country:", "商店國家：", "商店国家：", "ストアの国：" },
    { StringId::ui_api_key, "API key:", "API key：", "API key：", "API キー：" },
    { StringId::ui_use_upload, "Upload local art", "上傳本機封面", "上传本地封面", "ローカルのアートをアップロード" },
    { StringId::ui_command, "Command:", "指令：", "命令：", "コマンド：" },
    { StringId::ui_manual_note,
      "Art you set by hand (right-click > Utilities > Set Discord album art) always comes first. Empty server = musicbrainz.org.",
      "手動指定的封面（右鍵 > Utilities > 指定 Discord 專輯封面）一律最優先。伺服器留空＝musicbrainz.org。",
      "手动指定的封面（右键 > Utilities > 指定 Discord 专辑封面）始终最优先。服务器留空＝musicbrainz.org。",
      "手動で指定したアート（右クリック > Utilities > Discord のアルバムアートを指定）が常に最優先です。サーバーが空欄なら musicbrainz.org を使います。" },
    { StringId::ui_matching, "Matching", "比對", "匹配", "照合" },
    { StringId::ui_same_album, "Same album if equal:", "視為同一張專輯：", "视为同一张专辑：", "同じアルバムの条件：" },
    { StringId::ui_no_art_for, "No art for tracks:", "不抓封面的曲目：", "不获取封面的曲目：", "アートを取得しない曲：" },
    { StringId::ui_cache, "Cache", "快取", "缓存", "キャッシュ" },
    { StringId::ui_clear_cache, "Clear art cache", "清除封面快取", "清除封面缓存", "キャッシュを消去" },

    { StringId::ui_links_intro,
      "All fields use title formatting. Leave a field empty to turn it off. Spaces and non-ASCII characters in links are encoded automatically.",
      "所有欄位都使用 title formatting，留空即關閉。連結中的空白與中日文會自動編碼。",
      "所有字段都使用 title formatting，留空即关闭。链接中的空格与中日文会自动编码。",
      "すべての欄は title formatting を使います。空欄にすると無効です。リンク内の空白や日本語は自動でエンコードされます。" },
    { StringId::ui_links, "Links (opened when someone clicks the text or the album art)", "連結（別人點擊文字或封面時開啟）",
      "链接（别人点击文字或封面时打开）", "リンク（他の人が文字やアートをクリックした時に開く）" },
    { StringId::ui_line1_link, "Line 1 link:", "第一行連結：", "第一行链接：", "1 行目のリンク：" },
    { StringId::ui_line2_link, "Line 2 link:", "第二行連結：", "第二行链接：", "2 行目のリンク：" },
    { StringId::ui_art_link, "Album art link:", "封面連結：", "封面链接：", "アートのリンク：" },
    { StringId::ui_buttons, "Buttons (shown to other people, up to 2)", "按鈕（顯示給別人看，最多 2 個）", "按钮（显示给别人看，最多 2 个）",
      "ボタン（他の人に表示、最大 2 個）" },
    { StringId::ui_button1_text, "Button 1 text:", "按鈕 1 文字：", "按钮 1 文字：", "ボタン 1 の文字：" },
    { StringId::ui_button1_link, "Button 1 link:", "按鈕 1 連結：", "按钮 1 链接：", "ボタン 1 のリンク：" },
    { StringId::ui_button2_text, "Button 2 text:", "按鈕 2 文字：", "按钮 2 文字：", "ボタン 2 の文字：" },
    { StringId::ui_button2_link, "Button 2 link:", "按鈕 2 連結：", "按钮 2 链接：", "ボタン 2 のリンク：" },
    { StringId::ui_privacy, "Privacy", "隱私", "隐私", "プライバシー" },
    { StringId::ui_hide_tracks, "Hide tracks:", "不顯示的曲目：", "不显示的曲目：", "表示しない曲：" },
    { StringId::ui_query_note, "foobar2000 search syntax, e.g.  %genre% HAS podcast  OR  %path% HAS private",
      "foobar2000 搜尋語法，例如  %genre% HAS podcast  OR  %path% HAS private",
      "foobar2000 搜索语法，例如  %genre% HAS podcast  OR  %path% HAS private",
      "foobar2000 の検索構文。例：%genre% HAS podcast  OR  %path% HAS private" },
    { StringId::ui_hide_playlists, "Hide playlists:", "隱藏這些清單：", "隐藏这些列表：", "表示しないプレイリスト：" },
    { StringId::ui_only_playlists, "Only playlists:", "只顯示這些清單：", "只显示这些列表：", "表示するプレイリストのみ：" },
    { StringId::ui_playlist_note, "Playlist names separated by ;   * matches anything. Empty = no limit.",
      "清單名稱以 ; 分隔，* 代表任意文字。留空＝不限制。", "列表名称以 ; 分隔，* 代表任意文字。留空＝不限制。",
      "プレイリスト名を ; で区切ります。* は任意の文字列。空欄＝制限なし。" },
    { StringId::ui_example, "Example button: text  Search on YouTube   link  https://www.youtube.com/results?search_query=[%artist% ]%title%",
      "按鈕範例：文字  在 YouTube 搜尋   連結  https://www.youtube.com/results?search_query=[%artist% ]%title%",
      "按钮示例：文字  在 YouTube 搜索   链接  https://www.youtube.com/results?search_query=[%artist% ]%title%",
      "ボタンの例：文字  YouTube で検索   リンク  https://www.youtube.com/results?search_query=[%artist% ]%title%" },

    { StringId::ui_manual_caption, "Set Discord album art", "指定 Discord 專輯封面", "指定 Discord 专辑封面", "Discord のアルバムアートを指定" },
    { StringId::ui_manual_url_note, "Image URL (https://...). Leave empty to go back to automatic album art.",
      "圖片網址（https://...）。留空則改回自動抓取的封面。", "图片网址（https://...）。留空则改回自动获取的封面。",
      "画像の URL（https://...）。空欄にすると自動取得に戻ります。" },
    { StringId::ui_ok, "OK", "確定", "确定", "OK" },
    { StringId::ui_cancel, "Cancel", "取消", "取消", "キャンセル" },
};

static_assert(std::size(kStrings) == static_cast<size_t>(StringId::count_), "kStrings 與 StringId 數量不一致");

consteval bool TableMatchesEnum() {
    for (size_t i = 0; i < std::size(kStrings); ++i) {
        if (static_cast<size_t>(kStrings[i].id) != i) {
            return false;
        }
    }
    return true;
}
static_assert(TableMatchesEnum(), "kStrings 必須依 StringId 的順序、每個一列");

Language Detect() {
#ifdef FDL_FORCE_LANG
    return Language::FDL_FORCE_LANG;
#else
    const LANGID lang = GetUserDefaultUILanguage();
    switch (PRIMARYLANGID(lang)) {
    case LANG_CHINESE:
        switch (SUBLANGID(lang)) {
        case SUBLANG_CHINESE_TRADITIONAL:
        case SUBLANG_CHINESE_HONGKONG:
        case SUBLANG_CHINESE_MACAU: return Language::zh_tw;
        default: return Language::zh_cn;
        }
    case LANG_JAPANESE: return Language::ja;
    default: return Language::en;
    }
#endif
}

} // namespace

Language CurrentLanguage() {
    static const Language lang = Detect();
    return lang;
}

const char* Tr(StringId id, Language lang) {
    const auto& e = kStrings[static_cast<size_t>(id)];
    switch (lang) {
    case Language::zh_tw: return e.zh_tw;
    case Language::zh_cn: return e.zh_cn;
    case Language::ja: return e.ja;
    case Language::en: break;
    }
    return e.en;
}

const char* Tr(StringId id) {
    return Tr(id, CurrentLanguage());
}

std::string Format(StringId id, std::string_view value) {
    std::string s = Tr(id);
    if (const auto pos = s.find("{}"); pos != std::string::npos) {
        s.replace(pos, 2, value);
    }
    return s;
}

std::string Format(StringId id, size_t value) {
    return Format(id, std::to_string(value));
}

} // namespace fdl
