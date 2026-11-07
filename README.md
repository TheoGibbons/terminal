# Windows Terminal session drawer

This local fork adds the left sidebar and [pane notes](#pane-notes). Build and run this fork to use it; the installed Microsoft Store Terminal is not modified.

- Use the slim arrow button centered vertically on the divider to hide the drawer, and the right arrow at the left edge to show it again. The button and its strip are 14 px wide. Reopening restores its previous width. The button supports keyboard activation and has a Show/Hide tooltip. This choice lasts for the current window; the drawer opens by default in a new window.
- The drawer and divider appear once the window has the minimum number of tabs (one by default, so they always show), and hide again when fewer remain. The drawer starts directly with session cards, without a heading. The sidebar starts at 15% of the content width. The divider has an opaque background matching the drawer and shows a horizontal resize cursor on hover and during dragging. Drag it, or click the divider and press Left/Right, to resize it between 10% and 50%. The divider highlights while it has focus; Esc or Enter returns focus to the terminal.
- Each tab has a read-only preview of its last **ten rows containing text in the visible screen** (blank rows are skipped so a TUI's empty middle does not hide its reply), including alternate-screen TUIs and in-place updates. Long lines show their leftmost columns and are clipped at the card edge; padded TUI rows do not gain ellipsis markers. Empty prompt rows are skipped, including Claude prompts padded with non-breaking spaces; prompts containing text remain visible. Split tabs preview their last focused terminal pane. Their preview title starts with the pane count, such as `[2] Ubuntu` or `[3] Ubuntu`; the prefix updates as panes open or close and disappears when one remains.
- Up to three `+ Profile` buttons stay at the bottom of the drawer and open a new tab with that profile. They show a hand cursor on hover. They show the first three distinct profiles in tab/pane order, including split panes, and update as sessions open or close. Long names are shortened to fit; hover for the full name.
- Click a card, or focus it and press Enter/Space, to select the existing tab and return input focus to its main pane. Sessions keep running with their existing terminal dimensions while inactive. Previewing never sends input, scrolls, changes a selection, or resizes a session. Resizing the main window or drawer still resizes the active terminal normally.
- Right-click a card (or focus it and press Shift+F10) for **Notify me when this finishes…**, **Close**, and **Preview background color…**. Close uses the normal tab-close confirmation flow. The colour picker offers eight muted presets, a custom colour, and **Clear** to restore the terminal background. Each tab keeps its preview colour until it closes, including across card rebuilds and hiding/showing the drawer; colours are not saved across restarts. Explicit terminal text colours are preserved; ordinary default text is adjusted only when a custom background needs more contrast. Bell highlighting temporarily takes priority and restores the chosen colour when cleared.
- **Notify me when this finishes…** is enabled while activity is detected, using the same signal as the spinner. It asks for a delay in seconds (0–3600, default 30) and an optional callback URL such as `https://ntfy.sh/your-topic`; text without a scheme gets `https://`. Both values become the next defaults and persist across restarts in `state.json` (`sessionNotifyDelay`, `sessionNotifyCallbackUrl`; elevated windows keep their own copy). While a request is pending, the card shows a speaker icon left of its title and, unless it has its own preview colour, an orange background. Once activity stops and stays stopped for the delay, Terminal plays the Windows notification sound, flashes the taskbar button, and shows a desktop notification. With a callback URL, the notification says when the URL will be called; after the same delay again, Terminal sends it a `POST` whose plain-text body names the tab, which ntfy shows as the message. Activity resuming during either delay cancels that step and waits for the next finish. After activity stops, key presses, clicks or scrolling in the tab, switching to it (including clicking its card or the notification), or returning to the window while it is selected end the request silently; mouse movement alone does not. Input while the tab is still working is ignored. The request ends after it notifies (and calls back), or when its tab closes or moves to another window; while it is pending, the same menu item reads **Cancel finish notification**. Pending requests are not saved across restarts. Unpackaged and portable builds register their notification identity under `HKCU\Software\Classes\AppUserModelId` when the first one is shown, because Windows otherwise drops toasts from apps without a Start menu shortcut; afterwards, notifications requested by programs (OSC 9/777) also appear.
- Closing, adding, or reordering tabs preserves the drawer scroll position, clamped to the new bottom if the list becomes shorter.
- Visible cards refresh at most four times per second; unchanged text and colours are not reassigned. Offscreen cards skip buffer reads. The timer stops when the window is hidden, the page is unloaded, or focus mode hides the drawer.
- Cards show the tab title followed by the terminal's reported working directory on a separate line. Long paths are shortened from the front with a `...` prefix so the directory name stays visible; hover over the path for the full directory. The path follows the last focused pane in split tabs and updates when the shell reports a directory change; without shell reporting, Terminal retains its initial directory. Empty paths are hidden. The active card has a full outline that remains visible on hover, without redundant Active/Live text. Activity spinners, bell notifications, and connection-closed indicators remain in the header. The activity spinner appears whenever the visible preview contains `esc to interrupt`, covering Working, Waiting for background terminal, Compacting context, and other interruptible states. Claude Code clips its footer, including that hint, to one row, so the drawer also reads Claude's status row above the prompt, which wraps instead: a working row such as `✶ Cooking… (12s · ↓ 1.2k tokens)` counts as working in panes too narrow for the hint. When Claude Code is waiting for input but its latest turn summary ends with running tasks (`✻ Cooked for 4s · done 5:58 PM · 1 shell, 1 monitor still running`, or `✻ Waiting for 2 background agents to finish`), or its footer lists them (`1 shell, 1 monitor`, `2 local agents`, `3 background tasks`), the spinner is dimmed and turns at half speed. Only the latest summary counts; earlier ones stay in view after their tasks end. Claude's rows count only while the preview's last row looks like Claude's footer or prompt, so a shell prompt after Claude exits ends them. Summaries need Claude's Show turn duration setting, which is on by default. Background tasks still count as activity for alerts and finish notifications, so a session waiting on a monitor raises its alert only after its tasks end and it stops working. A background task that never ends, such as a dev server, keeps the session active. Bell notifications use a bell icon and a light red background with dark text, including on hover; when detected activity stops, the card enters this same bell state. Alerts on inactive tabs remain until the tab gains focus. Alerts raised while a tab has focus clear after five seconds; moving focus away keeps the alert pending until focus returns. Starting activity again does not clear a pending alert. Activity detection uses the visible preview, taskbar progress, and animated tab title; it does not distinguish task completion from waiting for input.

The drawer is enabled by default in this fork. Turn it off, or choose how many tabs (1 to 5) a window needs before it appears, in **Settings > Personalization** under **Tabs and title bar**. The same options are top-level settings in `settings.json` (hot reload is supported, and a minimum outside 1 to 5 uses the nearest limit):

```json
"experimental.showSessionDrawer": false,
"experimental.sessionDrawerMinimumTabs": 2
```

When the minimum is 1, new windows open wider so the terminal still gets its `initialCols` or `--size` columns.

Previews preserve the terminal's ANSI, 256-colour, and RGB foreground and background colours, including dim and reverse-video text. Hidden text stays hidden. Cards use the terminal background unless a custom preview colour or bell highlight overrides it. Terminal graphics and app-specific Working/Finished/Needs Input hooks are not implemented. A scrolled tab's preview follows its visible viewport and leaves its scroll position intact.

New interactive Windows PowerShell, PowerShell 7, and `wsl.exe` Bash sessions automatically report their working directory after each prompt. WSL launches support distribution names and the `--distribution-id` GUID option used by generated Ubuntu profiles. The launch wrapper loads the normal shell startup configuration, keeps custom prompts and existing Bash prompt hooks, and adds directory reporting in memory. It does not edit user profile files. WSL directories are reported as Windows paths (`C:\...` for mounted drives and `\\wsl.localhost\Ubuntu\...` for Linux folders). Existing sessions must be reopened to enable reporting. Explicit commands/scripts, unsupported shell options, and other shells keep their original launch behavior and need their own directory-reporting integration. Activity spinners in the cards use a bold 24 px glyph. Card headers reserve that glyph's height and width in idle and bell states, keeping the spinner-visible layout stable when activity changes.

## Pane notes

Every terminal pane has a small **+** button in its top right corner, in the gutter the terminal keeps for its scrollbar. Click it to open a note on top of the pane and write what the pane is for, such as `This is in a worktree called update-login`. The button becomes **−**; click it or press Esc to hide the note again, or press Tab to go back to the terminal and leave it showing. The note floats over the terminal, so it covers the top rows without resizing the terminal or reflowing its text, and it never grows past 60% of the pane. Key bindings for things like switching tabs, or the command palette, still work while typing in it. Keys for editing text, like Ctrl+V and the arrow keys, always stay in the note, and `paste`, `copy` and `sendInput` never run from it, so nothing typed or pasted in a note reaches the terminal.

A hidden note turns its **+** button the accent colour, and hovering over the button shows the note's text. Notes stay with their pane through splits, swaps, zooming, and moving a pane or tab, including to another window. They're saved with the window layout, so they come back after a restart if `firstWindowPreference` restores your layout. Duplicating a tab or pane doesn't copy its note, since the copy is a new session.

Turn notes off in **Settings > Personalization** under **Tabs and title bar**, or in `settings.json`. This hides existing notes rather than deleting them:

```json
"experimental.showPaneNotes": false
```

## Building and testing this fork

Build with the Visual Studio/C++ prerequisites listed [below](#prerequisites), then use a PowerShell 7 developer shell:

```powershell
Import-Module .\tools\OpenConsole.psm1
Set-MsBuildDevEnvironment
Invoke-OpenConsoleBuild /p:Configuration=Debug /p:Platform=x64 /m
```

Each portable build gets its own folder in [build/local/portable](build/local/portable), named `TerminalDrawer-YYYY-MM-DD-HH-MM` after its local build time, with a matching `TerminalDrawer-YYYY-MM-DD-HH-MM-x64.zip`. The names sort by build time, so the most recent build is the last `TerminalDrawer-*` folder in the list. To start it from the repository root:

```powershell
$latest = Get-ChildItem build\local\portable -Directory -Filter 'TerminalDrawer-*' | Sort-Object Name | Select-Object -Last 1
& (Join-Path $latest.FullName 'WindowsTerminal.exe')
```

Run `WindowsTerminal.exe` from its folder, or extract the ZIP; keep the DLLs and resources together. `TestPreviews.cmd` opens two live demo sessions; `TestColours.cmd` compares colour previews in dark and light schemes. Both require Python 3. The `.portable` marker keeps settings in a `settings` folder beside each build; copy that folder from the previous build to keep your settings. The nine `TestReadViewport*` native tests cover ANSI/256/RGB colours, dim and reverse video, hidden text, alternate buffers, Unicode, colour-only rewrites, wrapped rows, sparse replies, filtering, bounds, and preservation of size, selection, cursor, and scroll position. Interactive UI verification is manual.

Build output stays in the gitignored `build\local` folder: the repository's `bin` folder is a junction to `build\local\bin`, and portable builds go to `build\local\portable`. After the native build, run `D:\CodexTemp\windows-terminal-drawer-build\prepare-portable.cmd` to merge updated XAML and string resources, then `D:\CodexTemp\windows-terminal-drawer-build\package-dialog-typing.py` to package the build. The packaging script checks that the finish-notification, muted palette, and Show/Hide resources are present, and includes the colour demo launcher.

This workspace uses Visual Studio 2022 Build Tools with a v143 vcpkg overlay and the required SDK 10.0.26100.8249 XAML compiler. Those build-only overrides and the local rebuild command are in `D:\CodexTemp\windows-terminal-drawer-build\build-terminal.cmd`. The standard Visual Studio setup described below can also build/deploy `CascadiaPackage`.

After a native build, run the added `ControlCoreTests::TestReadViewportRows*` tests and `DeserializationTests::SessionDrawerSetting` with the repository's TAEF runner:

```powershell
.\build\scripts\Run-Tests.ps1 -MatchPattern Control.Unit.Tests.dll -Platform x64 -Configuration Debug -AdditionalTaefArguments '/name:*TestReadViewport*'
.\build\scripts\Run-Tests.ps1 -MatchPattern SettingsModel.Unit.Tests.dll -Platform x64 -Configuration Debug -AdditionalTaefArguments '/name:*SessionDrawerSetting*'
.\build\scripts\Run-Tests.ps1 -MatchPattern Terminal.App.Unit.Tests.dll -Platform x64 -Configuration Debug -AdditionalTaefArguments '/name:*SessionFinishNotifierTests*'
.\build\scripts\Run-Tests.ps1 -MatchPattern Terminal.App.Unit.Tests.dll -Platform x64 -Configuration Debug -AdditionalTaefArguments '/name:*SessionActivityTests*'
```

`SessionFinishNotifierTests` cover the finish-notification timing (both delays, resumed activity, user activity, zero delay) and callback URL validation. `SessionActivityTests` cover Claude Code's footer task summaries, working rows and wrapped turn summaries in panes too narrow for the footer, the latest-summary rule, and text that must not count, such as a shell prompt after Claude exits. A UI Automation smoke test of the Release build armed three working tabs with a 3 second delay and a local callback listener. The tab left alone received its `POST` 6.5 seconds after it stopped. The tab that resumed activity two seconds after stopping received its `POST` 6.4 seconds after its second stop. The tab selected one second after it stopped received nothing. Windows recorded a desktop notification for each of the first two. The dialog validated URLs, restored the saved delay and URL after a restart, and kept the item disabled for an idle tab. The speaker icon and orange background appeared only while a request was pending. The sound, the taskbar flash, and clicking a notification were not checked automatically.

For UI verification, run `python tools/session-drawer-demo.py --label One --sparse` and `--label Two` in separate tabs (in WSL, use `python3` and the corresponding mounted path). The sparse demo places a reply above a large blank gap and keeps input at the bottom; the other demo displays a changing counter, grid dimensions, blank-row rewrites, and periodic bells. Both should remain readable in previews. Switch away and confirm the counter keeps advancing in the preview. Click the card and compare its text with the main screen, allowing for skipped blank rows; the grid should be unchanged by preview polling. Then test a real Claude/Codex session, a split tab, tab close/reorder/move, drawer scrolling, keyboard activation, focus mode, minimize/restore, and toggling the setting. Leave a text selection and a scrolled viewport in an inactive tab and verify both survive refreshes. Ctrl+C restores the demo's original screen.

For alerts, leave a tab inactive while its activity spinner stops and confirm the bell icon and red background persist for more than five seconds, even if activity resumes. Focus that tab and confirm the alert clears. Repeat with `printf '\a'` in an inactive tab. With the tab already focused, trigger each alert and confirm it lasts five seconds. Move focus away before the timeout, wait longer than five seconds, and confirm it stays highlighted until focus returns. Reorder tabs or hide/show the drawer while an alert is pending and confirm the state survives.

For background tasks, run `python tools/session-drawer-demo.py --label Bg --background` in an inactive tab, then again in a pane about 32 columns wide, where the footer is clipped before its task item and interrupt hint as Claude Code's would be (the first row shows the width). Each 20 second cycle shows a working row for 2 seconds (full-speed spinner), a finished turn ending `1 shell, 1 monitor still running` for 8 seconds (dimmed, half-speed spinner, no alert), then a final turn without tasks. The bell alert should appear only then, even though an earlier turn's `still running` note stays in view. Bells are off in this mode unless `--bell-every` is given. UI Automation smoke tests of the Release build at 40 and 32 columns saw this sequence: the inactive card's spinner changed about four times a second for 2 seconds, about twice a second for the next 8, then the bell appeared. At 32 columns the footer showed only `⏵⏵ auto mode on · 1 shell, 1`. A window capture at 120 columns showed the dimmed spinner.

For drawer scrolling, open enough tabs to require scrolling, scroll into the middle or to the bottom, then close an active or inactive tab. Confirm the drawer keeps its position, moving only as far as the new bottom when necessary. Also check adding/reordering tabs while scrolled down.

For the bottom buttons, open duplicate profiles and four distinct profiles, including one in an unfocused split pane. Confirm each profile appears once, at most three buttons appear, and closing every pane of a listed profile brings the next profile into the row. Click a button and activate it with Enter/Space to verify it opens the named profile in a new tab. Scroll the cards and resize the drawer to confirm the buttons stay at the bottom and long names have full tooltips.

For the arrow button, resize the drawer, hide it, and show it again; confirm the width is restored and the main terminal fills the freed space while hidden. Check the arrow direction, Show/Hide tooltip, keyboard Enter/Space activation, and terminal input focus afterward. While collapsed, add/remove tabs, enter/leave focus mode, and toggle the drawer setting; the collapsed choice should survive and the button should appear only with multiple tabs, the drawer setting enabled, and focus mode off. Resize the window in both states with grid snapping enabled.

For pane notes, run `DeserializationTests::PaneNote*`, then `smoke-pane-notes.ps1`, `pane-notes-leak-check.ps1` and `pane-notes-key-sweep.ps1` from `D:\CodexTemp\windows-terminal-drawer-build`. They drive an isolated copy of a portable build with real mouse and keyboard input, so they take the foreground while they run. The leak check runs a key logger in the pane and confirms that nothing typed or pasted in a note reaches the terminal, which matters because the note's text box edits text without marking keys as handled.

For the preview context menu, right-click an inactive card and confirm it stays inactive. Choose a preset and a custom background colour, check text readability on light/dark colours and on hover, then use Clear. Set different colours on two cards, add/reorder/close another tab, enter/leave focus mode, and reduce to one tab before opening a second: each surviving tab should retain its colour. Ring a bell in a coloured tab and confirm its chosen colour returns when the notification clears. Use Close on an inactive card and confirm the correct tab closes; also check read-only/confirm-on-close prompts and cancel. Focus a card and use Shift+F10 to check keyboard access.

For finish notifications, run `Write-Host 'Working (esc to interrupt)'; Start-Sleep 20; Clear-Host` in one tab of a two-tab window, right-click its card while the spinner shows, and choose a 5 second delay with a test ntfy topic URL (or leave the URL empty). Switch to the other tab or another app. About 5 seconds after the spinner stops, expect the sound, a taskbar flash, and a desktop notification; about 5 seconds later, expect the ntfy message. Repeat and, after the spinner stops, type in, click, or switch to the tab before each step; nothing further should happen and the speaker icon should disappear. Also check that the item is disabled on an idle card, that **Cancel finish notification** removes the speaker and orange background, that a card with its own preview colour keeps it while pending, that clicking the notification switches to the tab and stops the callback, that you can type in both of its fields, and that it offers the last delay and URL after restarting Terminal.

---

![Windows Terminal project logos and branding image](https://github.com/microsoft/terminal/assets/91625426/333ddc76-8ab2-4eb4-a8c0-4d7b953b1179)

[![Terminal Build Status](https://dev.azure.com/shine-oss/terminal/_apis/build/status%2FTerminal%20CI?branchName=main)](https://dev.azure.com/shine-oss/terminal/_build/latest?definitionId=1&branchName=main)

# Welcome to the Windows Terminal, Console and Command-Line repo

<details>
  <summary><strong>Table of Contents</strong></summary>

- [Installing and running Windows Terminal](#installing-and-running-windows-terminal)
  - [Microsoft Store \[Recommended\]](#microsoft-store-recommended)
  - [Other install methods](#other-install-methods)
    - [Via GitHub](#via-github)
    - [Via Windows Package Manager CLI (aka winget)](#via-windows-package-manager-cli-aka-winget)
    - [Via Chocolatey (unofficial)](#via-chocolatey-unofficial)
    - [Via Scoop (unofficial)](#via-scoop-unofficial)
- [Installing Windows Terminal Canary](#installing-windows-terminal-canary)
- [Terminal \& Console Overview](#terminal--console-overview)
  - [Windows Terminal](#windows-terminal)
  - [The Windows Console Host](#the-windows-console-host)
  - [Shared Components](#shared-components)
  - [Creating the new Windows Terminal](#creating-the-new-windows-terminal)
- [Resources](#resources)
- [FAQ](#faq)
  - [I built and ran the new Terminal, but it looks just like the old console](#i-built-and-ran-the-new-terminal-but-it-looks-just-like-the-old-console)
- [Documentation](#documentation)
- [Contributing](#contributing)
- [Communicating with the Team](#communicating-with-the-team)
- [Developer Guidance](#developer-guidance)
- [Prerequisites](#prerequisites)
- [Building the Code](#building-the-code)
  - [Building in PowerShell](#building-in-powershell)
  - [Building in Cmd](#building-in-cmd)
- [Running \& Debugging](#running--debugging)
  - [Coding Guidance](#coding-guidance)
- [Code of Conduct](#code-of-conduct)

</details>

<br />

This repository contains the source code for:

* [Windows Terminal](https://aka.ms/terminal)
* [Windows Terminal Preview](https://aka.ms/terminal-preview)
* The Windows console host (`conhost.exe`)
* Components shared between the two projects
* [ColorTool](./src/tools/ColorTool)
* [Sample projects](./samples)
  that show how to consume the Windows Console APIs

Related repositories include:

* [Windows Terminal Documentation](https://learn.microsoft.com/windows/terminal)
  ([Repo: Contribute to the docs](https://github.com/MicrosoftDocs/terminal))
* [Console API Documentation](https://github.com/MicrosoftDocs/Console-Docs)
* [Cascadia Code Font](https://github.com/Microsoft/Cascadia-Code)

## Installing and running Windows Terminal

> [!NOTE]
> Windows Terminal requires Windows 10 2004 (build 19041) or later

### Microsoft Store [Recommended]

Install the [Windows Terminal from the Microsoft Store][store-install-link].
This allows you to always be on the latest version when we release new builds
with automatic upgrades.

This is our preferred method.

### Other install methods

#### Via GitHub

For users who are unable to install Windows Terminal from the Microsoft Store,
released builds can be manually downloaded from this repository's [Releases
page](https://github.com/microsoft/terminal/releases).

Download the `Microsoft.WindowsTerminal_<versionNumber>.msixbundle` file from
the **Assets** section. To install the app, you can simply double-click on the
`.msixbundle` file, and the app installer should automatically run. If that
fails for any reason, you can try the following command at a PowerShell prompt:

```powershell
# NOTE: If you are using PowerShell 7+, please run
# Import-Module Appx -UseWindowsPowerShell
# before using Add-AppxPackage.

Add-AppxPackage Microsoft.WindowsTerminal_<versionNumber>.msixbundle
```

> [!NOTE]
> If you install Terminal manually:
>
> * You may need to install the [VC++ v14 Desktop Framework Package](https://learn.microsoft.com/troubleshoot/cpp/c-runtime-packages-desktop-bridge#how-to-install-and-update-desktop-framework-packages).
>   This should only be necessary on older builds of Windows 10 and only if you get an error about missing framework packages.
> * Terminal will not auto-update when new builds are released so you will need
>   to regularly install the latest Terminal release to receive all the latest
>   fixes and improvements!

#### Via Windows Package Manager CLI (aka winget)

[winget](https://github.com/microsoft/winget-cli) users can download and install
the latest Terminal release by installing the `Microsoft.WindowsTerminal`
package:

```powershell
winget install --id Microsoft.WindowsTerminal -e
```

> [!NOTE]
> Dependency support is available in WinGet version [1.6.2631 or later](https://github.com/microsoft/winget-cli/releases). To install the Terminal stable release 1.18 or later, please make sure you have the updated version of the WinGet client.

#### Via Chocolatey (unofficial)

[Chocolatey](https://chocolatey.org) users can download and install the latest
Terminal release by installing the `microsoft-windows-terminal` package:

```powershell
choco install microsoft-windows-terminal
```

To upgrade Windows Terminal using Chocolatey, run the following:

```powershell
choco upgrade microsoft-windows-terminal
```

If you have any issues when installing/upgrading the package please go to the
[Windows Terminal package
page](https://chocolatey.org/packages/microsoft-windows-terminal) and follow the
[Chocolatey triage process](https://chocolatey.org/docs/package-triage-process)

#### Via Scoop (unofficial)

[Scoop](https://scoop.sh) users can download and install the latest Terminal
release by installing the `windows-terminal` package:

```powershell
scoop bucket add extras
scoop install windows-terminal
```

To update Windows Terminal using Scoop, run the following:

```powershell
scoop update windows-terminal
```

If you have any issues when installing/updating the package, please search for
or report the same on the [issues
page](https://github.com/lukesampson/scoop-extras/issues) of Scoop Extras bucket
repository.

---

## Installing Windows Terminal Canary
Windows Terminal Canary is a nightly build of Windows Terminal. This build has the latest code from our `main` branch, giving you an opportunity to try features before they make it to Windows Terminal Preview.

Windows Terminal Canary is our least stable offering, so you may discover bugs before we have had a chance to find them.

Windows Terminal Canary is available as an App Installer distribution and a Portable ZIP distribution.

The App Installer distribution supports automatic updates. Due to platform limitations, this installer only works on Windows 11.

The Portable ZIP distribution is a portable application. It will not automatically update and will not automatically check for updates. This portable ZIP distribution works on Windows 10 (19041+) and Windows 11.

| Distribution  | Architecture    | Link                                                 |
|---------------|:---------------:|------------------------------------------------------|
| App Installer | x64, arm64, x86 | [Download](https://aka.ms/terminal-canary-installer) |
| Portable ZIP  | x64             | [Download](https://aka.ms/terminal-canary-zip-x64)   |
| Portable ZIP  | ARM64           | [Download](https://aka.ms/terminal-canary-zip-arm64) |
| Portable ZIP  | x86             | [Download](https://aka.ms/terminal-canary-zip-x86)   |

_Learn more about the [types of Windows Terminal distributions](https://learn.microsoft.com/windows/terminal/distributions)._

---

## Terminal & Console Overview

Please take a few minutes to review the overview below before diving into the
code:

### Windows Terminal

Windows Terminal is a new, modern, feature-rich, productive terminal application
for command-line users. It includes many of the features most frequently
requested by the Windows command-line community including support for tabs, rich
text, globalization, configurability, theming & styling, and more.

The Terminal will also need to meet our goals and measures to ensure it remains
fast and efficient, and doesn't consume vast amounts of memory or power.

### The Windows Console Host

The Windows Console host, `conhost.exe`, is Windows' original command-line user
experience. It also hosts Windows' command-line infrastructure and the Windows
Console API server, input engine, rendering engine, user preferences, etc. The
console host code in this repository is the actual source from which the
`conhost.exe` in Windows itself is built.

Since taking ownership of the Windows command-line in 2014, the team added
several new features to the Console, including background transparency,
line-based selection, support for [ANSI / Virtual Terminal
sequences](https://en.wikipedia.org/wiki/ANSI_escape_code), [24-bit
color](https://devblogs.microsoft.com/commandline/24-bit-color-in-the-windows-console/),
a [Pseudoconsole
("ConPTY")](https://devblogs.microsoft.com/commandline/windows-command-line-introducing-the-windows-pseudo-console-conpty/),
and more.

However, because Windows Console's primary goal is to maintain backward
compatibility, we have been unable to add many of the features the community
(and the team) have been wanting for the last several years including tabs,
unicode text, and emoji.

These limitations led us to create the new Windows Terminal.

> You can read more about the evolution of the command-line in general, and the
> Windows command-line specifically in [this accompanying series of blog
> posts](https://devblogs.microsoft.com/commandline/windows-command-line-backgrounder/)
> on the Command-Line team's blog.

### Shared Components

While overhauling Windows Console, we modernized its codebase considerably,
cleanly separating logical entities into modules and classes, introduced some
key extensibility points, replaced several old, home-grown collections and
containers with safer, more efficient [STL
containers](https://docs.microsoft.com/en-us/cpp/standard-library/stl-containers?view=vs-2022),
and made the code simpler and safer by using Microsoft's [Windows Implementation
Libraries - WIL](https://github.com/Microsoft/wil).

This overhaul resulted in several of Console's key components being available
for re-use in any terminal implementation on Windows. These components include a
new DirectWrite-based text layout and rendering engine, a text buffer capable of
storing both UTF-16 and UTF-8, a VT parser/emitter, and more.

### Creating the new Windows Terminal

When we started planning the new Windows Terminal application, we explored and
evaluated several approaches and technology stacks. We ultimately decided that
our goals would be best met by continuing our investment in our C++ codebase,
which would allow us to reuse several of the aforementioned modernized
components in both the existing Console and the new Terminal. Further, we
realized that this would allow us to build much of the Terminal's core itself as
a reusable UI control that others can incorporate into their own applications.

The result of this work is contained within this repo and delivered as the
Windows Terminal application you can download from the Microsoft Store, or
[directly from this repo's
releases](https://github.com/microsoft/terminal/releases).

---

## Resources

For more information about Windows Terminal, you may find some of these
resources useful and interesting:

* [Command-Line Blog](https://devblogs.microsoft.com/commandline)
* [Command-Line Backgrounder Blog
  Series](https://devblogs.microsoft.com/commandline/windows-command-line-backgrounder/)
* Windows Terminal Launch: [Terminal "Sizzle
  Video"](https://www.youtube.com/watch?v=8gw0rXPMMPE&list=PLEHMQNlPj-Jzh9DkNpqipDGCZZuOwrQwR&index=2&t=0s)
* Windows Terminal Launch: [Build 2019
  Session](https://www.youtube.com/watch?v=KMudkRcwjCw)
* Run As Radio: [Show 645 - Windows Terminal with Richard
  Turner](https://www.runasradio.com/Shows/Show/645)
* Azure DevOps Podcast: [Episode 54 - Kayla Cinnamon and Rich Turner on DevOps
  on the Windows
  Terminal](http://azuredevopspodcast.clear-measure.com/kayla-cinnamon-and-rich-turner-on-devops-on-the-windows-terminal-team-episode-54)
* Microsoft Ignite 2019 Session: [The Modern Windows Command Line: Windows
  Terminal -
  BRK3321](https://myignite.techcommunity.microsoft.com/sessions/81329?source=sessions)

---

## FAQ

### I built and ran the new Terminal, but it looks just like the old console

Cause: You're launching the incorrect solution in Visual Studio.

Solution: Make sure you're building & deploying the `CascadiaPackage` project in
Visual Studio.

> [!NOTE]
> `OpenConsole.exe` is just a locally-built `conhost.exe`, the classic
> Windows Console that hosts Windows' command-line infrastructure. OpenConsole
> is used by Windows Terminal to connect to and communicate with command-line
> applications (via
> [ConPty](https://devblogs.microsoft.com/commandline/windows-command-line-introducing-the-windows-pseudo-console-conpty/)).

---

## Documentation

All project documentation is located at [aka.ms/terminal-docs](https://aka.ms/terminal-docs). If you would like
to contribute to the documentation, please submit a pull request on the [Windows
Terminal Documentation repo](https://github.com/MicrosoftDocs/terminal).

---

## Contributing

We are excited to work alongside you, our amazing community, to build and
enhance Windows Terminal\!

***BEFORE you start work on a feature/fix***, please read & follow our
[Contributor's
Guide](./CONTRIBUTING.md) to
help avoid any wasted or duplicate effort.

## Communicating with the Team

The easiest way to communicate with the team is via GitHub issues.

Please file new issues, feature requests and suggestions, but **DO search for
similar open/closed preexisting issues before creating a new issue.**

If you would like to ask a question that you feel doesn't warrant an issue
(yet), please reach out to us via Twitter:

* Christopher Nguyen, Product Manager:
  [@nguyen_dows](https://twitter.com/nguyen_dows)
* Dustin Howett, Engineering Lead: [@dhowett](https://twitter.com/DHowett)
* Mike Griese, Senior Developer: [@zadjii@mastodon.social](https://mastodon.social/@zadjii)
* Carlos Zamora, Developer: [@cazamor_msft](https://twitter.com/cazamor_msft)
* Pankaj Bhojwani, Developer
* Leonard Hecker, Developer: [@LeonardHecker](https://twitter.com/LeonardHecker)

## Developer Guidance

## Prerequisites

You can configure your environment to build Terminal in one of two ways:

### Using WinGet configuration file

After cloning the repository, you can use a [WinGet configuration file](https://learn.microsoft.com/en-us/windows/package-manager/configuration/#use-a-winget-configuration-file-to-configure-your-machine)
to set up your environment. The [default configuration file](.config/configuration.winget) installs Visual Studio 2026 Community & rest of the required tools. There are two other variants of the configuration file available in the [.config](.config) directory for Enterprise & Professional editions of Visual Studio 2026. To run the default configuration file, you can either double-click the file from explorer or run the following command:

```powershell
winget configure .config\configuration.winget
```

### Manual configuration

* You must be running Windows 10 2004 (build >= 10.0.19041.0) or later to run
  Windows Terminal
* You must [enable Developer Mode in the Windows Settings
  app](https://learn.microsoft.com/windows/uwp/get-started/enable-your-device-for-development)
  to locally install and run Windows Terminal
* You must have [PowerShell 7 or later](https://github.com/PowerShell/PowerShell/releases/latest) installed
* You must have the [Windows 11 (10.0.26100) SDK](https://developer.microsoft.com/windows/downloads/windows-sdk/) installed at version 10.0.26100.8249 or greater.
* You must have at least [VS 2026](https://visualstudio.microsoft.com/downloads/) version 18.6 installed
* You must install the following Workloads via the VS Installer. Note: Opening
  the solution will [prompt you to install missing components automatically](https://devblogs.microsoft.com/setup/configure-visual-studio-across-your-organization-with-vsconfig/):
  * Desktop Development with C++
  * WinUI application development
* You must install the [.NET Framework 4.7.2 Targeting Pack](https://learn.microsoft.com/dotnet/framework/install/guide-for-developers#to-install-the-net-framework-developer-pack-or-targeting-pack) to build test projects

## Building the Code

OpenConsole.slnx may be built from within Visual Studio or from the command-line
using a set of convenience scripts & tools in the **/tools** directory:

### Building in PowerShell

```powershell
Import-Module .\tools\OpenConsole.psm1
Set-MsBuildDevEnvironment
Invoke-OpenConsoleBuild
```

### Building in Cmd

```shell
.\tools\razzle.cmd
bcz
```

## Running & Debugging

To debug the Windows Terminal in VS, right click on `CascadiaPackage` (in the
Solution Explorer) and go to properties. In the Debug menu, change "Application
process" and "Background task process" to "Native Only".

You should then be able to build & debug the Terminal project by hitting
<kbd>F5</kbd>. Make sure to select either the "x64" or the "x86" platform - the
Terminal doesn't build for "Any Cpu" (because the Terminal is a C++ application,
not a C# one).

> 👉 You will _not_ be able to launch the Terminal directly by running the
> WindowsTerminal.exe. For more details on why, see
> [#926](https://github.com/microsoft/terminal/issues/926),
> [#4043](https://github.com/microsoft/terminal/issues/4043)

### Coding Guidance

Please review these brief docs below about our coding practices.

> 👉 If you find something missing from these docs, feel free to contribute to
> any of our documentation files anywhere in the repository (or write some new
> ones!)

This is a work in progress as we learn what we'll need to provide people in
order to be effective contributors to our project.

* [Coding Style](./doc/STYLE.md)
* [Code Organization](./doc/ORGANIZATION.md)
* [Exceptions in our legacy codebase](./doc/EXCEPTIONS.md)
* [Helpful smart pointers and macros for interfacing with Windows in WIL](./doc/WIL.md)

---

## Code of Conduct

This project has adopted the [Microsoft Open Source Code of
Conduct][conduct-code]. For more information see the [Code of Conduct
FAQ][conduct-FAQ] or contact [opencode@microsoft.com][conduct-email] with any
additional questions or comments.

[conduct-code]: https://opensource.microsoft.com/codeofconduct/
[conduct-FAQ]: https://opensource.microsoft.com/codeofconduct/faq/
[conduct-email]: mailto:opencode@microsoft.com
[store-install-link]: https://aka.ms/terminal
