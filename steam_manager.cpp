//
// Created by hy on 2024/1/17.
//

#include "steam_manager.h"
#include "tc_common/log.h"
#include "tc_common/task_runtime.h"
#include "tc_common/http_client.h"
#include "tc_common/thread.h"
#include "tc_common/vdf_parser.hpp"
#include "tc_common/string_ext.h"
#include "tc_common/file_ext.h"

#include <map>
#include <ranges>
#include <Windows.h>
#include <boost/filesystem.hpp>
#include <filesystem>

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

namespace tc
{

    std::shared_ptr<SteamManager> SteamManager::Make() {
        return std::make_shared<SteamManager>();
    }

    SteamManager::SteamManager() = default;
    SteamManager::~SteamManager() = default;

    void SteamManager::QueryInstalledApps(HKEY hKey) {
        TCHAR achKey[MAX_KEY_LENGTH];   // buffer for subkey name
        DWORD cbName = 0;                   // size of name string
        TCHAR achClass[MAX_PATH] = TEXT("");  // buffer for class name
        DWORD cchClassName = MAX_PATH;  // size of class string
        DWORD cSubKeys = 0;               // number of subkeys
        DWORD cbMaxSubKey = 0;              // longest subkey size
        DWORD cchMaxClass = 0;              // longest class string
        DWORD cValues = 0;              // number of values for key
        DWORD cchMaxValue = 0;          // longest value name
        DWORD cbMaxValueData = 0;       // longest value data
        DWORD cbSecurityDescriptor = 0; // size of security descriptor
        FILETIME ftLastWriteTime;      // last write time

        DWORD i = 0, j = 0, retCode = 0;

        TCHAR achValue[MAX_VALUE_NAME] = {'\0'};
        DWORD cchValue = MAX_VALUE_NAME;

        // Get the class name and the value count.
        retCode = ::RegQueryInfoKey(
                hKey,                    // key handle
                achClass,                // buffer for class name
                &cchClassName,           // size of class string
                NULL,                    // reserved
                &cSubKeys,               // number of subkeys
                &cbMaxSubKey,            // longest subkey size
                &cchMaxClass,            // longest class string
                &cValues,                // number of values for this key
                &cchMaxValue,            // longest value name
                &cbMaxValueData,         // longest value data
                &cbSecurityDescriptor,   // security descriptor
                &ftLastWriteTime);       // last write time
        if (ERROR_SUCCESS != retCode) {
            LOGE("RegQueryInfoKey failed.");
            return;
        }

        // Enumerate the subkeys, until RegEnumKeyEx fails.
        if (cSubKeys) {
            printf("\nNumber of subkeys: %d\n", cSubKeys);

            for (i = 0; i < cSubKeys; i++) {
                cbName = MAX_KEY_LENGTH;
                retCode = ::RegEnumKeyEx(hKey, i,
                                         achKey,
                                         &cbName,
                                         NULL,
                                         NULL,
                                         NULL,
                                         &ftLastWriteTime);

                if (retCode == ERROR_SUCCESS) {

                    RegAppInfo reg_app_info;
                    reg_app_info.app_id_ = std::atoi(StringExt::ToUTF8(std::wstring(achKey)).c_str());
                    LOGI("Reg: id:{}", reg_app_info.app_id_);

                    std::wstring name;
                    auto innerSubKey = steam_app_base_path_ + L"\\" + std::wstring(achKey);
                    HKEY innerKey;
                    std::wcout << "app path: " << innerSubKey << std::endl;
                    if (RegOpenKeyExW(HKEY_CURRENT_USER, innerSubKey.c_str(), 0, KEY_READ, &innerKey) == ERROR_SUCCESS) {
                        {
                            wchar_t nameValue[MAX_PATH] = L"Name";
                            DWORD bufferSize = MAX_PATH * sizeof(wchar_t);
                            wchar_t buffer[MAX_PATH] = {0};
                            if (RegGetValueW(innerKey, nullptr, nameValue, RRF_RT_REG_SZ, nullptr, buffer,
                                             &bufferSize) == ERROR_SUCCESS) {
                                std::wcout << "Game name: " << buffer << std::endl;
                            }
                        }

                        {
                            wchar_t nameValue[MAX_PATH] = L"Installed";
                            DWORD data = 0;
                            DWORD dataSize = sizeof(DWORD);
                            if (RegGetValueW(innerKey, nullptr, nameValue, RRF_RT_REG_DWORD, nullptr, &data,
                                             &dataSize) == ERROR_SUCCESS) {
                                std::wcout << "Installed : " << data << std::endl;
                                reg_app_info.installed_ = data;
                            }
                        }

                        {
                            wchar_t nameValue[MAX_PATH] = L"Running";
                            DWORD data = 0;
                            DWORD dataSize = sizeof(DWORD);
                            if (RegGetValueW(innerKey, nullptr, nameValue, RRF_RT_REG_DWORD, nullptr, &data,
                                             &dataSize) == ERROR_SUCCESS) {
                                std::wcout << "Running : " << data << std::endl;
                                reg_app_info.running_ = data;
                            }
                        }

                        {
                            wchar_t nameValue[MAX_PATH] = L"Updating";
                            DWORD data = 0;
                            DWORD dataSize = sizeof(DWORD);
                            if (RegGetValueW(innerKey, nullptr, nameValue, RRF_RT_REG_DWORD, nullptr, &data,
                                             &dataSize) == ERROR_SUCCESS) {
                                std::wcout << "updating : " << data << std::endl;
                            }
                        }
                        RegCloseKey(innerKey);

                        if (reg_app_info.installed_) {
                            reg_apps_info_.push_back(reg_app_info);
                        }
                    }
                }
            }
        }

        // Enumerate the key values.

        if (cValues) {
            printf("\nNumber of values: %d\n", cValues);

            for (i = 0; i < cValues; i++) {
                cchValue = MAX_VALUE_NAME;
                achValue[0] = '\0';
                retCode = ::RegEnumValue(hKey, i,
                                         achValue,
                                         &cchValue,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL);

                if (retCode == ERROR_SUCCESS) {
                    printf(("(%d) %s\n"), i + 1, achValue);
                }
            }
        }
    }

