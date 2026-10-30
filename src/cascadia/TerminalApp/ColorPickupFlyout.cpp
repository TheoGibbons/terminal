#include "pch.h"
#include "ColorPickupFlyout.h"
#include "ColorPickupFlyout.g.cpp"

namespace winrt::TerminalApp::implementation
{
    // Method Description:
    // - Default constructor, localizes the buttons and hooks
    // up the event fired by the custom color picker, so that
    // the tab color is set on the fly when selecting a non-preset color
    // Arguments:
    // - <none>
    ColorPickupFlyout::ColorPickupFlyout() :
        ColorPickupFlyout(false)
    {
    }

    ColorPickupFlyout::ColorPickupFlyout(bool mutedPalette)
    {
        InitializeComponent();

        OkButton().Content(winrt::box_value(RS_(L"Ok")));
        CustomColorButton().Content(winrt::box_value(RS_(L"TabColorCustomButton/Content")));
        ClearColorButton().Content(winrt::box_value(RS_(L"TabColorClearButton/Content")));
        if (mutedPalette)
        {
            const std::array<std::pair<hstring, Windows::UI::Color>, 8> presets{
                std::pair{ RS_(L"SessionPreviewCharcoal"), Windows::UI::Color{ 255, 40, 42, 46 } },
                std::pair{ RS_(L"SessionPreviewSlate"), Windows::UI::Color{ 255, 45, 52, 62 } },
                std::pair{ RS_(L"SessionPreviewBlue"), Windows::UI::Color{ 255, 39, 52, 69 } },
                std::pair{ RS_(L"SessionPreviewTeal"), Windows::UI::Color{ 255, 35, 58, 59 } },
                std::pair{ RS_(L"SessionPreviewGreen"), Windows::UI::Color{ 255, 43, 58, 45 } },
                std::pair{ RS_(L"SessionPreviewPlum"), Windows::UI::Color{ 255, 54, 43, 62 } },
                std::pair{ RS_(L"SessionPreviewRose"), Windows::UI::Color{ 255, 64, 44, 50 } },
                std::pair{ RS_(L"SessionPreviewBrown"), Windows::UI::Color{ 255, 61, 51, 42 } },
            };
            const auto buttons = PresetColors().Children();
            buttons.Clear();
            for (const auto& [name, color] : presets)
            {
                Windows::UI::Xaml::Controls::Button button;
                button.Background(Windows::UI::Xaml::Media::SolidColorBrush{ color });
                Windows::UI::Xaml::Automation::AutomationProperties::SetName(button, name);
                Windows::UI::Xaml::Controls::ToolTipService::SetToolTip(button, winrt::box_value(name));
                button.Click({ this, &ColorPickupFlyout::ColorButton_Click });
                buttons.Append(button);
            }
        }
    }

    // Method Description:
    // - Handler of the click event for the preset color swatches.
    // Reads the color from the clicked rectangle and fires an event
    // with the selected color. After that hides the flyout
    // Arguments:
    // - sender: the rectangle that got clicked
    // Return Value:
    // - <none>
    void ColorPickupFlyout::ColorButton_Click(const IInspectable& sender, const Windows::UI::Xaml::RoutedEventArgs&)
    {
        auto button{ sender.as<Windows::UI::Xaml::Controls::Button>() };
        auto rectClr{ button.Background().as<Windows::UI::Xaml::Media::SolidColorBrush>() };
        ColorSelected.raise(rectClr.Color());
        Hide();
    }

    // Method Description:
    // - Handler of the clear color button. Clears the current
    // color of the tab, if any. Hides the flyout after that
    // Arguments:
    // - <none>
    // Return Value:
    // - <none>
    void ColorPickupFlyout::ClearColorButton_Click(const IInspectable&, const Windows::UI::Xaml::RoutedEventArgs&)
    {
        ColorCleared.raise();
        Hide();
    }

    // Method Description:
    // - Handler of the select custom color button. Expands or collapses the flyout
    // to show the color picker. In order to accomplish this a FlyoutPresenterStyle is used,
    // in which a Style is embedded, containing the desired width
    // Arguments:
    // - <none>
    // Return Value:
    // - <none>
    void ColorPickupFlyout::ShowColorPickerButton_Click(const Windows::Foundation::IInspectable&, const Windows::UI::Xaml::RoutedEventArgs&)
    {
        auto visibility = customColorPanel().Visibility();
        if (visibility == winrt::Windows::UI::Xaml::Visibility::Collapsed)
        {
            customColorPanel().Visibility(winrt::Windows::UI::Xaml::Visibility::Visible);
        }
        else
        {
            customColorPanel().Visibility(winrt::Windows::UI::Xaml::Visibility::Collapsed);
        }
    }

    // Method Description:
    // - Handles the color selection of the color pickup. Gets
    // the currently selected color and fires an event with it
    // Arguments:
    // - <none>
    // Return Value:
    // - <none>
    void ColorPickupFlyout::CustomColorButton_Click(const Windows::Foundation::IInspectable&, const Windows::UI::Xaml::RoutedEventArgs&)
    {
        auto color = customColorPicker().Color();
        ColorSelected.raise(color);
        Hide();
    }

    void ColorPickupFlyout::ColorPicker_ColorChanged(const Microsoft::UI::Xaml::Controls::ColorPicker&, const Microsoft::UI::Xaml::Controls::ColorChangedEventArgs& args)
    {
        ColorSelected.raise(args.NewColor());
    }
}
