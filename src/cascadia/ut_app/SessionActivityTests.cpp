// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "precomp.h"
#include "../TerminalApp/SessionActivity.h"

using namespace WEX::TestExecution;
using ::TerminalApp::ClaudeSessionActivity;
using ::TerminalApp::RemoveClaudeFooterClutter;
using ::TerminalApp::SessionActivity;

namespace TerminalAppUnitTests
{
    // Stands in for PreviewTextRun, whose other members are its style.
    struct StyledRun
    {
        std::wstring Text;
        int Style;

        bool operator==(const StyledRun&) const = default;
    };

    // The card text left after removing the footer's clutter.
    static std::wstring CardText(const std::wstring_view rows)
    {
        std::vector<StyledRun> runs;
        for (size_t start = 0;;)
        {
            const auto end = rows.find(L'\n', start);
            runs.push_back({ std::wstring{ rows.substr(start, end - start) }, 0 });
            if (end == std::wstring_view::npos)
            {
                break;
            }
            runs.push_back({ L"\n", 0 });
            start = end + 1;
        }
        std::wstring text{ rows };
        RemoveClaudeFooterClutter(runs, text);
        std::wstring joined;
        for (const auto& run : runs)
        {
            joined.append(run.Text);
        }
        VERIFY_IS_TRUE(joined == text, text.c_str());
        return text;
    }

    class SessionActivityTests
    {
        BEGIN_TEST_CLASS(SessionActivityTests)
        END_TEST_CLASS()

        TEST_METHOD(FooterListsBackgroundTasks)
        {
            // Claude Code 2.1.302's task summaries, beside other footer items.
            for (const auto footer : { L"  \u23f5\u23f5 auto mode on \u00b7 install gh for PR status \u00b7 1 shell, 1 monitor \u00b7 \u2193 to manage",
                                       L"  \u23f5\u23f5 auto mode on \u00b7 2 shells",
                                       L"  3 monitors \u00b7 \u2193 to manage",
                                       L"  ? for shortcuts \u00b7 1 Artifact comment monitor",
                                       L"  1 team \u00b7 \u2193 to manage",
                                       L"  2 local agents \u00b7 \u2193 to manage",
                                       L"  \u25c7 1 cloud session \u00b7 \u2193 to manage",
                                       L"  \u25c7 2 remote dynamic workflows",
                                       L"  1 background dynamic workflow",
                                       L"  1 MCP task",
                                       L"  4 background tasks",
                                       L"  \u21b3 2 background \u00b7 \u2193 to manage",
                                       L"  1 shell, 2 monitors            \u25ef IDE" })
            {
                VERIFY_IS_TRUE(ClaudeSessionActivity(footer) == SessionActivity::Background, footer);
            }
        }

        TEST_METHOD(NarrowPaneUsesLatestTurn)
        {
            // The footer is clipped before its task item, but the turn's
            // summary wraps onto indented rows.
            constexpr std::wstring_view footer{ L"  \u23f5\u23f5 auto mode on \u00b7 install gh for PR st" };
            for (const auto rows : { L"\u273b Cooked for 8s \u00b7 done 7:40 PM \u00b7 1 shell, 1\n  monitor still running\n",
                                     L"\u273b Cooked for 8s\n  \u00b7 done 7:40 PM \u00b7 1 shell, 1 monitor still\n  running\n",
                                     L"\u273b Cooked for 8s \u00b7 done 7:40 PM \u00b7 1 shell, 1 monitor still running\n"
                                     L"\u25cf How is Claude doing this session? (optional)\n  1: Bad    2: Fine   3: Good   0: Dismiss\n",
                                     L"\u273b Brewed for 4s \u00b7 done 5:57 PM \u00b7 2 local agents still running\n\u276f next, check the\n  results\n",
                                     L"\u273b Waiting for 2 background agents and 1 dynamic workflow to finish\n" })
            {
                VERIFY_IS_TRUE(ClaudeSessionActivity(rows + std::wstring{ footer }) == SessionActivity::Background, rows);
            }
        }

