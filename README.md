<p align="center">
  <img src="assets/tideline-logo-512.png" width="128" alt="Tideline logo">
</p>

<h1 align="center">Tideline</h1>

<p align="center">
  A modern launcher for Old School RuneScape and RuneScape with all your Jagex accounts and characters in one place.
</p>

Tideline is a modified version of [Bolt](https://codeberg.org/Adamcake/Bolt), the free, open-source third-party launcher by Adamcake. It keeps everything that makes Bolt useful (signing in with Jagex accounts, launching the Official client, RuneLite or HDOS) and adds a new interface built around switching between many accounts quickly.

> **Tideline is not affiliated with or endorsed by Jagex.** See [Legal](#legal).

## Features

- **Every account and character in one sidebar.** Each Jagex account is a card with its characters underneath. Click a character to select it, click it again to launch.
- **Run several characters at once.** Each launch opens its own client window, and green dots show which characters are running, even after you close and reopen Tideline.
- **Last played** time for every character.
- **A picture for each account**, from an image file, a drag-and-drop, or a link.
- **Themes.** Match Windows (dark or light), 20 presets, and a four-colour editor for your own. Colours are worked out from four choices: background, main colour, second colour and text.
- **Frameless, resizable window** with its own title bar.
- **Optional dashboard link.** If you run a companion website on your own PC, Tideline can show an "Open dashboard" button and share its colour theme with it. Off by default.
- The Official client, RuneLite and HDOS are all supported for Old School RuneScape. The Official client is supported for RuneScape (RS3).

## Requirements

- Windows 10 or 11, 64-bit
- **Java 17 or newer** installed (RuneLite's launcher needs it). A free build such as [Eclipse Temurin](https://adoptium.net) works well.
- A Jagex account (accounts must be Jagex accounts, not legacy accounts)

The RS3 plugin loader from Bolt is not included in this build.

## Install

Download the latest installer from the [Releases page](https://github.com/tidelinegp/tideline/releases/latest) (`Tideline-Setup-<version>.exe`). Tideline checks for updates itself and installs them with one click. You can also [build it yourself](#building-from-source).

Windows may show a SmartScreen warning because the build is not code-signed yet. Only download Tideline from this repository.

On first start Tideline looks for an existing Bolt data folder and copies your saved logins and settings into its own folder (`%APPDATA%\tideline`). It only copies; your Bolt data is left untouched.

## Privacy and security

- You sign in on Jagex's own login page, shown inside the launcher. Tideline does not ask for or store your password.
- Tideline adds no analytics or telemetry. It talks to Jagex (sign-in, character list, game downloads), the RuneLite release server on GitHub, and RuneScape's public news feed, as Bolt does.
- Fonts and the interface are bundled with the app, so no request is made to font or CDN servers.
- If you turn on the optional dashboard link, Tideline talks to the address you set (for example `http://127.0.0.1:8080`) and sends only the colour theme.
- Saved logins, settings and account pictures are stored locally in `%APPDATA%\tideline`.

## Building from source

You need Visual Studio 2022 (or Build Tools) with the C++ workload, CMake, Node.js, and a [CEF](https://adamcake.com/cef) binary distribution (Windows 64-bit, version 139). The steps below are what this project uses; see [BOLT-README.md](BOLT-README.md) for Bolt's own, more general build notes.

```bash
git clone --recurse-submodules https://github.com/tidelinegp/tideline.git
cd tideline

# put your CEF distribution in cef/dist, then build the interface
cd app
npm install
npm run build
cd ..

# configure, build and install (adjust the install path to taste)
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 \
  -D CMAKE_BUILD_TYPE=Release -D BOLT_SKIP_LIBRARIES=1 \
  -D "BOLT_CEF_INSTALLDIR=C:/Users/you" \
  -D "CMAKE_CXX_FLAGS=/DWIN32 /D_WINDOWS /EHsc /D_WINSOCKAPI_="
cmake --build build --config Release
cmake --install build --config Release
```

The interface is a Svelte and TypeScript app in [`app/`](app/). The native part is a small C++ program around Chromium Embedded Framework in [`src/`](src/).

## Roadmap

- Signed releases and an automatic updater
- A bundled Java so no separate install is needed
- More client options and per-character settings

## Credits

Tideline is built on **[Bolt](https://codeberg.org/Adamcake/Bolt)** by Adamcake and its contributors, and would not exist without it. Thank you.

Other things Tideline uses: [Chromium Embedded Framework](https://bitbucket.org/chromiumembedded/cef) (BSD licence), and the fonts Bricolage Grotesque and Figtree (SIL Open Font Licence 1.1). See [NOTICE.md](NOTICE.md).

## Licence

Tideline is free software under the **GNU Affero General Public License v3** (see [LICENCE](LICENCE)), the same licence as Bolt. If you distribute a modified version, or let people use one over a network, you must make its source code available under the same licence.

## Legal

Created using intellectual property belonging to Jagex Limited under the terms of Jagex's [Fan Content Policy](https://legal.jagex.com/docs/policies/fan-content-policy). This content is not endorsed by or affiliated with Jagex.

Tideline is a fan-made project. RuneScape and Old School RuneScape are trademarks of Jagex Limited.

Using a third-party launcher is at your own risk. Jagex has not published a ruling on third-party launchers; read [Jagex's rules](https://legal.jagex.com/docs/rules/macro-and-client-features-not-permitted) and decide for yourself.
