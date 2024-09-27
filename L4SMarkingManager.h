//
// Created by PhilippSteininger on 27/09/2024.
//

#ifndef L4SMARKINGMANAGER_H
#define L4SMARKINGMANAGER_H

#include "Settings.h"

class L4SMarkingManager {
public:
    static inline void Enable() {
        std::string msg = "./update_mark.sh -s";

        auto x = Settings::ServerIPs;
        for (auto i = 0; i != x.size(); i++)
        {
            msg += " " + x[i];
        }

        system(msg.c_str());
    }
    static inline void Disable() {
        std::string msg = "./update_mark.sh -c";
        system(msg.c_str());
    }
};
#endif //L4SMARKINGMANAGER_H
