//
// Created by hy on 2024/1/17.
//

#ifndef TC_SERVER_STEAM_STEAM_MANAGER_H
#define TC_SERVER_STEAM_STEAM_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "steam_entities.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace tc
{

    // 安装的应用使用信息
    class RegAppInfo{
    public:
        int app_id_;
        bool installed_;
        bool running_;
        // 应该是app占用的空间大小
        int app_size_;
    };

    class InstalledFolder {
    public:
        std::string path_;
        std::vector<RegAppInfo> app_id_value_;
    };

    class SteamManager {
    public:

        static std::shared_ptr<SteamManager> Make();

        explicit SteamManager();
        ~SteamManager();

        bool ScanInstalledGames();
        std::vector<SteamApp> GetInstalledGames();
        void DumpGamesInfo();
        void UpdateAppDetails();

    private:
        std::string ScanInstalledSteamPath();
        void QueryInstalledApps(HKEY key);
        void ParseLibraryFolders();
        void ParseConfigForEachGame();
        void ScanHeaderImageInAppCache();
        bool FindRunningExes(SteamApp& app);
        bool IgnoreByPolicy(const std::string& path);

    private:
        std::string installed_steam_path_;
        std::wstring steam_app_base_path_;
        std::vector<RegAppInfo> reg_apps_info_;
        std::vector<SteamApp> games_;
        std::vector<InstalledFolder> installed_folders_;
    };

}


#endif