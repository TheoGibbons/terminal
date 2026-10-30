// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include <algorithm>
#include <regex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace TerminalApp
{
    enum class SessionActivity
    {
        Idle,
        // Waiting for input while background tasks run. A task wakes the
        // session when it reports, so this still counts as activity.
        Background,
        Working,
    };

    // Reads Claude Code's state from the preview text, whose last row is its
    // footer. The footer is a single clipped row, so a narrow pane can cut off
    // its task item ("1 shell, 1 monitor") and its "esc to interrupt" hint.
    // The status row above the prompt wraps instead: "✶ Cooking… (12s)" while
    // working, or "✻ Cooked for 4s · done 5:58 PM · 1 shell, 1 monitor still
    // running" after a turn. Only the latest status row counts; earlier turns
    // stay in view after their tasks end. Summaries follow Claude Code 2.1.302.
    inline SessionActivity ClaudeSessionActivity(const std::wstring_view snapshot)
    {
        // A count and noun, such as "1 shell", "2 local agents", or
        // "2 background" when space is short. A list may start with a glyph,
        // as in "◇ 1 cloud session". Uncounted chores, such as "dreaming",
        // are not activity.
        static const std::wstring item{ LR"(\d+ (?:[A-Za-z-]+ ){0,2}(?:(?:shell|monitor|agent|session|workflow|task|team)s?|background))" };
        static const std::wstring tasks{ LR"((?:[^\w\s]+ )?)" + item + L"(?:, " + item + L")*" };
        static const std::wregex footerTasks{ LR"((?:^\s*|\s\s|· ))" + tasks + LR"((?=\s·|\s\s|$))" };
        static const std::wregex runningTurn{ LR"(· )" + tasks + LR"( still running$|^✻ Waiting for .+ to finish$)" };
        static const std::wregex workingRow{ LR"(^. [^…]{1,80}…(?: \(|$))" };
        // The working glyph cycles through these. ✻ also starts a turn's summary.
        static constexpr std::wstring_view statusGlyphs{ L"·✢*✶✻✽" };

        std::vector<std::wstring_view> rows;
        for (size_t start = 0;;)
        {
            const auto end = snapshot.find(L'\n', start);
            rows.push_back(snapshot.substr(start, end - start));
            if (end == std::wstring_view::npos)
            {
                break;
            }
            start = end + 1;
        }

        // Claude indents its footer and wrapped rows by two spaces. A
        // different bottom row, such as a shell prompt after Claude exits,
        // means the rows above it are history.
        const auto indented = [](const std::wstring_view row) {
            return row.size() > 2 && row.starts_with(L"  ") && row[2] != L' ' && row[2] != L'⎿';
        };
        const auto footer = rows.back();
        if (!indented(footer) && !footer.starts_with(L"❯ "))
        {
            return SessionActivity::Idle;
        }

        for (auto i = rows.size() - 1; i-- > 0;)
        {
            const auto row = rows[i];
            if (row.size() < 3 || row[1] != L' ' || statusGlyphs.find(row[0]) == std::wstring_view::npos)
            {
                continue;
            }
            if (std::regex_search(row.begin(), row.end(), workingRow))
            {
                return SessionActivity::Working;
            }
            // Rejoin the rows a narrow pane wrapped the summary onto.
            std::wstring turn{ row };
            for (auto next = i + 1;; ++next)
            {
                if (std::regex_search(turn, runningTurn))
                {
                    return SessionActivity::Background;
                }
                if (next + 1 >= rows.size() || !indented(rows[next]))
                {
                    break;
                }
                turn.append(L" ").append(rows[next].substr(2));
            }
            break;
        }
        return std::regex_search(footer.begin(), footer.end(), footerTasks) ? SessionActivity::Background : SessionActivity::Idle;
    }

    // The ranges of Claude Code's footer row that drawer cards leave out: the
    // "auto mode on" label with its "(shift+tab to cycle)" hint, and the gh
    // nag for PR status. They lead the row, so a card would clip the task
    // item after them. The mode glyph stays, and a removed nag takes one "·"
    // separator with it. A narrow pane clips the row itself, so an item cut
    // off at the row's end matches the start of its text.
    inline std::vector<std::pair<size_t, size_t>> ClaudeFooterClutter(const std::wstring_view footer)
    {
        static constexpr std::wstring_view separator{ L" · " };
        static constexpr std::wstring_view autoModeGlyph{ L"⏵⏵" };
        static constexpr std::wstring_view autoMode{ L" auto mode on" };
        static constexpr std::wstring_view nags[]{ L"install gh for PR status", L"gh auth login for PR status" };

        std::vector<std::pair<size_t, size_t>> cuts;
        if (footer.size() <= 2 || !footer.starts_with(L"  ") || footer[2] == L' ')
        {
            return cuts;
        }
        const auto cut = [&](const size_t begin, const size_t end) {
            if (!cuts.empty() && cuts.back().second >= begin)
            {
                cuts.back().second = std::max(cuts.back().second, end);
            }
            else
            {
                cuts.emplace_back(begin, end);
            }
        };
        for (size_t start = 2; start < footer.size();)
        {
            // An item ends at a separator, at the gap before right-aligned
            // items, or at the row's end.
            const auto end = std::min({ footer.find(separator, start), footer.find(L"  ", start), footer.size() });
            const auto item = footer.substr(start, end - start);
            const auto clipped = end == footer.size();
            const auto label = item.starts_with(autoModeGlyph) ? item.substr(autoModeGlyph.size()) : std::wstring_view{};
            const auto autoModeLabel = label.starts_with(autoMode) ? label.size() == autoMode.size() || label.substr(autoMode.size()).starts_with(L" (") :
                                                                     clipped && !label.empty() && autoMode.starts_with(label);
            const auto nag = std::any_of(std::begin(nags), std::end(nags), [&](const std::wstring_view text) {
                return item == text || (clipped && !item.empty() && text.starts_with(item));
            });
            if (autoModeLabel)
            {
                cut(start + autoModeGlyph.size(), end);
            }
            else if (nag && start > 2 && footer.substr(start - separator.size(), separator.size()) == separator)
            {
                cut(start - separator.size(), end);
            }
            else if (nag)
            {
                cut(start, footer.substr(end).starts_with(separator) ? end + separator.size() : end);
            }
            start = footer.substr(end).starts_with(separator) ? end + separator.size() : footer.find_first_not_of(L' ', end);
        }
        return cuts;
    }

    // Leaves ClaudeFooterClutter out of a card's last row. The remaining text
    // keeps its runs' styles, and a footer left blank goes with its line
    // break. `text` is the runs' text, joined.
    template<typename Run>
    void RemoveClaudeFooterClutter(std::vector<Run>& runs, std::wstring& text)
    {
        const auto footerStart = text.rfind(L'\n') + 1;
        const auto cuts = ClaudeFooterClutter(std::wstring_view{ text }.substr(footerStart));
        if (cuts.empty())
        {
            return;
        }
        std::vector<bool> removed(text.size() - footerStart);
        for (const auto& [begin, end] : cuts)
        {
            for (auto i = begin; i < end; ++i)
            {
                removed[i] = true;
            }
        }

        std::vector<Run> kept;
        kept.reserve(runs.size());
        size_t offset = 0;
        for (auto& run : runs)
        {
            const std::wstring_view runText{ run.Text };
            std::wstring keptText;
            for (const auto ch : runText)
            {
                if (offset < footerStart || !removed[offset - footerStart])
                {
                    keptText.push_back(ch);
                }
                ++offset;
            }
            if (keptText.size() != runText.size())
            {
                run.Text = decltype(run.Text){ std::wstring_view{ keptText } };
            }
            if (!keptText.empty())
            {
                kept.push_back(std::move(run));
            }
        }
        runs = std::move(kept);

        text.clear();
        for (const auto& run : runs)
        {
            text.append(std::wstring_view{ run.Text });
        }
        if (std::wstring_view{ text }.substr(footerStart).find_first_not_of(L' ') == std::wstring_view::npos)
        {
            while (!runs.empty())
            {
                const auto lineBreak = std::wstring_view{ runs.back().Text } == L"\n";
                runs.pop_back();
                if (lineBreak)
                {
                    break;
                }
            }
            text.resize(footerStart > 0 ? footerStart - 1 : 0);
        }
    }
}