        TEST_METHOD(LatestTurnCounts)
        {
            // An earlier turn's note stays in view after its tasks end.
            constexpr std::wstring_view earlier{ L"\u273b Crunched for 6s \u00b7 done 5:55 PM \u00b7 1 shell, 1 monitor still running\n\u25cf All runs are done.\n" };
            VERIFY_IS_TRUE(ClaudeSessionActivity(std::wstring{ earlier } + L"\u273b Cooked for 3s \u00b7 done 5:58 PM\n  ? for shortcuts") == SessionActivity::Idle);
            // A footer wide enough for its task item counts by itself.
            VERIFY_IS_TRUE(ClaudeSessionActivity(L"\u273b Cooked for 3s \u00b7 done 5:58 PM\n  1 shell \u00b7 \u2193 to manage") == SessionActivity::Background);
            // A shell prompt after Claude exits makes the rows above history.
            VERIFY_IS_TRUE(ClaudeSessionActivity(L"\u273b Cooked for 3s \u00b7 done 5:58 PM \u00b7 1 shell still running\nPS C:\\>") == SessionActivity::Idle);
            VERIFY_IS_TRUE(ClaudeSessionActivity(L"\u273b Cooked for 3s \u00b7 done 5:58 PM \u00b7 1 shell still running\ntheo@host:~$") == SessionActivity::Idle);
        }

        TEST_METHOD(WorkingRowCounts)
        {
            // These footers are clipped before the interrupt hint.
            constexpr std::wstring_view footer{ L"  \u23f5\u23f5 auto mode on \u00b7 1 shell, 1 monitor \u00b7 esc to" };
            for (const auto rows : { L"\u2736 Determining\u2026 (9m 27s \u00b7 \u2193 53.3k tokens)\n  \u23bf  Tip: Use /btw to ask a quick side question\n",
                                     L"\u2722 Cogitating\u2026\n",
                                     L"* Clauding\u2026 (2s)\n",
                                     L"\u273b Brewed for 4s \u00b7 done 5:57 PM \u00b7 1 shell, 1 monitor still running\n\u25cf Monitor event: RESULT\n\u273b Determining\u2026 (3s)\n" })
            {
                VERIFY_IS_TRUE(ClaudeSessionActivity(rows + std::wstring{ footer }) == SessionActivity::Working, rows);
            }
        }

        TEST_METHOD(OtherTextIgnored)
        {
            for (const auto rows : { L"",
                                     L"\u273b Cooked for 4s \u00b7 done 5:58 PM \u00b7 1 shell, 1 monitor still running",
                                     L"  \u23f5\u23f5 auto mode on \u00b7 ? for shortcuts",
                                     L"  dreaming \u00b7 \u2193 to manage",
                                     L"\u25cf 2 background agents launched (\u2193 to manage)",
                                     L"  Done (3 tool uses \u00b7 12.5k tokens \u00b7 20s)",
                                     L"I started 1 shell for the benchmark",
                                     L"  1 shellfish",
                                     L"  3 files changed",
                                     L"  1 shell\n  \u23f5\u23f5 auto mode on",
                                     L"* Loading the data\u2026\nPS C:\\>",
                                     L"\u00b7 still running\n  \u23f5\u23f5 auto mode on" })
            {
                VERIFY_IS_TRUE(ClaudeSessionActivity(rows) == SessionActivity::Idle, rows);
            }
        }

