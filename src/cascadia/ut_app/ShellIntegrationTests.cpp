// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "precomp.h"
#include "../TerminalConnection/ShellIntegration.h"

using namespace WEX::TestExecution;
using namespace Microsoft::Terminal::ShellIntegration;

namespace TerminalAppUnitTests
{
    class ShellIntegrationTests
    {
        BEGIN_TEST_CLASS(ShellIntegrationTests)
        END_TEST_CLASS()

        TEST_METHOD(InteractiveShells)
        {
            for (const auto command : { L"powershell.exe", L"PWSH.EXE -NoLogo", L"pwsh -NoProfile -NoExit", L"pwsh -ExecutionPolicy RemoteSigned" })
            {
                const auto result = PrepareCommandline(command);
                VERIFY_IS_TRUE(result.starts_with(command));
                int argc;
                std::unique_ptr<wchar_t*, decltype(&LocalFree)> args{ CommandLineToArgvW(result.c_str(), &argc), &LocalFree };
                VERIFY_IS_NOT_NULL(args.get());
                VERIFY_ARE_EQUAL(PowerShellInit, std::wstring_view{ args.get()[argc - 1] });
            }
            for (const auto command : { L"wsl.exe", L"wsl -d Ubuntu --cd /tmp", L"\"C:\\Windows\\System32\\wsl.exe\" --distribution Ubuntu --user test",
                                        LR"("C:\Windows\System32\wsl.exe" --cd "~" --distribution-id {5a238a40-3a37-4a12-98b5-def823946ada})" })
            {
                const auto result = PrepareCommandline(command);
                VERIFY_IS_TRUE(result.starts_with(command));
                VERIFY_IS_TRUE(result.find(L" --exec /bin/sh -c ") != std::wstring::npos);
                int argc;
                std::unique_ptr<wchar_t*, decltype(&LocalFree)> args{ CommandLineToArgvW(result.c_str(), &argc), &LocalFree };
                VERIFY_IS_NOT_NULL(args.get());
                VERIFY_ARE_EQUAL(WslInit, std::wstring_view{ args.get()[argc - 1] });
            }
        }

        TEST_METHOD(ExplicitCommandsUnchanged)
        {
            for (const auto command : { L"cmd.exe", L"powershell -File script.ps1", L"pwsh -Command Get-Date", L"pwsh -NonInteractive",
                                        L"wsl --shutdown", L"wsl -d Ubuntu -- ls /tmp", L"wsl --exec bash", L"wsl -d", L"wsl --user",
                                        L"wsl --distribution-id", L"wsl --distribution-id {5a238a40-3a37-4a12-98b5-def823946ada} --exec bash" })
            {
                VERIFY_ARE_EQUAL(std::wstring{ command }, PrepareCommandline(command));
            }
        }

        TEST_METHOD(ArgumentQuoting)
        {
            for (const auto value : { L"", L"path with spaces\\", L"quote\"and\\\"backslash", L"semi;colon", L"line\nfeed" })
            {
                const auto command = L"app.exe " + QuoteArgument(value);
                int argc;
                std::unique_ptr<wchar_t*, decltype(&LocalFree)> args{ CommandLineToArgvW(command.c_str(), &argc), &LocalFree };
                VERIFY_IS_NOT_NULL(args.get());
                VERIFY_ARE_EQUAL(2, argc);
                VERIFY_ARE_EQUAL(std::wstring_view{ value }, std::wstring_view{ args.get()[1] });
            }
        }
    };
}
