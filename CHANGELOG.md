# Changelog

本專案的所有重要變更都記錄於此。
格式參考 [Keep a Changelog](https://keepachangelog.com/zh-TW/1.1.0/)，版本號遵循 [Semantic Versioning](https://semver.org/lang/zh-TW/)。

All notable changes to this project are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/).

---

## [0.3.0] - 2026-10-07

### Added

- **暫停太久自動清除狀態。** 暫停（或「停止時保留」）超過指定分鐘數就清除，預設 15 分鐘，0 為不清除。
  *Clear the status after being paused or stopped for a while (15 minutes by default).*
- **依播放清單過濾。** 「隱藏這些清單」與「只顯示這些清單」，分號分隔、支援 `*`、不分大小寫。
  *Hide the status for some playlists, or show it only for some.*
- **設定頁即時預覽。** 修改三行文字格式時，直接顯示目前這首歌套用後的樣子。
  *A live preview of the three text lines on the preferences page.*
- **指定 Discord 版本。** 同時開了正式版、PTB、Canary 時可選擇要連哪一個；狀態列顯示實際連上的版本。
  *Choose between Discord, PTB, and Canary. The status shows which one is connected.*
- **簡體中文與日文介面**，連同原本的英文與繁體中文，依 Windows 介面語言自動切換。
  *Simplified Chinese and Japanese UI, alongside English and Traditional Chinese.*
- **foo_acfu 支援。** 32 位元 foobar2000 可在 foo_acfu 中檢查本元件的更新（foo_acfu 只有 32 位元版）。
  *Update checks through foo_acfu on 32-bit foobar2000.*
- README 加入示意動圖。
  *A demo animation in the README.*

### Changed

- 封面改抓較大的尺寸：iTunes 1000×1000、Cover Art Archive 1200px。
  *Larger album art: 1000×1000 from iTunes, 1200px from Cover Art Archive.*
- 設定頁改為單一版面＋翻譯表，新增語言時不必再複製對話框。
  *Preferences use one layout plus a translation table, so adding a language no longer means copying dialogs.*

### Fixed

- 設定「隱藏這些清單」時，剛換曲的瞬間可能還不知道正在播放哪個清單，被隱藏的歌會短暫送出；現在會等確認清單後才顯示。
  *A hidden playlist's song could briefly be sent right after a track change.*

---

## [0.2.0] - 2026-10-07

### Added

- **更多封面來源，依序嘗試。** 手動指定 → MusicBrainz → iTunes → Last.fm → 上傳本機封面。某個來源暫時連不上時改試下一個，稍後再重試。
  *More album art sources, tried in order: hand-set, MusicBrainz, iTunes, Last.fm, local upload. A source that is temporarily unreachable is skipped and retried later.*
  - **iTunes**：免申請 key，日本動畫、遊戲音樂收錄齊全；比對時忽略「- Single」「- EP」與全形／半形差異。
    *iTunes, no key needed, strong on Japanese anime and game music.*
  - **Last.fm**：使用你自己的 API key。key 錯誤或限流時不會把專輯記成「沒有封面」。
    *Last.fm with your own API key. Key errors and rate limits are not cached as "no art".*
  - **MusicBrainz 鏡像站**（上游 #63），以及專輯層級沒有封面時改找同專輯其他版本（上游 #110）。
    *A MusicBrainz mirror setting, and a fallback to other releases of the same album.*
- **手動指定封面。** 右鍵 Utilities > 指定 Discord 專輯封面，以專輯為單位指定圖片網址，優先於所有自動來源（上游 #96）。
  *Set album art by hand from the context menu. It takes priority over every automatic source.*
- **播放／暫停／停止小圖示與無封面預設圖。** 原創圖示，以外部網址提供，不需要上傳到 Discord。
  *Play / pause / stop icons and a placeholder for tracks without art, all original artwork.*
- **停止時保留狀態**（上游 #20）。
  *Optionally keep showing the last song after stopping.*
- **連結與按鈕。** 第一行、第二行、封面可設定點擊後開啟的網址；最多兩個按鈕。
  *Links on the text and album art, and up to two buttons.*
- **隱私過濾。** 符合條件的曲目不顯示狀態（上游 #68），或不抓封面（上游 #93）。
  *Hide the status, or skip album art, for tracks matching a search query.*
- **繁體中文介面。** Windows 介面為繁體中文時，設定頁、選單與狀態訊息自動顯示中文。
  *A Traditional Chinese UI, used automatically on Traditional Chinese Windows.*
- **從 foo_discord_rich 匯入設定。** 第一次啟動時沿用你改過的值；會避開 foo_discord_rich 2.0.2 的 GUID 衝突。
  *Settings you changed in foo_discord_rich are picked up on first start.*
- **更新檢查。** 每天一次，有新版時在 Console 與設定頁提示；可在 Advanced Preferences 關閉。
  *A daily update check, which can be turned off.*