        TEST_METHOD(FooterClutterRemoved)
        {
            // Claude Code 2.1.305's footers, whole and clipped by a narrow pane.
            const std::pair<std::wstring_view, std::wstring_view> footers[]{
                { L"  \u23f5\u23f5 auto mode on \u00b7 install gh for PR status \u00b7 1 shell \u00b7 \u2193 to manage", L"  \u23f5\u23f5 \u00b7 1 shell \u00b7 \u2193 to manage" },
                { L"  \u23f5\u23f5 auto mode on (shift+tab to cycle)", L"  \u23f5\u23f5" },
                { L"  \u23f5\u23f5 auto mode on (shift+tab to cycle) \u00b7 install gh for PR status", L"  \u23f5\u23f5" },
                { L"  \u23f5\u23f5 auto mode on \u00b7 install gh for PR st", L"  \u23f5\u23f5" },
                { L"  \u23f5\u23f5 auto mode on (shift+tab to cy", L"  \u23f5\u23f5" },
                { L"  \u23f5\u23f5 auto mo", L"  \u23f5\u23f5" },
                { L"  \u23f5\u23f5 auto mode on \u00b7 1 shell, 2 monitors            \u25ef IDE", L"  \u23f5\u23f5 \u00b7 1 shell, 2 monitors            \u25ef IDE" },
                { L"  ? for shortcuts \u00b7 gh auth login for PR status \u00b7 2 shells", L"  ? for shortcuts \u00b7 2 shells" },
                { L"  install gh for PR status \u00b7 2 shells", L"  2 shells" },
                { L"  \u23f8 plan mode on \u00b7 install gh for PR status", L"  \u23f8 plan mode on" },
            };
            for (const auto& [footer, expected] : footers)
            {
                VERIFY_IS_TRUE(CardText(footer) == expected, footer.data());
                // Rows above the footer stay, even with the same text.
                const auto above = std::wstring{ footer } + L"\n";
                VERIFY_IS_TRUE(CardText(above + std::wstring{ footer }) == above + std::wstring{ expected }, footer.data());
            }
        }

        TEST_METHOD(FooterClutterOnlyFromFooters)
        {
            for (const auto rows : { L"  \u23f5\u23f5 accept edits on (shift+tab to cycle)",
                                     L"  \u23f8 plan mode on \u00b7 1 shell \u00b7 \u2193 to manage",
                                     L"  \u23f5\u23f5 auto mode only",
                                     L"  install gh for PR status checks",
                                     L"I turned \u23f5\u23f5 auto mode on \u00b7 install gh for PR status",
                                     L"  \u23f5\u23f5 auto mode on\nPS C:\\>" })
            {
                VERIFY_IS_TRUE(CardText(rows) == rows, rows);
            }
        }

        TEST_METHOD(FooterLeftBlankIsDropped)
        {
            VERIFY_IS_TRUE(CardText(L"\u273b Cooked for 3s\n  install gh for PR status") == L"\u273b Cooked for 3s");
            VERIFY_IS_TRUE(CardText(L"  gh auth login for PR status").empty());
        }

        TEST_METHOD(FooterClutterKeepsStyles)
        {
            std::vector<StyledRun> runs{ { L"\u276f commit", 1 },
                                         { L"\n", 0 },
                                         { L"  ", 0 },
                                         { L"\u23f5\u23f5 auto mode on", 2 },
                                         { L" (shift+tab to cycle) \u00b7 install gh for PR status \u00b7 ", 3 },
                                         { L"1 shell", 4 },
                                         { L" \u00b7 \u2193 to manage", 3 } };
            std::wstring text;
            for (const auto& run : runs)
            {
                text.append(run.Text);
            }
            RemoveClaudeFooterClutter(runs, text);
            const std::vector<StyledRun> expected{ { L"\u276f commit", 1 },
                                                   { L"\n", 0 },
                                                   { L"  ", 0 },
                                                   { L"\u23f5\u23f5", 2 },
                                                   { L" \u00b7 ", 3 },
                                                   { L"1 shell", 4 },
                                                   { L" \u00b7 \u2193 to manage", 3 } };
            VERIFY_IS_TRUE(runs == expected, text.c_str());
            VERIFY_IS_TRUE(text == L"\u276f commit\n  \u23f5\u23f5 \u00b7 1 shell \u00b7 \u2193 to manage", text.c_str());
        }
    };
}
