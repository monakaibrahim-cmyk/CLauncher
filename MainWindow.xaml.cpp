#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include "Core/Logging/Logs.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Windows::Foundation;

namespace winrt::CLauncher::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();

        // ── Custom Titlebar ──
        ExtendsContentIntoTitleBar(true);
        SetTitleBar(AppTitleBar());

        LOG_INFO("Initialized.");

        // Automatically start checking for updates on startup
        StartUpdateCheck();
    }

    MainWindow::~MainWindow()
    {
        if (m_downloader)
        {
            m_downloader->CANCEL();
        }

        if (m_workerThread.joinable())
        {
            m_workerThread.join();
        }
    }

    // ═══════════════════════════════════════════════════════════
    //                   STATE MANAGEMENT
    // ═══════════════════════════════════════════════════════════

    void MainWindow::SetLauncherState(LauncherState state)
    {
        m_state = state;

        switch (state)
        {
        case LauncherState::Ready:
            PlayPanel().Visibility(Visibility::Visible);
            ProgressPanel().Visibility(Visibility::Collapsed);
            StatusText().Text(L"Up to date");
            PlayButton().Content(box_value(L"PLAY"));
            PlayButton().IsEnabled(true);
            break;

        case LauncherState::CheckingUpdate:
            PlayPanel().Visibility(Visibility::Visible);
            ProgressPanel().Visibility(Visibility::Collapsed);
            StatusText().Text(L"Checking for updates...");
            PlayButton().IsEnabled(false);
            PlayButton().Content(box_value(L"PLAY"));
            break;

        case LauncherState::Downloading:
            PlayPanel().Visibility(Visibility::Collapsed);
            ProgressPanel().Visibility(Visibility::Visible);
            DownloadStatusText().Text(L"Downloading update...");
            DownloadProgressBar().IsIndeterminate(false);
            DownloadProgressBar().Value(0);
            DownloadPercentText().Text(L"0%");
            DownloadSpeedText().Text(L"0 MB/s");
            DownloadETAText().Text(L"ETA: --");
            DownloadBytesText().Text(L"0 MB / 0 MB");
            break;

        case LauncherState::Installing:
            PlayPanel().Visibility(Visibility::Collapsed);
            ProgressPanel().Visibility(Visibility::Visible);
            DownloadStatusText().Text(L"Installing update...");
            DownloadProgressBar().IsIndeterminate(true);
            DownloadPercentText().Text(L"");
            DownloadSpeedText().Text(L"");
            DownloadETAText().Text(L"");
            DownloadBytesText().Text(L"Please wait");
            CancelButton().Visibility(Visibility::Collapsed);
            break;

        case LauncherState::Playing:
            PlayPanel().Visibility(Visibility::Visible);
            ProgressPanel().Visibility(Visibility::Collapsed);
            StatusText().Text(L"Playing...");
            PlayButton().Content(box_value(L"PLAYING"));
            PlayButton().IsEnabled(false);
            break;
        }
    }

    // ═══════════════════════════════════════════════════════════
    //                  PROGRESS UI UPDATE
    // ═══════════════════════════════════════════════════════════

    void MainWindow::UpdateProgressUI(uint64_t bytesReceived, uint64_t totalBytes)
    {
        if (totalBytes == 0) return;

        // ── Percentage ──
        double percent = (static_cast<double>(bytesReceived) / static_cast<double>(totalBytes)) * 100.0;
        DownloadProgressBar().Value(percent);

        wchar_t buf[64];
        swprintf_s(buf, L"%.0f%%", percent);
        DownloadPercentText().Text(buf);

        // ── Bytes downloaded ──
        double mbReceived = static_cast<double>(bytesReceived) / (1024.0 * 1024.0);
        double mbTotal = static_cast<double>(totalBytes) / (1024.0 * 1024.0);
        swprintf_s(buf, L"%.1f MB / %.1f MB", mbReceived, mbTotal);
        DownloadBytesText().Text(buf);

        // ── Speed & ETA (recalculated every 500ms+) ──
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - m_lastSpeedTime).count();

        if (elapsed >= 0.5)
        {
            double bytesDelta = static_cast<double>(bytesReceived - m_lastBytes);
            double bytesPerSec = bytesDelta / elapsed;
            double mbPerSec = bytesPerSec / (1024.0 * 1024.0);

            swprintf_s(buf, L"%.1f MB/s", mbPerSec);
            DownloadSpeedText().Text(buf);

            // ETA
            if (bytesPerSec > 0)
            {
                double remainingBytes = static_cast<double>(totalBytes - bytesReceived);
                double etaSeconds = remainingBytes / bytesPerSec;

                if (etaSeconds < 60.0)
                {
                    swprintf_s(buf, L"ETA: %.0fs", etaSeconds);
                }
                else if (etaSeconds < 3600.0)
                {
                    swprintf_s(buf, L"ETA: %.1fm", etaSeconds / 60.0);
                }
                else
                {
                    swprintf_s(buf, L"ETA: %.1fh", etaSeconds / 3600.0);
                }
                DownloadETAText().Text(buf);
            }

            m_lastBytes = bytesReceived;
            m_lastSpeedTime = now;
        }
    }

    // ═══════════════════════════════════════════════════════════
    //                  SIMULATED DOWNLOAD
    // ═══════════════════════════════════════════════════════════

    void MainWindow::StartSimulatedDownload()
    {
        m_simTotalBytes = 365ULL * 1024ULL * 1024ULL; // 365 MB
        m_simBytesReceived = 0;
        m_lastBytes = 0;
        m_lastSpeedTime = std::chrono::steady_clock::now();
        m_downloadCancelled = false;

        SetLauncherState(LauncherState::Downloading);
        CancelButton().Visibility(Visibility::Visible);

        // Timer fires every 100ms for smooth real-time progress
        m_downloadTimer = DispatcherTimer();
        m_downloadTimer.Interval(std::chrono::milliseconds(100));
        m_downloadTimer.Tick({ this, &MainWindow::OnDownloadTimerTick });
        m_downloadTimer.Start();
    }

    void MainWindow::OnDownloadTimerTick(IInspectable const&, IInspectable const&)
    {
        if (m_downloadCancelled)
        {
            m_downloadTimer.Stop();
            SetLauncherState(LauncherState::Ready);
            return;
        }

        // Simulate ~50 MB/s download (5 MB per 100ms tick)
        uint64_t chunk = 5ULL * 1024ULL * 1024ULL;
        m_simBytesReceived += chunk;

        if (m_simBytesReceived >= m_simTotalBytes)
        {
            m_simBytesReceived = m_simTotalBytes;
            UpdateProgressUI(m_simBytesReceived, m_simTotalBytes);
            m_downloadTimer.Stop();

            // Brief "Installing" phase
            SetLauncherState(LauncherState::Installing);

            // Auto-complete install after 2 seconds
            DispatcherTimer installTimer;
            installTimer.Interval(std::chrono::seconds(2));
            installTimer.Tick([this, installTimer](auto&&, auto&&) mutable {
                installTimer.Stop();
                SetLauncherState(LauncherState::Ready);
            });
            installTimer.Start();
            return;
        }

        UpdateProgressUI(m_simBytesReceived, m_simTotalBytes);
    }

    // ═══════════════════════════════════════════════════════════
    //                    GAME LAUNCH
    // ═══════════════════════════════════════════════════════════

    void MainWindow::LaunchGame()
    {
        hstring gamePath = GamePathTextBox().Text();
        if (gamePath.empty())
        {
            // Show a helpful message if no path is set
            StatusText().Text(L"Set game path in ⚙ Settings");
            return;
        }

        STARTUPINFOW si{};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi{};

        if (CreateProcessW(
                gamePath.c_str(),   // Application path
                nullptr,            // Command line
                nullptr,            // Process security
                nullptr,            // Thread security
                FALSE,              // Inherit handles
                0,                  // Creation flags
                nullptr,            // Environment
                nullptr,            // Current directory
                &si, &pi))
        {
            m_gameProcess = pi.hProcess;
            CloseHandle(pi.hThread);

            SetLauncherState(LauncherState::Playing);

            // Monitor the game process — check every 2 seconds
            m_gameProcessTimer = DispatcherTimer();
            m_gameProcessTimer.Interval(std::chrono::seconds(2));
            m_gameProcessTimer.Tick({ this, &MainWindow::OnGameProcessTimerTick });
            m_gameProcessTimer.Start();
        }
        else
        {
            StatusText().Text(L"Failed to launch game");
        }
    }

    void MainWindow::OnGameProcessTimerTick(IInspectable const&, IInspectable const&)
    {
        if (m_gameProcess != nullptr)
        {
            DWORD exitCode = 0;
            if (GetExitCodeProcess(m_gameProcess, &exitCode) && exitCode != STILL_ACTIVE)
            {
                CloseHandle(m_gameProcess);
                m_gameProcess = nullptr;
                m_gameProcessTimer.Stop();
                SetLauncherState(LauncherState::Ready);
            }
        }
    }

    // ═══════════════════════════════════════════════════════════
    //                 CLIENT UPDATE WORKFLOW
    // ═══════════════════════════════════════════════════════════

    void MainWindow::StartUpdateCheck()
    {
        if (m_state == LauncherState::Downloading || m_state == LauncherState::CheckingUpdate)
        {
            return;
        }

        SetLauncherState(LauncherState::CheckingUpdate);

        std::filesystem::path targetDirectory = winrt::CLauncher::Core::Helper::GET_ROOT_DIRECTORY();
        hstring configuredPath = GamePathTextBox().Text();
        if (!configuredPath.empty())
        {
            std::filesystem::path p = configuredPath.c_str();
            if (p.has_parent_path())
            {
                targetDirectory = p.parent_path();
            }
        }

        if (m_workerThread.joinable())
        {
            m_workerThread.join();
        }

        m_workerThread = std::thread([this, targetDirectory]()
        {
            RunUpdateWorkflow(targetDirectory);
        });
    }

    void MainWindow::RunUpdateWorkflow(std::filesystem::path targetDirectory)
    {
        try
        {
            auto queue = DispatcherQueue();
            if (!m_downloader)
            {
                m_downloader = std::make_shared<winrt::CLauncher::Core::API::Downloader>(targetDirectory);
            }
            else
            {
                m_downloader->SET_TARGET_DIRECTORY(targetDirectory);
            }

            m_downloader->SET_STATUS_CALLBACK([this, queue](const std::string& message)
            {
                queue.TryEnqueue([this, message]()
                {
                    std::wstring wideMessage(message.begin(), message.end());
                    if (m_state == LauncherState::CheckingUpdate)
                    {
                        StatusText().Text(wideMessage);
                    }
                    else if (m_state == LauncherState::Downloading)
                    {
                        DownloadStatusText().Text(wideMessage);
                    }
                });
            });

            m_downloader->SET_PROGRESS_CALLBACK([this, queue](uint64_t bytesReceived, uint64_t totalBytes, double /*speedMBs*/, const std::string& currentFileName)
            {
                queue.TryEnqueue([this, bytesReceived, totalBytes, currentFileName]()
                {
                    UpdateProgressUI(bytesReceived, totalBytes);
                    if (!currentFileName.empty())
                    {
                        std::wstring wideName(currentFileName.begin(), currentFileName.end());
                        DownloadStatusText().Text(L"Downloading " + wideName);
                    }
                });
            });

            // Step 1: Fetch client manifest
            if (!m_downloader->FETCH_CLIENT_MANIFEST())
            {
                queue.TryEnqueue([this]()
                {
                    StatusText().Text(L"Failed to fetch update manifest");
                    SetLauncherState(LauncherState::Ready);
                });
                return;
            }

            if (m_downloader->IS_CANCELLED())
            {
                queue.TryEnqueue([this]()
                {
                    SetLauncherState(LauncherState::Ready);
                    StatusText().Text(L"Update cancelled");
                });
                return;
            }

            // Step 2: Check client files against manifest
            std::vector<CLIENT_MANIFEST_ENTRY> filesToDownload;
            uint64_t totalBytes = 0;

            if (!m_downloader->CHECK_CLIENT_FILES(filesToDownload, totalBytes))
            {
                queue.TryEnqueue([this]()
                {
                    StatusText().Text(L"File verification failed");
                    SetLauncherState(LauncherState::Ready);
                });
                return;
            }

            if (filesToDownload.empty())
            {
                queue.TryEnqueue([this]()
                {
                    SetLauncherState(LauncherState::Ready);
                    StatusText().Text(L"Up to date");
                });
                return;
            }

            // Step 3: Transition to Downloading
            m_lastBytes = 0;
            m_lastSpeedTime = std::chrono::steady_clock::now();

            queue.TryEnqueue([this, totalBytes]()
            {
                SetLauncherState(LauncherState::Downloading);
                CancelButton().Visibility(Visibility::Visible);
                UpdateProgressUI(0, totalBytes);
            });

            // Step 4: Download client files
            bool success = m_downloader->DOWNLOAD_CLIENT_FILES(filesToDownload, totalBytes);

            queue.TryEnqueue([this, success]()
            {
                if (m_downloader->IS_CANCELLED())
                {
                    SetLauncherState(LauncherState::Ready);
                    StatusText().Text(L"Download cancelled");
                }
                else if (success)
                {
                    SetLauncherState(LauncherState::Installing);
                    DownloadStatusText().Text(L"Client files verified");

                    DispatcherTimer timer;
                    timer.Interval(std::chrono::milliseconds(800));
                    timer.Tick([this, timer](auto&&, auto&&) mutable
                    {
                        timer.Stop();
                        SetLauncherState(LauncherState::Ready);
                        StatusText().Text(L"Up to date");
                    });
                    timer.Start();
                }
                else
                {
                    SetLauncherState(LauncherState::Ready);
                    StatusText().Text(L"Download failed");
                }
            });
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("RunUpdateWorkflow exception: {}", e.what());
            DispatcherQueue().TryEnqueue([this]()
            {
                SetLauncherState(LauncherState::Ready);
                StatusText().Text(L"Update error");
            });
        }
    }

    // ═══════════════════════════════════════════════════════════
    //                    EVENT HANDLERS
    // ═══════════════════════════════════════════════════════════

    void MainWindow::PlayButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        if (m_state == LauncherState::Ready)
        {
            LaunchGame();
        }
    }

    void MainWindow::CancelDownload_Click(IInspectable const&, RoutedEventArgs const&)
    {
        m_downloadCancelled = true;
        if (m_downloader)
        {
            m_downloader->CANCEL();
        }
    }

    void MainWindow::SettingsButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        // Flyout is attached in XAML — opens automatically
    }

    void MainWindow::TestDownloadButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        // Close the settings flyout
        SettingsButton().Flyout().Hide();

        if (m_state == LauncherState::Ready)
        {
            StartUpdateCheck();
        }
    }
}

