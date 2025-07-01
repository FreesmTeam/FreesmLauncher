<div align="center">

<img width="160" height="160" align="center" src="./favicon.webp" alt="Favicon">

<h1>
<a style="color:#f5c2e7" href="https://freesmlauncher.org/">Freesm Launcher</a>
</h1>

A Prism Launcher fork that **removes offline account restrictions**, adds custom auth server support and provides more customization

This fork is **not** endorsed by Prism Launcher

<p align="center">
<strong>English</strong> | <a style="color:#f5c2e7" href="./README_ru.md">Русский</a>
</p>

<div>

[![GitHub Repo stars](https://img.shields.io/github/stars/freesmteam/freesmlauncher?label=Stars&style=for-the-badge&color=%23f5c2e7&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0idXRmLTgiPz4KPHN2ZyBoZWlnaHQ9IjI0IiB2aWV3Qm94PSIwIC05NjAgOTYwIDk2MCIgd2lkdGg9IjI0IiB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciPgogIDxwYXRoIGQ9Im0zNTQtMjQ3IDEyNi03NiAxMjYgNzctMzMtMTQ0IDExMS05Ni0xNDYtMTMtNTgtMTM2LTU4IDEzNS0xNDYgMTMgMTExIDk3LTMzIDE0M1pNMjMzLTgwbDY1LTI4MUw4MC01NTBsMjg4LTI1IDExMi0yNjUgMTEyIDI2NSAyODggMjUtMjE4IDE4OSA2NSAyODEtMjQ3LTE0OUwyMzMtODBabTI0Ny0zNTBaIiBzdHlsZT0iZmlsbDogcmdiKDI0NSwgMTk0LCAyMzEpOyIvPgo8L3N2Zz4%3D)](https://github.com/freesmteam/freesmlauncher/stargazers)
![DRM Free Badge](https://img.shields.io/badge/drm-free-%23f5c2e7?style=for-the-badge)
![Garnix Badge](https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fgarnix.io%2Fapi%2Fbadges%2FFreesmTeam%2FFreesmLauncher&query=%24.message&style=for-the-badge&label=%20&color=white&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSI5NnB0IiBoZWlnaHQ9Ijk2cHQiIHZpZXdCb3g9IjAgMCAzNCAzNCIgZmlsbD0id2hpdGUiPjxwYXRoIGQ9Ik0zMyAxSDFWMzNIMzNWMVoiIGZpbGw9IndoaXRlIiBzdHJva2U9ImJsYWNrIiBzdHJva2Utd2lkdGg9IjIiIHN0cm9rZS1taXRlcmxpbWl0PSIxMCIgLz48cGF0aCBkPSJNMzIuOTkzNyAxNi45OTcxSDE3LjAwMzJWMjQuNzgyNCIgc3Ryb2tlPSJibGFjayIgc3Ryb2tlLXdpZHRoPSIyIiBzdHJva2UtbWl0ZXJsaW1pdD0iMTAiIC8%2BPC9zdmc%2B)

</div>

</div>

## 💻 Screenshots

<details>
  <summary>Show</summary>

  <div align="center">
    <div style="display: grid; grid-template-columns: repeat(3, 1fr); gap: 10px;">
      <img src="screenshots/freesmlauncher_home_screenshot.png" alt="Dark theme dashboard" width="512" />
      <img src="screenshots/freesmlauncher_home_screenshot_opacity.png" alt="Dashboard with opacity" width="512" />
      <img src="screenshots/freesmlauncher_home_mita.gif" alt="Mita catpack" width="512" />
      <img src="screenshots/freesmlauncher_home_maxwell.gif" alt="Maxwell catpack" width="512" />
      <img src="screenshots/freesmlauncher_settings_accounts_screenshot.png" alt="Accounts settings" width="512" />
      <img src="screenshots/freesmlauncher_instance_add_screenshot.png" alt="Add instance" width="512" />
      <img src="screenshots/freesmlauncher_instance_settings_screenshot.png" alt="Instance settings" width="512" />
      <img src="screenshots/freesmlauncher_settings_theme_screenshot.png" alt="Theme settings" width="512" />
    </div>
  </div>

</details>

## ✨ Features

- Offline mode doesn't require signing in with a microsoft account anymore
- [Ely.by](https://ely.by/) can be used as an account auth option, providing a seamless integration with minecraft. You will see your minecraft skin anywhere without any mods or plugins
- Custom authentication server support
- Polished, minimalist dark and light themes based on a [Fluent-Dark](https://github.com/PrismLauncher/Themes/tree/main/themes/Fluent-Dark) theme with [catppuccin](https://catppuccin.com/)/[rosé pine](https://rosepinetheme.com/) colors and [Microsoft Fluent](https://fluent2.microsoft.design/iconography) icons
- Animated GIF cat packs with image cropping support
- In-game screenshots copying to the buffer history support
- Animated snow effect for those who love... snow?
- Random username and instance icon selection with ultra super advanced and cryptographically secure, absolutely random number generator based on the [lavarand](https://en.wikipedia.org/wiki/Lavarand)
- FLOSS
- ...all the Prism Launcher's features

## 📊 Comparison


| Feature                                                   | Freesm  Launcher | Shattered  Prism | HMCL | Fjord   | PollyMC       | ElyPrism      | UltimMC | Prism-Cracked | Prism Launcher |
|-----------------------------------------------------------|------------------|------------------|------|---------|---------------|---------------|---------|---------------|----------------|
| Offline Mode without a Microsoft account                  | ✅                | ✅                | ✅    | ❌       | ✅             | ✅             | ✅       | ✅             | ❌              |
| FTB packs                                                 | ✅                | ✅                | ❌    | ✅       | ✅             | ✅             | ❌       | ✅             | ✅              |
| Ely.by support¹                                           | ✅                | 🟨               | 🟨   | 🟨      | 🟨            | ✅             | 🟨      | ❌             | ❌              |
| Authlib-injector support²                                 | ✅                | ✅                | ✅    | ✅       | ✅             | ❌             | ❌       | ❌             | ❌              |
| Custom Authlib-injector jar support²                      | ❌                | ✅                | ❌    | ✅       | ❌             | ❌             | ❌       | ❌             | ❌              |
| Differentiating between auth servers                      | ✅                | ✅                | ✅    | ✅       | ❌             | ❌             | ❌       | ❌             | ❌              |
| Animated Cat Packs & Cropping                             | ✅                | ❌                | ❌    | ❌       | ❌             | ❌             | ❌       | ❌             | ❌              |
| Screenshots saving to the buffer history without any mods | ✅                | ❌                | ❌    | ❌       | ❌             | ❌             | ❌       | ❌             | ❌              |
| Fork                                                      | PrismLauncher    | FjordLauncher    | ❌    | PollyMC | PrismLauncher | PrismLauncher | MultiMC | PrismLauncher | PolyMC         |

¹ If it is marked as "🟨", then skins won't work in many cases, because the provided launcher doesn't use official Ely.by authlib patches

² If it is marked as "❌", you can still change a `javaagent` JVM argument to use your `authlib-injector` jar file as an auth server

// old

---

## 📥 Installation

### 🏷️ Stable Releases

Download Freesm Launcher from our [official website](https://freesmlauncher.org) or the [GitHub Releases](https://github.com/FreesmTeam/FreesmLauncher/releases) page. Packages available for **Windows, macOS, and Linux (x86-64)**.

### 🌙 Development Builds

For the latest features, check out the [GitHub Actions](https://github.com/FreesmTeam/FreesmLauncher/actions) artifacts or [Nightly Builds](https://nightly.link/FreesmTeam/FreesmLauncher/workflows/develop) (not recommended for most users).

> [!CAUTION]  
> Development (nightly) builds may be unstable, larger, and contain debug info. Use **at your own risk**.

---

## 💬 Community & Support

If you find bugs or want to suggest features, please open an issue on our [GitHub repository](https://github.com/FreesmTeam/FreesmLauncher). Pull requests and contributions (code, docs, translations) are welcome!

> [!CAUTION]  
> Do not mention Freesm Launcher on Prism Launcher’s Discord, forums, or other channels. Freesm is a standalone project and has its own community.

> [!NOTE]  
> Freesm does **not collect any personal data** or usage information. All features run locally. The source code is fully open under GPL-3.0 for your audit.

---

## 🌐 Translations

<!-- Freesm Launcher supports community translations via [Weblate](https://hosted.weblate.org/projects/freesmlauncher/). Help us translate or improve existing translations by visiting our [Weblate page](https://hosted.weblate.org/projects/freesmlauncher/) or our [GitHub Translations guide](https://github.com/FreesmTeam/Translations). -->

---

## 🛠️ Building from Source

To build Freesm Launcher yourself, see the [Prism Launcher build instructions](https://prismlauncher.org/wiki/development/build-instructions/) (Freesm uses Garnix build system).  
The source code is available on GitHub. Contributions to build scripts and packaging are welcome.

---

## 📜 License

Freesm Launcher is free software under the **GPL-3.0-only** license. See the [LICENSE](LICENSE) file for details.
