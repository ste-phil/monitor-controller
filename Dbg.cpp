//
// Created by PhilippSteininger on 27/09/2024.
//

#include "Dbg.h"

void Dbg::Log(std::string msg) {
    m_lastMsg = msg;
}

const std::string& Dbg::GetLastMessage() {
    return m_lastMsg;
}
