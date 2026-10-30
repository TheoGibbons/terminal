// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace TerminalApp
{
    // Tracks one drawer "Notify me when this finishes" request. Activity must
    // stay stopped for the delay before the user is notified, then for the
    // same delay again before the callback is due. Times are milliseconds
    // from a monotonic clock.
    class SessionFinishNotifier
    {
    public:
        enum class Action
        {
            None,
            Notify,
            Callback,
        };

        SessionFinishNotifier(const uint64_t delay, const bool hasCallback) noexcept :
            _delay{ delay },
            _hasCallback{ hasCallback }
        {
        }

        bool Done() const noexcept
        {
            return _phase == Phase::Done;
        }

        Action Update(const uint64_t now, const bool working) noexcept
        {
            if (_phase == Phase::Done)
            {
                return Action::None;
            }
            if (working)
            {
                // The task hadn't finished. Wait for it to stop again, and
                // drop a callback the notification already announced.
                _phase = Phase::Waiting;
                return Action::None;
            }
            if (_phase == Phase::Waiting)
            {
                _phase = Phase::Finished;
                _since = now;
            }
            if (now - _since < _delay)
            {
                return Action::None;
            }
            if (_phase == Phase::Finished && _hasCallback)
            {
                _phase = Phase::Notified;
                _since = now;
                return Action::Notify;
            }
            const auto action = _phase == Phase::Finished ? Action::Notify : Action::Callback;
            _phase = Phase::Done;
            return action;
        }

        // Input after activity stops means the user has seen the tab, so the
        // request ends silently. Input while the task runs is ignored.
        void UserActivity() noexcept
        {
            if (_phase == Phase::Finished || _phase == Phase::Notified)
            {
                _phase = Phase::Done;
            }
        }

    private:
        enum class Phase
        {
            Waiting,
            Finished,
            Notified,
            Done,
        };

        uint64_t _delay;
        uint64_t _since{ 0 };
        Phase _phase{ Phase::Waiting };
        bool _hasCallback;
    };

    // Returns the URL to call, an empty string when no callback is wanted, or
    // nullopt when the text is not an http(s) URL. Text without a scheme, such
    // as "ntfy.sh/topic", is treated as https.
    inline std::optional<std::wstring> NormalizeCallbackUrl(std::wstring_view text)
    {
        constexpr std::wstring_view whitespace{ L" \t\r\n" };
        const auto first = text.find_first_not_of(whitespace);
        if (first == std::wstring_view::npos)
        {
            return std::wstring{};
        }
        text = text.substr(first, text.find_last_not_of(whitespace) - first + 1);

        std::wstring url{ text };
        if (url.find(L"://") == std::wstring::npos)
        {
            url.insert(0, L"https://");
        }
        const auto schemeEnd = url.find(L"://");
        auto scheme = url.substr(0, schemeEnd);
        for (auto& ch : scheme)
        {
            if (ch >= L'A' && ch <= L'Z')
            {
                ch += L'a' - L'A';
            }
        }
        const auto hostStart = schemeEnd + 3;
        if ((scheme != L"http" && scheme != L"https") ||
            hostStart == url.size() || url.find_first_of(L"/?#", hostStart) == hostStart)
        {
            return std::nullopt;
        }
        for (const auto ch : url)
        {
            if (ch <= L' ' || ch == L'\x7f')
            {
                return std::nullopt;
            }
        }
        return url;
    }
}
