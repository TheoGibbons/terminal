// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "precomp.h"
#include "../TerminalApp/SessionFinishNotifier.h"

using namespace WEX::TestExecution;
using ::TerminalApp::NormalizeCallbackUrl;
using ::TerminalApp::SessionFinishNotifier;
using Action = SessionFinishNotifier::Action;

namespace TerminalAppUnitTests
{
    class SessionFinishNotifierTests
    {
        BEGIN_TEST_CLASS(SessionFinishNotifierTests)
        END_TEST_CLASS()

        TEST_METHOD(NotifiesAfterActivityStaysStopped)
        {
            SessionFinishNotifier notifier{ 30'000, false };
            VERIFY_IS_TRUE(notifier.Update(0, true) == Action::None);
            VERIFY_IS_TRUE(notifier.Update(60'000, true) == Action::None);
            VERIFY_IS_TRUE(notifier.Update(61'000, false) == Action::None);
            VERIFY_IS_TRUE(notifier.Update(90'999, false) == Action::None);
            VERIFY_IS_TRUE(notifier.Update(91'000, false) == Action::Notify);
            VERIFY_IS_TRUE(notifier.Done());
            VERIFY_IS_TRUE(notifier.Update(200'000, false) == Action::None);
        }

        TEST_METHOD(CallbackFollowsNotificationAfterSameDelay)
        {
            SessionFinishNotifier notifier{ 30'000, true };
            VERIFY_IS_TRUE(notifier.Update(0, false) == Action::None);
            VERIFY_IS_TRUE(notifier.Update(30'000, false) == Action::Notify);
            VERIFY_IS_FALSE(notifier.Done());
            VERIFY_IS_TRUE(notifier.Update(59'999, false) == Action::None);
            VERIFY_IS_TRUE(notifier.Update(60'000, false) == Action::Callback);
            VERIFY_IS_TRUE(notifier.Done());
        }

        TEST_METHOD(ResumedActivityRestartsDelay)
        {
            SessionFinishNotifier notifier{ 30'000, true };
            VERIFY_IS_TRUE(notifier.Update(0, false) == Action::None);
            VERIFY_IS_TRUE(notifier.Update(20'000, true) == Action::None);
            VERIFY_IS_TRUE(notifier.Update(25'000, false) == Action::None);
            VERIFY_IS_TRUE(notifier.Update(54'999, false) == Action::None);
            VERIFY_IS_TRUE(notifier.Update(55'000, false) == Action::Notify);

            // Activity after the notification drops its callback, and the
            // next finish starts the whole sequence again.
            VERIFY_IS_TRUE(notifier.Update(70'000, true) == Action::None);
            VERIFY_IS_TRUE(notifier.Update(100'000, false) == Action::None);
            VERIFY_IS_TRUE(notifier.Update(129'999, false) == Action::None);
            VERIFY_IS_TRUE(notifier.Update(130'000, false) == Action::Notify);
            VERIFY_IS_TRUE(notifier.Update(160'000, false) == Action::Callback);
        }

        TEST_METHOD(UserActivityAfterFinishCancels)
        {
            SessionFinishNotifier beforeNotify{ 30'000, true };
            beforeNotify.UserActivity();
            VERIFY_IS_FALSE(beforeNotify.Done());
            VERIFY_IS_TRUE(beforeNotify.Update(0, false) == Action::None);
            VERIFY_IS_TRUE(beforeNotify.Update(10'000, false) == Action::None);
            beforeNotify.UserActivity();
            VERIFY_IS_TRUE(beforeNotify.Done());
            VERIFY_IS_TRUE(beforeNotify.Update(30'000, false) == Action::None);

            SessionFinishNotifier beforeCallback{ 30'000, true };
            VERIFY_IS_TRUE(beforeCallback.Update(0, false) == Action::None);
            VERIFY_IS_TRUE(beforeCallback.Update(30'000, false) == Action::Notify);
            beforeCallback.UserActivity();
            VERIFY_IS_TRUE(beforeCallback.Done());
            VERIFY_IS_TRUE(beforeCallback.Update(60'000, false) == Action::None);
        }

        TEST_METHOD(ZeroDelay)
        {
            SessionFinishNotifier notifier{ 0, true };
            VERIFY_IS_TRUE(notifier.Update(0, true) == Action::None);
            VERIFY_IS_TRUE(notifier.Update(500, false) == Action::Notify);
            VERIFY_IS_TRUE(notifier.Update(1'000, false) == Action::Callback);
            VERIFY_IS_TRUE(notifier.Done());
        }

        TEST_METHOD(CallbackUrls)
        {
            VERIFY_ARE_EQUAL(std::wstring{}, NormalizeCallbackUrl(L"").value());
            VERIFY_ARE_EQUAL(std::wstring{}, NormalizeCallbackUrl(L" \t ").value());
            VERIFY_ARE_EQUAL(std::wstring{ L"https://ntfy.sh/topic" }, NormalizeCallbackUrl(L" https://ntfy.sh/topic ").value());
            VERIFY_ARE_EQUAL(std::wstring{ L"https://ntfy.sh/topic" }, NormalizeCallbackUrl(L"ntfy.sh/topic").value());
            VERIFY_ARE_EQUAL(std::wstring{ L"HTTP://localhost:8080/hook?x=1" }, NormalizeCallbackUrl(L"HTTP://localhost:8080/hook?x=1").value());
            for (const auto text : { L"ftp://example.com", L"file:///C:/x", L"https://", L"https:///path", L"https://exa mple.com", L"/relative" })
            {
                VERIFY_IS_FALSE(NormalizeCallbackUrl(text).has_value(), text);
            }
        }
    };
}
