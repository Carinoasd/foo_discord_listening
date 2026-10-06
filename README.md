# foo_discord_listening

**繁體中文** · [English](README.en.md)

> 在 Discord 上顯示你正在用 foobar2000 聽什麼，狀態是「Listening to」，附進度條與專輯封面。
>
> Shows what you're playing in foobar2000 as a Discord "Listening to" activity, with a progress bar and album art.

[![Build](https://github.com/Carinoasd/foo_discord_listening/actions/workflows/build.yml/badge.svg)](https://github.com/Carinoasd/foo_discord_listening/actions/workflows/build.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

這是一個從零重寫的 foobar2000 元件，靈感來自 [foo_discord_rich](https://github.com/TheQwertiest/foo_discord_rich)。原版自 2024 年起就沒有再維護，累積了不少問題，例如時間軸不動、封面不顯示、設定重開後被還原。這個版本參考了原版所有 issue 的討論重新設計，沒有沿用原版的程式碼。

---

## 功能

- **Listening to 狀態**：成員清單直接顯示歌名，不再只是「foobar2000」。也可以改成顯示歌手或應用程式名稱。
- **進度條**：換曲、拖動進度、暫停後繼續都會重新對時。
- **專輯封面**：
  - 優先用標籤裡的 MusicBrainz ID；沒有的話搜尋 MusicBrainz，並核對專輯名稱，避免抓錯封面。
  - 也可以把本機封面（外部圖檔或內嵌）縮圖後，交給你指定的上傳指令。
- **三行文字都能自訂**，使用 foobar2000 的 title formatting 語法。
- **快取可以清除**：設定頁有「清除封面快取」，右鍵選單有「Re-fetch Discord album art」。
- **不會拖慢 foobar2000**：
  - 與 Discord 的連線和網路請求都在背景執行緒進行，每個請求都有逾時。
  - Discord 沒開時會自動重試連線。
  - 關閉 foobar2000 時會立即中斷進行中的請求。
- 支援 foobar2000 2.x 的 x64 與 x86 版本，設定頁支援深色模式。

## 安裝

1. 到 [Releases](https://github.com/Carinoasd/foo_discord_listening/releases) 下載 `foo_discord_listening-x.y.z.fb2k-component`。
2. 在 foobar2000 開啟 **File > Preferences > Components**，把檔案拖進去，按 **Apply** 後重新啟動。
3. 開啟 **File > Preferences > Tools > Discord Listening**，填入 Discord application ID（見下一節）。

> 如果你同時裝了 foo_discord_rich，請先移除。兩個元件會同時更新 Discord 狀態，互相覆蓋。

### Discord application ID

Discord 顯示的「Listening to **XXX**」，其中 XXX 是 Discord 應用程式的名稱。所以你需要一個應用程式：

1. 到 [Discord Developer Portal](https://discord.com/developers/applications) 按 **New Application**，名稱取為你想顯示的字（例如 `foobar2000`）。
2. 複製 **Application ID**（一串數字），貼到設定頁的 *Application ID* 欄位，按 **Apply**。

Application ID 不是密碼，可以公開。

## 設定說明

| 設定 | 預設 | 說明 |
| --- | --- | --- |
| Type | Listening to | 也可改成 Playing / Watching |
| Status shows | Line 1 | 成員清單與個人狀態列顯示的欄位 |
| When paused | Keep showing the song | 暫停時保留歌曲資訊但不顯示時間，或直接清除狀態 |
| Show progress bar | 開 | 時間由「目前時間 − 已播放時間」推算，換曲、拖動、繼續播放時重新計算 |
| Line 1 / 2 / 3 | `[%title%]` / `[%artist%]` / `[%album%]` | 第三行同時是封面的滑鼠提示 |
| Album art source | MusicBrainz | 也可改成「先 MusicBrainz，找不到再上傳」或「只上傳」 |
| Upload once per | `$if([%album%],[%album artist%]\|[%album%],%path%)` | 同一張專輯只上傳一次 |

### 上傳本機封面

MusicBrainz 查不到的專輯（例如同人音樂、遊戲原聲）可以改用上傳。指令中的 `{path}` 會替換成縮好的 JPEG 檔路徑；沒寫 `{path}` 時，路徑會從 stdin 傳入，這和 foo_discord_rich 的上傳腳本相容。指令輸出的第一個 `https://` 網址會被當作封面。

範例（Windows 10 以上內建 curl，上傳到 catbox.moe）：

```
curl -s -F reqtype=fileupload -F fileToUpload=@{path} https://catbox.moe/user/api.php
```

> 上傳等於把封面公開到第三方網站。請選擇會長期保存檔案的服務：快取會記住網址，檔案過期後封面就會失效。

## 疑難排解

- **完全沒顯示**
  - 到 Discord 的 **使用者設定 > 活動隱私** 開啟「與其他人分享你的活動狀態」。
  - 確認 Discord 與 foobar2000 用相同權限執行：以系統管理員身分執行的 Discord，一般權限的程式連不到。
  - 不要在 Discord 裡把 foobar2000 手動加成「遊戲」。
- **封面是問號或錯的**：對該曲目按右鍵 **Utilities > Re-fetch Discord album art**。若是 MusicBrainz 搜尋錯誤，替檔案加上 `MUSICBRAINZ_ALBUMID` 標籤最準確。
- **詳細紀錄**：**View > Console**，所有訊息都以 `[foo_discord_listening]` 開頭。
- 自己的個人檔案上看不到進度條是 Discord 的限制，別人看得到。

## 從原始碼建置

需要 Visual Studio 2022（或 Build Tools）、CMake 3.25 以上和 Ninja。依賴項目（foobar2000 SDK、WTL、nlohmann/json）會在設定時自動下載，並驗證 SHA256。

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

在 WSL 裡開發的話，`scripts/build.sh [Release|Debug] [x64|x86]` 會呼叫 Windows 端的 MSVC 建置。`tests/run.sh` 可以在 Linux 上執行不依賴 foobar2000 的單元測試。

## 授權

MIT License，(C) 2026 Carinoasd。第三方元件的授權見 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
