# 驗證清單 / Verification checklist

每次發佈前在實機上跑一次。環境：Windows、foobar2000 2.x、Discord 桌面版，使用既有的 Discord 帳號。
另一台裝置（或另一個帳號）可以用來確認「別人看到的樣子」；自己的個人檔案不會顯示進度條。

## 安裝

- [ ] 移除 foo_discord_rich（如有），安裝 `.fb2k-component`，重新啟動 foobar2000。
- [ ] **Preferences > Components** 列出 *Discord Listening*，版本號正確。
- [ ] **Preferences > Tools > Discord Listening** 正常開啟；切換 foobar2000 深色模式後，設定頁跟著變色。
- [ ] Application ID 留空時使用內建 ID，狀態顯示 *Connected*。

## 連線

- [ ] 填入自建的 application ID 並 Apply：重新連線，Discord 顯示自建應用程式的名稱；清空後回到內建的。
- [ ] 關閉 Discord：狀態變成錯誤訊息，foobar2000 不卡頓。
- [ ] 重新開啟 Discord：不需任何操作，60 秒內自動重新連上並顯示目前歌曲。
- [ ] 取消勾選「Show what I'm playing」：Discord 狀態消失；重新勾選後恢復。
- [ ] Playback 選單的 *Show on Discord* 勾選狀態與設定頁同步。

## 播放狀態

- [ ] 播放：Discord 顯示 *Listening to \<應用程式名稱\>*，三行文字與設定相符。
- [ ] 成員清單顯示歌名（Status shows = Line 1）。
- [ ] 進度條起點與 foobar2000 的播放時間一致（誤差 1 秒內）。
- [ ] 拖動進度條：Discord 的時間跟著跳。
- [ ] 暫停（Keep showing）：第一行後面出現「(Paused)」、時間消失；繼續播放後時間正確。
- [ ] 暫停（Clear）：狀態消失；繼續播放後恢復。
- [ ] 連續快速切歌 10 次：最後停下的那首在數秒內正確顯示（限流後只送最新狀態）。
- [ ] 停止：狀態消失。
- [ ] 播放清單最後一首時按「下一首」：狀態在約 1 秒後消失（上游 #85）。
- [ ] 網路電台：顯示電台推送的目前曲名，沒有進度條；經過時間在電台換歌時歸零。
- [ ] 歌名含 emoji 或超過 128 字：正常顯示，以刪節號截斷，Console 沒有 Discord 拒絕的訊息。
- [ ] 改 Line 1/2/3 的格式並 Apply：立刻生效。

## 封面

- [ ] 有 `MUSICBRAINZ_ALBUMID` 標籤的專輯：顯示正確封面。
- [ ] 沒有 MBID 但很知名的專輯（例如 The Beatles - Abbey Road）：數秒內顯示正確封面。
- [ ] 沒有專輯標籤的檔案：不顯示封面，也不送出查詢（Console 沒有相關錯誤）。
- [ ] 設定頁的快取數量會增加；按 *Clear art cache* 後歸零，目前歌曲重新抓取。
- [ ] 對曲目按右鍵 **Utilities > Re-fetch Discord album art**：Console 顯示已清除的筆數。
- [ ] 重新啟動 foobar2000：快取仍在（不重新查詢），設定都沒有被還原。
- [ ] 拔掉網路後播放新專輯：Console 顯示稍後重試，foobar2000 不卡頓；恢復網路 10 分鐘後重試成功。

## 上傳本機封面

- [ ] 來源設為 *Upload local art*，指令使用 README 的 catbox 範例：內嵌封面的曲目顯示上傳後的封面。
- [ ] 同一張專輯的其他曲目不會重複上傳（Console 中只有一次）。
- [ ] 把指令改成不存在的程式：Console 顯示上傳失敗並稍後重試，不會把錯誤訊息當成封面。
- [ ] `%TEMP%` 中沒有殘留的 `fdl-art-*.jpg`。

## 除錯紀錄

- [ ] Advanced Preferences 開啟 *Write debug log*：profile 下出現 `foo_discord_listening\debug.log`，內含送出與收到的 JSON；關閉後不再寫入。

## 結束

- [ ] 播放中關閉 foobar2000：2 秒內關閉完成，Discord 狀態消失。
- [ ] 正在上傳時關閉 foobar2000：上傳程式一併結束（工作管理員中沒有殘留的 curl）。
