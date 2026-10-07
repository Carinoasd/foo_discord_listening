# 驗證清單 / Verification checklist

每次發佈前在實機上跑一次。環境：Windows、foobar2000 2.x、Discord 桌面版，使用既有的 Discord 帳號。
另一台裝置（或另一個帳號）可以用來確認「別人看到的樣子」；自己的個人檔案不會顯示進度條。

## 安裝

- [ ] 移除 foo_discord_rich（如有），安裝 `.fb2k-component`，重新啟動 foobar2000。
- [ ] **Preferences > Components** 列出 *Discord Listening*，版本號正確。
- [ ] **Preferences > Tools > Discord Listening** 正常開啟，所有欄位完整顯示、沒有被下方按鈕遮住；切換 foobar2000 深色模式後，設定頁跟著變色。
- [ ] Application ID 留空時使用內建 ID，狀態顯示 *Connected*。

## 連線

- [ ] 填入自建的 application ID 並 Apply：重新連線，Discord 顯示自建應用程式的名稱；清空後回到內建的。
- [ ] 關閉 Discord：狀態變成錯誤訊息，foobar2000 不卡頓。
- [ ] 重新開啟 Discord：不需任何操作，60 秒內自動重新連上並顯示目前歌曲。
- [ ] 取消勾選「Show what I'm playing」：Discord 狀態消失；重新勾選後恢復。
- [ ] Playback 選單的 *Show on Discord* 勾選狀態與設定頁同步。
- [ ] Playback 選單的 *Discord Listening settings* 直接開到本元件的設定頁。

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

## 閒置、清單、版本

- [ ] 「閒置 N 分後清除」設為 1：暫停約 1 分鐘後狀態清除；繼續播放後恢復。
- [ ] 「隱藏這些清單」設為目前清單（可用 `*`）：換曲時被隱藏的歌**一次都不會**送出（除錯紀錄只有清除）。
- [ ] 「只顯示這些清單」設為其他清單：不顯示；設為目前清單（大小寫不同）：顯示。
- [ ] Discord 版本選沒有在執行的版本：狀態顯示該版本沒有在執行；選「任何 Discord」：連上並顯示版本名稱。
- [ ] 設定頁預覽：顯示目前歌曲套用三行格式的結果；修改格式（不按 Apply）時立即更新；沒有播放時顯示提示文字。

## 圖示與停止

- [ ] 播放／暫停時封面角落顯示對應的小圖示；沒有封面的曲目顯示預設圖。
- [ ] 「停止時：保留最後一首歌」：停止後仍顯示最後一首，第一行加上「(已停止)」，顯示停止圖示，沒有進度條。
- [ ] 類型改 Playing／Watching、狀態列改顯示第二行：Discord 依設定顯示。

## 連結、按鈕與過濾

- [ ] 第一行連結與按鈕使用 `https://www.youtube.com/results?search_query=[%artist% ]%title%`：送出的網址空白與中日文已編碼，Discord 沒有拒絕（除錯紀錄沒有 ERROR）。
- [ ] 「不顯示的曲目」設為 `%album% HAS 某字`：播放符合的曲目時狀態清除。
- [ ] 「不抓封面的曲目」：符合的曲目顯示預設圖，不送出封面查詢。

## 封面

- [ ] 有 `MUSICBRAINZ_ALBUMID` 標籤的專輯：顯示正確封面。
- [ ] 沒有 MBID 但很知名的專輯（例如 The Beatles - Abbey Road）：數秒內顯示正確封面。
- [ ] 沒有專輯標籤的檔案：不顯示封面，也不送出查詢（Console 沒有相關錯誤）。
- [ ] MusicBrainz 伺服器設成連不上的位址（例如 `https://127.0.0.1:9`）：除錯紀錄顯示稍後重試，並改由 iTunes 找到封面。
- [ ] 右鍵 **Utilities > 指定 Discord 專輯封面...**：對話框顯示專輯名稱；填入網址後狀態立刻改用該圖；「清除封面快取」後手動指定的仍保留。
- [ ] Last.fm 填入錯誤的 key：Console 顯示 Last.fm 錯誤，該專輯不會被記成「沒有封面」。
- [ ] 設定頁的快取數量會增加；按 *Clear art cache* 後歸零，目前歌曲重新抓取。
- [ ] 對曲目按右鍵 **Utilities > Re-fetch Discord album art**：Console 顯示已清除的筆數。
- [ ] 重新啟動 foobar2000：快取仍在（不重新查詢），設定都沒有被還原。
- [ ] 拔掉網路後播放新專輯：Console 顯示稍後重試，foobar2000 不卡頓；恢復網路 10 分鐘後重試成功。

## 上傳本機封面

- [ ] 來源設為 *Upload local art*，指令使用 README 的 catbox 範例：內嵌封面的曲目顯示上傳後的封面。
- [ ] 同一張專輯的其他曲目不會重複上傳（Console 中只有一次）。
- [ ] 把指令改成不存在的程式：Console 顯示上傳失敗並稍後重試，不會把錯誤訊息當成封面。
- [ ] `%TEMP%` 中沒有殘留的 `fdl-art-*.jpg`。

## 介面

- [ ] Windows 介面為繁體中文時，三個設定頁、手動指定封面對話框、選單、狀態都是中文，且沒有文字被裁掉。
- [ ] 以 `-DFDL_FORCE_LANG=en|zh_cn|ja` 建置截圖：英文、簡體中文、日文的三個設定頁都沒有文字被裁掉。
- [ ] foobar2000 深色模式下三個設定頁都正確變色。
- [ ] 修改任一欄位後 Apply 變成可按，按下後立即生效；Reset page 恢復預設值。

## 匯入與更新

- [ ] 全新 profile 的 `configuration\foo_discord_rich.dll.cfg` 有改過的值：第一次啟動後被沿用，foo_discord_rich 的預設值不會蓋掉本元件的預設；第二次啟動不再匯入。
- [ ] 更新檢查：設定頁與 Console 在有新版時提示；Advanced Preferences 關閉後不再連線。

## 除錯紀錄

- [ ] Advanced Preferences 開啟 *Write debug log*：profile 下出現 `foo_discord_listening\debug.log`，內含送出與收到的 JSON；關閉後不再寫入。

## 32 位元

- [ ] 在 32 位元的 foobar2000 上安裝：元件載入、連上 Discord、抓到封面，關閉正常。
- [ ] 安裝 foo_acfu：Preferences > Components > Auto Check for Updates 列出 Discord Listening；勾選後 Check for Updates，Available 顯示 GitHub 最新版本。

## 結束

- [ ] 播放中關閉 foobar2000：2 秒內關閉完成，Discord 狀態消失。
- [ ] 正在上傳時關閉 foobar2000：上傳程式一併結束（工作管理員中沒有殘留的 curl）。
