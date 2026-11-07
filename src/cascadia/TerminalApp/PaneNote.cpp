// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include "PaneNote.h"
#include "PaneNote.g.cpp"

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Controls;
using namespace winrt::Windows::UI::Xaml::Input;

namespace winrt::TerminalApp::implementation
{
    PaneNote::PaneNote()
    {
        InitializeComponent();
    }

    winrt::hstring PaneNote::Text()
    {
        return NoteBox().Text();
    }

    void PaneNote::Text(const winrt::hstring& value)
    {
        NoteBox().Text(value);
        _UpdateToggle();
    }

    bool PaneNote::IsOpen()
    {
        return NotePanel().Visibility() == Visibility::Visible;
    }

    void PaneNote::IsOpen(bool value)
    {
        NotePanel().Visibility(value ? Visibility::Visible : Visibility::Collapsed);
        _UpdateToggle();
    }

    void PaneNote::SetMaxNoteHeight(double height)
    {
        NoteBox().MaxHeight(height);
    }

    void PaneNote::_UpdateToggle()
    {
        const auto open = IsOpen();
        const auto note = NoteBox().Text();
        const auto hiddenNote = !open && !note.empty();

        ToggleIcon().Glyph(open ? L"\xE738" /* Remove */ : L"\xE710" /* Add */);

        const auto state = hiddenNote                 ? L"HiddenNote" :
                           open || _pointerOverToggle ? L"Prominent" :
                                                        L"Quiet";
        VisualStateManager::GoToState(*this, state, false);

        const auto toggle = ToggleButton();
        const auto action = open ? RS_(L"PaneNoteHide") : hiddenNote ? RS_(L"PaneNoteShow") : RS_(L"PaneNoteAdd");
        Automation::AutomationProperties::SetName(toggle, action);
        // A hidden note's tooltip is the note, so it can be read without opening it.
        ToolTipService::SetToolTip(toggle, winrt::box_value(hiddenNote ? note : action));
    }

    void PaneNote::_Loaded(const IInspectable&, const RoutedEventArgs&)
    {
        // A restored note was given its state before there was anything to apply it to.
        _UpdateToggle();
    }

    void PaneNote::_Tapped(const IInspectable&, const TappedRoutedEventArgs& args)
    {
        // Pane moves focus to the terminal whenever anything inside it is tapped.
        // Let that through, and the note could never be typed in.
        args.Handled(true);
    }

    void PaneNote::_ToggleClicked(const IInspectable&, const RoutedEventArgs&)
    {
        if (IsOpen())
        {
            IsOpen(false);
            ReturnFocusRequested.raise(*this, nullptr);
        }
        else
        {
            IsOpen(true);
            NoteBox().Focus(FocusState::Programmatic);
            // Pick up where the text ends, rather than wherever the caret was left.
            NoteBox().Select(gsl::narrow_cast<int32_t>(NoteBox().Text().size()), 0);
        }
    }

    void PaneNote::_TogglePointerEntered(const IInspectable&, const PointerRoutedEventArgs&)
    {
        _pointerOverToggle = true;
        _UpdateToggle();
    }

    void PaneNote::_TogglePointerExited(const IInspectable&, const PointerRoutedEventArgs&)
    {
        _pointerOverToggle = false;
        _UpdateToggle();
    }

    void PaneNote::_NoteBoxKeyDown(const IInspectable&, const KeyRoutedEventArgs& args)
    {
        const auto key = args.OriginalKey();
        if (key == winrt::Windows::System::VirtualKey::Escape)
        {
            IsOpen(false);
            ReturnFocusRequested.raise(*this, nullptr);
            args.Handled(true);
        }
        // Without this, Tab would wander off to the tab row or the drawer. The
        // control check leaves Ctrl+Tab for the page's key bindings.
        else if (key == winrt::Windows::System::VirtualKey::Tab && GetKeyState(VK_CONTROL) >= 0)
        {
            ReturnFocusRequested.raise(*this, nullptr);
            args.Handled(true);
        }
    }

    void PaneNote::_NoteBoxTextChanged(const IInspectable&, const TextChangedEventArgs&)
    {
        _UpdateToggle();
    }
}
