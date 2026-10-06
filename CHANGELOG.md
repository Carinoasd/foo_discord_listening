# Changelog

本專案的所有重要變更都記錄於此。
格式參考 [Keep a Changelog](https://keepachangelog.com/zh-TW/1.1.0/)，版本號遵循 [Semantic Versioning](https://semver.org/lang/zh-TW/)。

All notable changes to this project are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/).

---

## [0.1.0] - 未發布 / Unreleased

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
- Playback 選單的「Show on Discord」開關、支援深色模式的設定頁、x64 與 x86 版本。
  *A Playback menu toggle, a dark-mode-aware preferences page, x64 and x86 builds.*

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

[0.1.0]: https://github.com/Carinoasd/foo_discord_listening/releases/tag/v0.1.0
