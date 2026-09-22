#pragma once

#include "MainWindow.g.h"
#include <chrono>

namespace winrt::CLauncher::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        // ── XAML Event Handlers ──
        void PlayButton_Click(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);

        void CancelDownload_Click(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);

        void SettingsButton_Click(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);

        void TestDownloadButton_Click(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);

    private:
        // ── Launcher State Machine ──
        enum class LauncherState
        {
            Ready,
            CheckingUpdate,
            Downloading,
            Installing,
            Playing
        };

        void SetLauncherState(LauncherState state);
        void UpdateProgressUI(uint64_t bytesReceived, uint64_t totalBytes);
        void LaunchGame();
        void StartSimulatedDownload();
        void OnGameProcessTimerTick(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Windows::Foundation::IInspectable const& args);
        void OnDownloadTimerTick(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Windows::Foundation::IInspectable const& args);

        // ── State ──
        LauncherState m_state{ LauncherState::Ready };

        // ── Download progress tracking ──
        uint64_t m_lastBytes{ 0 };
        uint64_t m_simBytesReceived{ 0 };
        uint64_t m_simTotalBytes{ 0 };
        std::chrono::steady_clock::time_point m_lastSpeedTime{};
        winrt::Microsoft::UI::Xaml::DispatcherTimer m_downloadTimer{ nullptr };
        bool m_downloadCancelled{ false };

        // ── Game process tracking ──
        winrt::Microsoft::UI::Xaml::DispatcherTimer m_gameProcessTimer{ nullptr };
        HANDLE m_gameProcess{ nullptr };
    };
}

namespace winrt::CLauncher::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}

