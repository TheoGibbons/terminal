// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
//
// Module Name:
// - PaneNote.h
//
// Abstract:
// - A note the user can write on a terminal pane, to remember what it's for. A
//   small button in the pane's top right corner shows or hides it. It floats over
//   the terminal, so showing it doesn't resize the terminal or reflow its text.

#pragma once

#include "PaneNote.g.h"

namespace winrt::TerminalApp::implementation
{
    struct PaneNote : PaneNoteT<PaneNote>
    {
        PaneNote();

        winrt::hstring Text();
        void Text(const winrt::hstring& value);
        bool IsOpen();
        void IsOpen(bool value);
        void SetMaxNoteHeight(double height);

        void _Loaded(const winrt::Windows::Foundation::IInspectable& sender, const winrt::Windows::UI::Xaml::RoutedEventArgs& args);
        void _Tapped(const winrt::Windows::Foundation::IInspectable& sender, const winrt::Windows::UI::Xaml::Input::TappedRoutedEventArgs& args);
        void _ToggleClicked(const winrt::Windows::Foundation::IInspectable& sender, const winrt::Windows::UI::Xaml::RoutedEventArgs& args);
        void _TogglePointerEntered(const winrt::Windows::Foundation::IInspectable& sender, const winrt::Windows::UI::Xaml::Input::PointerRoutedEventArgs& args);
        void _TogglePointerExited(const winrt::Windows::Foundation::IInspectable& sender, const winrt::Windows::UI::Xaml::Input::PointerRoutedEventArgs& args);
        void _NoteBoxKeyDown(const winrt::Windows::Foundation::IInspectable& sender, const winrt::Windows::UI::Xaml::Input::KeyRoutedEventArgs& args);
        void _NoteBoxTextChanged(const winrt::Windows::Foundation::IInspectable& sender, const winrt::Windows::UI::Xaml::Controls::TextChangedEventArgs& args);

        til::typed_event<TerminalApp::PaneNote, winrt::Windows::Foundation::IInspectable> ReturnFocusRequested;

    private:
        void _UpdateToggle();

        bool _pointerOverToggle{ false };
    };
}

namespace winrt::TerminalApp::factory_implementation
{
    BASIC_FACTORY(PaneNote);
}
