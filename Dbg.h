//
// Created by PhilippSteininger on 27/09/2024.
//

#ifndef DBG_H
#define DBG_H

#include <string>

class Dbg {
public:
    static void Log(std::string msg);
    static const std::string& GetLastMessage();

private:
  static inline std::string m_lastMsg;
};



#endif //DBG_H
