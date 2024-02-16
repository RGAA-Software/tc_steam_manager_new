//
// Created by RGAA on 2024-02-06.
//

#ifndef TC_APPLICATION_STEAM_ENTITIES_H
#define TC_APPLICATION_STEAM_ENTITIES_H

#include <sstream>

namespace tc
{

    class SteamApp {
    public:
        // steam app的id
        int app_id_{};
        // app name
        std::string name_;
        // installed dir
        std::string installed_dir_;
        // 目录下所有的exe，ue的在内部文件夹中
        std::vector<std::string> exes_;
        std::vector<std::string> exe_names_;
        // 查注册表能知道是否已经运行了
        bool is_running_{};
        // 查注册表能知道是否已经安装了
        bool is_installed_{};
        // 拼接的steam 快捷方式
        std::string steam_url_{};

    public:

        [[nodiscard]] std::string Dump() const {
            std::stringstream ss;
            ss << "name: " << name_ << std::endl;
            ss << "app id: " << app_id_ << std::endl;
            ss << "installed dir: " << installed_dir_ << std::endl;
            ss << "exe size: " << exes_.size() << std::endl;
            for (auto& exe : exes_) {
                ss << "  - " << exe << std::endl;
            }
            return ss.str();
        }
    };

}

#endif //TC_APPLICATION_STEAM_ENTITIES_H
