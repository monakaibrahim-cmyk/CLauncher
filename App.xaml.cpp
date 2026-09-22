#include "pch.h"
#include "App.xaml.h"
#include "MainWindow.xaml.h"
#include "Core/Logging/Logs.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::CLauncher::implementation
{
    /// <summary>
    /// Initializes the singleton application object.  This is the first line of authored code
    /// executed, and as such is the logical equivalent of main() or WinMain().
    /// </summary>
    App::App()
    {
        // Xaml objects should not call InitializeComponent during construction.
        // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent

#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
        UnhandledException([](IInspectable const&, UnhandledExceptionEventArgs const& e)
        {
            if (IsDebuggerPresent())
            {
                auto errorMessage = e.Message();
                __debugbreak();
            }
        });
#endif
    }

    /// <summary>
    /// Invoked when the application is launched.
    /// </summary>
    /// <param name="e">Details about the launch request and process.</param>
    void App::OnLaunched([[maybe_unused]] LaunchActivatedEventArgs const& e)
    {
		Core::Logging::Logs::AllocateConsole();
        window = make<MainWindow>();

        // ── Set window size ──
        if (auto appWindow = window.AppWindow())
        {
            // Initial size: 1280 x 720
            appWindow.Resize(winrt::Windows::Graphics::SizeInt32{ 1280, 720 });

            // Center the window on screen (optional, nice UX)
            auto displayArea = Microsoft::UI::Windowing::DisplayArea::GetFromWindowId(
                appWindow.Id(), Microsoft::UI::Windowing::DisplayAreaFallback::Primary);
            if (displayArea)
            {
                auto workArea = displayArea.WorkArea();
                int x = (workArea.Width - 1280) / 2 + workArea.X;
                int y = (workArea.Height - 720) / 2 + workArea.Y;
                appWindow.Move(winrt::Windows::Graphics::PointInt32{ x, y });
            }
        }

        window.Activate();
    }
}