- 設定頁拆成「Discord Listening」「專輯封面」「連結與過濾」三頁。
  *Preferences are split into three pages.*

### Fixed

- 連線失敗時沒有留下任何紀錄。現在會寫到 Console，同樣的錯誤只記一次。
  *Connection failures were not logged at all.*
- 時間戳因取整跨秒差 1 秒時會多送一次更新，浪費 Discord 的限流額度。
  *A one-second rounding difference no longer costs an extra update.*

### Testing

- 新增以假的 Discord 伺服器進行的整合測試（重連、限流、Discord 卡住時的關閉），並在 CI 執行。
  *Integration tests against a fake Discord server, run in CI.*
- 已在 foobar2000 2.26 x64 與 x86 上實機驗證所有功能。
  *Every feature was verified on foobar2000 2.26 x64 and x86.*

---

## [0.1.0] - 2026-10-07

第一個版本。從零重寫，參考 foo_discord_rich 的 issue 紀錄設計。
*First release. A from-scratch rewrite designed around the foo_discord_rich issue history.*

### Added

- **「Listening to」狀態，成員清單可直接顯示歌名。** 支援 Discord 的 `status_display_type`，也能改成顯示歌手或應用程式名稱，或把類型改為 Playing / Watching。
  *"Listening to" activity with `status_display_type`, so the member list can show the song title.*
- **進度條。** 每次換曲、拖動、繼續播放都以毫秒重新對時；無法拖動的串流只顯示經過時間。
  *A progress bar, resynced in milliseconds on every track change, seek and resume.*
- **MusicBrainz / Cover Art Archive 封面。** 優先使用標籤中的 MBID；搜尋時要求高分且專輯名稱相符，並跳脫查詢語法，避免抓錯封面。遵守 MusicBrainz 每秒一次的限制。
  *Album art from MusicBrainz / Cover Art Archive, preferring tagged MBIDs and requiring an exact title match when searching.*
- **上傳本機封面。** 縮成 512px JPEG 後交給自訂指令上傳，相容 foo_discord_rich 的上傳腳本。
  *Local art upload through a user command, downscaled to 512px first.*
- **封面快取管理。** 設定頁可清除全部快取，右鍵選單可重新抓取所選曲目的封面。
  *Clear the whole art cache from preferences, or re-fetch art for selected tracks from the context menu.*
- **內建的 Discord 應用程式**，安裝後不需任何設定即可使用；想顯示其他名稱可以填入自建的 Application ID。
  *A built-in Discord application, so it works with no setup. Enter your own application ID to show a different name.*
- **除錯紀錄。** 在 Advanced Preferences 開啟後，與 Discord 往來的完整內容會寫入 profile 資料夾的 `debug.log`。
  *An optional debug log of everything exchanged with Discord, enabled in Advanced Preferences.*
- Playback 選單的「Show on Discord」開關與「Discord Listening settings」捷徑、支援深色模式的設定頁、x64 與 x86 版本。
  *A Playback menu toggle and settings shortcut, a dark-mode-aware preferences page, x64 and x86 builds.*

### 與 foo_discord_rich 相比修正的問題 / Fixed compared to foo_discord_rich

- 時間軸不動或從奇怪的時間開始（#67 #81 #89 #101 #104）。
  *Stuck or wrong time display.*
- 封面查詢失敗後永遠不再重試；快取存的 archive.org 節點網址失效（#69 #105）。網路錯誤現在只暫停 10 分鐘，「沒有封面」的結果也只保留 7 天。
  *Failed art lookups were never retried and cached URLs went stale.*
- 封面快取無法清除（#108）。
  *The art cache could not be cleared.*
- 上傳設定在重新啟動後被還原（#103 #109）：原因是兩個設定共用了同一個 GUID，現在會在編譯時檢查。
  *Upload settings reset on restart because two settings shared a GUID. GUID uniqueness is now checked at compile time.*
- 欄位缺值時以「?」查詢，導致封面抓錯（#95）。
  *Missing tags were queried as "?", producing wrong art.*
- 在清單最後一首按「下一首」後，狀態永遠不會消失（#85）。
  *The status never cleared after skipping past the last track.*
- 網路卡住時，foobar2000 關閉得很慢（#80）。
  *Slow foobar2000 shutdown when the network hung.*
- 上傳程式輸出較多時會卡住，錯誤訊息也被當成封面網址（#79）。
  *The uploader could deadlock on large output, and error text was used as an art URL.*

[0.3.0]: https://github.com/Carinoasd/foo_discord_listening/releases/tag/v0.3.0
[0.2.0]: https://github.com/Carinoasd/foo_discord_listening/releases/tag/v0.2.0
[0.1.0]: https://github.com/Carinoasd/foo_discord_listening/releases/tag/v0.1.0
