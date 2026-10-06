# foo_discord_listening

[繁體中文](README.md) · **English**

> Shows what you're playing in foobar2000 as a Discord "Listening to" activity, with a progress bar and album art.

[![Build](https://github.com/Carinoasd/foo_discord_listening/actions/workflows/build.yml/badge.svg)](https://github.com/Carinoasd/foo_discord_listening/actions/workflows/build.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

A from-scratch foobar2000 component inspired by [foo_discord_rich](https://github.com/TheQwertiest/foo_discord_rich). The original has been unmaintained since 2024 and has many open issues, such as a stuck time display, missing album art, and settings that reset on restart. This rewrite was designed around those issue reports and uses no code from the original.

---

## Features

- **"Listening to" activity.** The member list shows the song title instead of just "foobar2000". You can switch it to the artist or the application name.
- **Progress bar.** It resyncs on track change, seek, and resume.
- **Album art.**
  - The MusicBrainz IDs in your tags are used first.
  - Otherwise the component searches MusicBrainz and checks the album title, so it won't pick the wrong cover.
  - You can also upload your local art (external or embedded, downscaled first) with a command of your choice.
- **All three text lines are customizable** with foobar2000 title formatting.
- **Clearable cache.** The preferences page has a "Clear art cache" button, and the context menu has **Re-fetch Discord album art**.
- **Never slows foobar2000 down.**
  - The Discord connection and all network requests run on background threads, and every request has a timeout.
  - The component reconnects automatically when Discord starts.
  - Pending requests are aborted immediately when foobar2000 exits.
- Supports foobar2000 2.x x64 and x86. The preferences page supports dark mode.

## Installation

1. Download `foo_discord_listening-x.y.z.fb2k-component` from [Releases](https://github.com/Carinoasd/foo_discord_listening/releases).
2. In foobar2000, open **File > Preferences > Components**, drop the file in, click **Apply**, and restart.
3. Open **File > Preferences > Tools > Discord Listening** and enter a Discord application ID (see below).

> If you have foo_discord_rich installed, remove it first. Both components update your Discord status and will overwrite each other.

### Discord application ID

In "Listening to **XXX**", XXX is the name of a Discord application, so you need one:

1. Go to the [Discord Developer Portal](https://discord.com/developers/applications), click **New Application**, and give it the name you want to show (for example `foobar2000`).
2. Copy the **Application ID** (a number) into the *Application ID* field and click **Apply**.

The application ID is not a secret.

## Settings

| Setting | Default | Notes |
| --- | --- | --- |
| Type | Listening to | Playing / Watching are also available |
| Status shows | Line 1 | What the member list and your status show |
| When paused | Keep showing the song | Keep the song without time, or clear the status |
| Show progress bar | On | Computed as "now − elapsed time"; recomputed on track change, seek, and resume |
| Line 1 / 2 / 3 | `[%title%]` / `[%artist%]` / `[%album%]` | Line 3 is also the album art tooltip |
| Album art source | MusicBrainz | Or "MusicBrainz, then upload", or "Upload local art" |
| Upload once per | `$if([%album%],[%album artist%]\|[%album%],%path%)` | One upload per album |

### Uploading local art

For albums MusicBrainz doesn't have, such as doujin or game soundtracks, you can upload the art yourself.

- `{path}` in the command is replaced with the path of a downscaled JPEG.
- If the command has no `{path}`, the path is written to stdin, which is compatible with foo_discord_rich upload scripts.
- The first `https://` URL the command prints is used as the art.

Example using the curl that ships with Windows 10 and later, uploading to catbox.moe:

```
curl -s -F reqtype=fileupload -F fileToUpload=@{path} https://catbox.moe/user/api.php
```

> Uploading publishes your cover art on a third-party site. Pick a host that keeps files permanently: the URL is cached, and the art breaks if the file expires.

## Troubleshooting

- **Nothing shows up.**
  - Enable *Share your detected activities with others* in Discord's **Activity Privacy** settings.
  - Run Discord and foobar2000 at the same privilege level. A Discord running as administrator can't be reached by a normal process.
  - Don't add foobar2000 as a "game" in Discord.
- **The art is a question mark or wrong.** Right-click the track and choose **Utilities > Re-fetch Discord album art**. Adding a `MUSICBRAINZ_ALBUMID` tag gives the most accurate results.
- **Logs.** Open **View > Console**. All messages start with `[foo_discord_listening]`.
- Discord doesn't show the progress bar on your own profile. Others can see it.

## Building

Requires Visual Studio 2022 (or the Build Tools), CMake 3.25+, and Ninja. The dependencies (foobar2000 SDK, WTL, nlohmann/json) are downloaded during configure and verified by SHA256.

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

When developing inside WSL, `scripts/build.sh [Release|Debug] [x64|x86]` drives the Windows MSVC toolchain. `tests/run.sh` runs the unit tests that don't need foobar2000 on Linux.

## License

MIT License, (C) 2026 Carinoasd. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for third-party licenses.