    bool SteamManager::ScanInstalledGames() {
        installed_steam_path_ = ScanInstalledSteamPath();
        steam_app_base_path_ = L"SOFTWARE\\Valve\\Steam\\Apps";

#if 1
        HKEY hkey;
        if (::RegOpenKeyEx(HKEY_CURRENT_USER, steam_app_base_path_.c_str(), 0, KEY_READ, &hkey) ==
            ERROR_SUCCESS) {
            QueryInstalledApps(hkey);
        }
        ::RegCloseKey(hkey);
#endif
        ParseLibraryFolders();
        ParseConfigForEachGame();
        ScanHeaderImageInAppCache();

        LOGI("-----------------------------------------------Finally-----------------------------------------");
        LOGI("steam path: {}", installed_steam_path_);
        LOGI("all steam apps size: {}", games_.size());
        for (auto& game : games_) {
            LOGI("game: \n{}", game.Dump());
        }
        return true;
    }

    std::string SteamManager::ScanInstalledSteamPath() {
        HKEY hKey;
        LPCWSTR subKey = L"SOFTWARE\\Valve\\Steam";
        LPCWSTR valueName = L"SteamPath";
        DWORD bufferSize = MAX_PATH * sizeof(wchar_t);
        wchar_t buffer[MAX_PATH] = {0};

        if (RegOpenKeyExW(HKEY_CURRENT_USER, subKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            if (RegGetValueW(hKey, nullptr, valueName, RRF_RT_REG_SZ, nullptr, buffer, &bufferSize) == ERROR_SUCCESS) {
                RegCloseKey(hKey);
                return StringExt::ToUTF8(buffer);
            }
            RegCloseKey(hKey);
        }

        return "";
    }

    std::vector<SteamApp> SteamManager::GetInstalledGames() {
        return games_;
    }

    void SteamManager::DumpGamesInfo() {
        LOGI("Total games: {}", games_.size());
        for (auto& game : games_) {
            LOGI("Game: {}", game.Dump());
        }
    }

    void SteamManager::ParseLibraryFolders() {
        auto library_folders_path = installed_steam_path_ + "/steamapps/libraryfolders.vdf";
        std::ifstream library_folders(library_folders_path);

        if (!std::filesystem::exists(std::filesystem::u8path(library_folders_path))) {
            LOGE("Library folder not exist: {}", library_folders_path);
            return;
        }

        bool ok;
        tyti::vdf::basic_object<char> objs;
        try {
            objs = tyti::vdf::read(library_folders, &ok);
        } catch(std::exception& e) {
            LOGE("Parse failed: {}, error: {}", library_folders_path, e.what());
            return;
        }
        if (!ok) {
            LOGE("Parse library folders failed.");
            return;
        }

        for (auto& c : objs.childs) {
            LOGI("key: {}", c.first);
            auto s = c.second;
            // path
            if (s->attribs.find("path") == s->attribs.end()) {
                continue;
            }
            std::string path = s->attribs["path"];

            //
            StringExt::Replace(path, "\\", "/");

            InstalledFolder installed_folder;
            installed_folder.path_ = path;

            // apps
#if 0
            for (auto& child : s->childs) {
                if (child.first != "apps") {
                    continue;
                }
                for (auto& app : child.second->attribs) {
                    LOGI("{} => {}", app.first, app.second);
                    installed_folder.app_id_value_.push_back(RegAppInfo {
                            .app_id_ = std::atoi(app.first.c_str()),
                            .app_size_ = std::atoi(app.second.c_str()),
                    });
                }
            }
#endif
            // 读取这个地方不准，把注册表里的都拿出来，如果安装在不同磁盘，每个磁盘都把所有的app文件夹都检测一次
            // 最后只保留检测到的就可以
            installed_folder.app_id_value_.insert(installed_folder.app_id_value_.begin(), reg_apps_info_.begin(), reg_apps_info_.end());
            installed_folders_.push_back(installed_folder);
        }

//        std::ofstream config_back(path);
//        tyti::vdf::write(config_back, objs);
//        config_back.close();
    }

    void SteamManager::ParseConfigForEachGame() {
        for (const auto& folder : installed_folders_) {
            if (folder.path_.empty() || folder.app_id_value_.empty()) {
                continue;
            }

            auto steam_apps_path = folder.path_ + "/steamapps";
            for (const auto& app : folder.app_id_value_) {
                auto app_cfg = steam_apps_path + "/appmanifest_" + std::to_string(app.app_id_) + ".acf";
                auto u8_cfg_path = std::filesystem::u8path(app_cfg);
                if (!std::filesystem::exists(u8_cfg_path)) {
                    LOGE("Path not exist: {}", app_cfg);
                    continue;
                }

                std::ifstream app_cfg_path(app_cfg);
                bool ok;
                tyti::vdf::basic_object<char> objs;
                try {
                    objs = tyti::vdf::read(app_cfg_path, &ok);
                } catch(std::exception& e) {
                    LOGE("Read file failed: {}, error: {}", app_cfg, e.what());
                    continue;
                }
                if (!ok) {
                    LOGE("Parse library folders failed.");
                    return;
                }

                if (objs.attribs.find("installdir") == objs.attribs.end()) {
                    continue;
                }
                if (objs.attribs.find("name") == objs.attribs.end()) {
                    continue;
                }

                auto installed_dir_name = objs.attribs["installdir"];
                auto installed_dir = steam_apps_path;
                installed_dir.append("/common/").append(installed_dir_name);

                SteamApp steam_app;
                steam_app.app_id_ = app.app_id_;
                steam_app.name_ = objs.attribs["name"];
                steam_app.installed_dir_ = installed_dir;
                steam_app.steam_url_ = std::format("steam://rungameid/{}", app.app_id_);

                LOGI("will find: {}, name: {}", app.app_id_, steam_app.name_);
                if (!FindRunningExes(steam_app)) {
                    continue;
                }

                //LOGI("game: {}", steam_app.Dump());
                games_.push_back(steam_app);
            }
        }
        LOGI("---Parse completed....");
    }

    bool SteamManager::FindRunningExes(SteamApp& app) {
        auto app_path = std::filesystem::u8path(app.installed_dir_);
        if (!std::filesystem::exists(app_path)) {
            LOGE("folder not exist: {}", app.installed_dir_);
            return false;
        }

        bool find_exe = false;
        std::filesystem::directory_iterator end_iter;
        for (std::filesystem::directory_iterator iter(app_path); iter != end_iter; iter++) {
            if (std::filesystem::is_directory(*iter)) {
                continue;
            }

            auto path = iter->path().wstring();
            auto u8path = StringExt::ToUTF8(path);
            StringExt::Replace(u8path, "\\", "/");
            auto suffix = FileExt::GetFileSuffix(u8path);
            if (suffix.empty()) {
                continue;
            }
            bool is_exe = StringExt::ToLowerCpy(suffix) == "exe";
            bool ignore_by_policy = IgnoreByPolicy(u8path);
            if (is_exe && !ignore_by_policy) {
                app.exes_.push_back(u8path);
                std::cout << "find exe: " << u8path << std::endl;
                auto exe_name = FileExt::GetFileNameFromPath(u8path);
                app.exe_names_.push_back(exe_name);
                find_exe = true;
            }
        }
        return find_exe;
    }

    bool SteamManager::IgnoreByPolicy(const std::string& path) {
        auto name_no_suffix = StringExt::ToLowerCpy(FileExt::GetFileNameFromPathNoSuffix(path));
        auto filter_names = {"crashhandler", "ffmpeg", "ffprobe", "qtwebengine", "vc_redist",
             "uninstall", "crashpad"
        };
        auto find = std::ranges::any_of(filter_names, [&name_no_suffix](const std::string& name) {
            return boost::algorithm::contains(name_no_suffix, name);
        });
        return find;
    }

    void SteamManager::ScanHeaderImageInAppCache() {
#if 0
        QString library_cache_path = installed_steam_path_ + "/appcache/librarycache";
        QDir dir(library_cache_path);
        if (!dir.exists()) {
            LOGE("Cache not exist:{}", library_cache_path.toStdString());
            return;
        }

        dir.setFilter(QDir::Files);
        QDirIterator iterator(dir);
        std::vector<QString> cached_file_names;
        while (iterator.hasNext()) {
            auto file_name = iterator.fileName();
            iterator.next();
            if (file_name.isEmpty()) {
                continue;
            }
            cached_file_names.push_back(file_name);
        }

        for (auto& game : games_) {
            QString file_prefix = std::format("{}_header", game.app_id_).c_str();
            for (auto& file_name : cached_file_names) {
                if (file_name.startsWith(file_prefix)) {
                    game.cover_url_ = library_cache_path + "/" + file_name;
                    break;
                }
            }
        }
#endif
    }

    void SteamManager::UpdateAppDetails() {
#if 0
        context_->GetTaskRuntime()->Post(SimpleThreadTask::Make([=, this]() {
            for (auto& game : games_) {
                auto path = kApiAppDetails + "?appids=" + std::to_string(game.app_id_);
                auto client = HttpClient::MakeSSL(kApiBase, path);
                auto resp = client->Request({});
                //LOGI("resp : {} {}", resp.status, resp.body);
            }
        }));
#endif
    }

}