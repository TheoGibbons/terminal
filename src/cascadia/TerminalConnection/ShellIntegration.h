// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include <windows.h>
#include <shellapi.h>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace Microsoft::Terminal::ShellIntegration
{
    // Quote one CreateProcess argument, including embedded quotes and trailing
    // backslashes. Shell code is a single argument, never terminal input.
    inline std::wstring QuoteArgument(const std::wstring_view value)
    {
        std::wstring result{ L"\"" };
        size_t backslashes = 0;
        for (const auto ch : value)
        {
            if (ch == L'\\')
            {
                ++backslashes;
                continue;
            }
            result.append(backslashes * (ch == L'"' ? 2 : 1) + (ch == L'"' ? 1 : 0), L'\\');
            result.push_back(ch);
            backslashes = 0;
        }
        result.append(backslashes * 2, L'\\');
        result.push_back(L'"');
        return result;
    }

    inline constexpr std::wstring_view PowerShellInit = LR"wt(
$global:__WTSessionDrawerOriginalPrompt = $function:prompt
function global:prompt {
    $promptText = & $global:__WTSessionDrawerOriginalPrompt
    $directory = $ExecutionContext.SessionState.Path.CurrentFileSystemLocation.ProviderPath
    [Console]::Write([char]27 + ']9;9;' + $directory + [char]27 + '\')
    $promptText
}
)wt";

    // WSL normally starts a login shell. Load its startup files before adding
    // our hook, keeping the user's prompt and existing PROMPT_COMMAND entries.
    // The here-document is an inherited descriptor, so no files are installed
    // in the distro or in the user's shell configuration.
    inline constexpr std::wstring_view WslInit = LR"wt(
if [ "${SHELL##*/}" != bash ]; then
    exec "${SHELL:-/bin/sh}" -l
fi
exec "$SHELL" --rcfile /dev/fd/3 -i 3<<'__WT_SESSION_DRAWER_INIT__'
if [[ -r /etc/profile ]]; then . /etc/profile; fi
for __wt_profile in "$HOME/.bash_profile" "$HOME/.bash_login" "$HOME/.profile"; do
    if [[ -r $__wt_profile ]]; then . "$__wt_profile"; break; fi
done
unset __wt_profile
__wt_session_drawer_cwd() {
    local previous_status=$?
    local directory
    if directory=$(wslpath -w "$PWD" 2>/dev/null); then
        printf '\033]9;9;%s\033\\' "$directory"
    fi
    return "$previous_status"
}
PROMPT_COMMAND+=(__wt_session_drawer_cwd)
__WT_SESSION_DRAWER_INIT__
)wt";

    // Only augment interactive shell launches. Explicit commands, scripts,
    // administrative WSL invocations, and unknown options keep their semantics.
    inline std::wstring PrepareCommandline(const std::wstring& commandline)
    {
        int argc = 0;
        std::unique_ptr<wchar_t*, decltype(&LocalFree)> argv{ CommandLineToArgvW(commandline.c_str(), &argc), &LocalFree };
        if (!argv || argc == 0)
        {
            return commandline;
        }
        const auto executable = std::filesystem::path{ argv.get()[0] }.filename().wstring();
        const auto is = [](const std::wstring_view a, const std::wstring_view b) {
            return CompareStringOrdinal(a.data(), static_cast<int>(a.size()), b.data(), static_cast<int>(b.size()), TRUE) == CSTR_EQUAL;
        };
        const auto powershell = is(executable, L"powershell.exe") || is(executable, L"powershell") ||
                                is(executable, L"pwsh.exe") || is(executable, L"pwsh");
        const auto wsl = is(executable, L"wsl.exe") || is(executable, L"wsl");
        if (!powershell && !wsl)
        {
            return commandline;
        }

        bool noExit = false;
        for (int i = 1; i < argc; ++i)
        {
            const std::wstring_view arg{ argv.get()[i] };
            if (powershell)
            {
                if (is(arg, L"-NoExit"))
                {
                    noExit = true;
                }
                else if (is(arg, L"-NoLogo") || is(arg, L"-NoProfile"))
                {
                }
                else if ((is(arg, L"-ExecutionPolicy") || is(arg, L"-WorkingDirectory")) && i + 1 < argc)
                {
                    ++i;
                }
                else
                {
                    return commandline;
                }
            }
            // Generated WSL profiles select the distro by GUID rather than name.
            else if ((arg == L"-d" || arg == L"--distribution" || arg == L"--distribution-id" || arg == L"-u" || arg == L"--user" || arg == L"--cd") && i + 1 < argc)
            {
                ++i;
            }
            else
            {
                return commandline;
            }
        }
        if (powershell)
        {
            return commandline + (noExit ? L" -Command " : L" -NoExit -Command ") + QuoteArgument(PowerShellInit);
        }
        return commandline + L" --exec /bin/sh -c " + QuoteArgument(WslInit);
    }
}
