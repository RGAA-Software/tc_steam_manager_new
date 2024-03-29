//
// Created by hy on 2024/1/17.
//

#ifndef TC_SERVER_STEAM_STEAM_MANAGER_H
#define TC_SERVER_STEAM_STEAM_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <memory>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace tc
{
    class SteamApp;
    class InstalledFolder;
    class RegAppInfo;
    class TaskRuntime;

    class SteamManager {
    public:

        static std::shared_ptr<SteamManager> Make(const std::shared_ptr<TaskRuntime>& rt);

        explicit SteamManager(const std::shared_ptr<TaskRuntime>& rt);
        ~SteamManager();

        bool ScanInstalledGames();
        std::vector<std::shared_ptr<SteamApp>> GetInstalledGames();
        void DumpGamesInfo();
        void UpdateAppDetails();
        std::string GetSteamInstalledPath();
        std::string GetSteamImageCachePath();

    private:
        std::string ScanInstalledSteamPath();
        void QueryInstalledApps(HKEY key);
        void ParseLibraryFolders();
        void ParseConfigForEachGame();
        void ScanHeaderImageInAppCache();
        bool FindRunningExes(const std::shared_ptr<SteamApp>& app);
        bool IgnoreByPolicy(const std::string& path);
        std::string EstimateEngine(const std::shared_ptr<SteamApp>& app);
        bool ExeFilter(const std::string& lowcase_exe_name);

    private:
        std::string installed_steam_path_;
        std::wstring steam_app_base_path_;
        std::vector<std::shared_ptr<RegAppInfo>> reg_apps_info_;
        std::vector<std::shared_ptr<SteamApp>> games_;
        std::vector<std::shared_ptr<InstalledFolder>> installed_folders_;
        std::shared_ptr<TaskRuntime> task_runtime_ = nullptr;
    };

}


#endif