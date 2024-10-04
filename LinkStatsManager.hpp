//
// Created by PhilippSteininger on 04/10/2024.
//

#ifndef LINKSTATSMANAGER_H
#define LINKSTATSMANAGER_H

#include <chrono>
#include <stdexcept>
#include <iostream>
#include <cstdio>

static std::string s_exampleInput = R"(qdisc htb 1: dev bng-e1 root refcnt 17 r2q 10 default 0 direct_packets_stat 515 ver 3.17 direct_qlen 1000
 Sent 226872157 bytes 1041515 pkt (dropped 345, overlimits 73281 requeues 0)
 backlog 0b 0p requeues 0
qdisc dualpi2 11: dev bng-e1 parent 1:1 limit 500p target 15ms tupdate 16ms alpha 0.156250 beta 3.195312 l4s_ect coupling_factor 2 drop_on_overload step_thresh 1ms drop_dequeue split_gso classic_protection 10%
 Sent 226842179 bytes 1041000 pkt (dropped 345, overlimits 0 requeues 0)
 backlog 0b 0p requeues 0
prob 0.000000 delay_c 0us delay_l 0us
pkts_in_c 95006 pkts_in_l 946339 maxq 131
ecn_mark 79867 step_marks 48147
credit -7170 (L)

qdisc ingress ffff: dev bng-e1 parent ffff:fff1 ----------------
 Sent 2349102995 bytes 2012073 pkt (dropped 0, overlimits 0 requeues 0)
 backlog 0b 0p requeues 0
qdisc htb 1: dev bng-e3 root refcnt 17 r2q 10 default 0 direct_packets_stat 477 ver 3.17 direct_qlen 1000
 Sent 2320786283 bytes 1970473 pkt (dropped 41203, overlimits 1635798 requeues 0)
 backlog 3028b 2p requeues 0
qdisc dualpi2 11: dev bng-e3 parent 1:1 limit 500p target 15ms tupdate 16ms alpha 0.156250 beta 3.195312 l4s_ect coupling_factor 2 drop_on_overload step_thresh 1ms drop_dequeue split_gso classic_protection 10%
 Sent 2320761517 bytes 1969996 pkt (dropped 41203, overlimits 201 requeues 0)
 backlog 3028b 2p requeues 0
prob 0.002827 delay_c 0us delay_l 4138us
pkts_in_c 429291 pkts_in_l 1581709 maxq 499
ecn_mark 607562 step_marks 534064
credit -87290 (L)

qdisc ingress ffff: dev bng-e3 parent ffff:fff1 ----------------
 Sent 212850097 bytes 1042181 pkt (dropped 0, overlimits 0 requeues 0)
 backlog 0b 0p requeues 0)";

;

struct LinkStats {
    uint64_t packetCount;
    uint64_t droppedPacketCount;
    std::chrono::microseconds l4sQueueLatency;
    std::chrono::microseconds claQueueLatency;
    float markingProbability;
};

class LinkStatsManager {
public:
    bool Fetch(LinkStats& stats) {
        static std::string result;
        RunCommand(m_systemCommand, result);
        if (result.empty())
            return false;

        ParseElementData(result, stats);
        return true;
    }
private:
    bool ParseElementData(const std::string& cmdOutput, LinkStats& stats)
    {
        auto elems = Split(cmdOutput, "\n\n");
        if (elems.size() < 2)
            return false;

        std::string line;
        if (TryFindGroupLineThatContains(elems[1], "qdisc dualpi2 11:", "prob", line))
        {
            auto words = Split(line, " ");

            auto x = SubstringBack(words[3], 2);
            auto x2 = SubstringBack(words[5], 2);

            stats.markingProbability = std::stof(words[1]);
            stats.claQueueLatency = std::chrono::microseconds(std::stoi(SubstringBack(words[3], 2)));
            stats.l4sQueueLatency = std::chrono::microseconds(std::stoi(SubstringBack(words[5], 2)));
        }


        if (TryFindGroupLineThatContains(elems[1], "qdisc dualpi2 11:", " Sent", line))
        {
            auto words = Split(line, " ");

            auto x2 = SubstringBack(words[7], 1);
            stats.droppedPacketCount = std::stoul(x2);
            stats.packetCount = std::stoul(words[2]);
        }
        return true;
    }

    bool TryFindGroupLineThatContains(const std::string& input, const std::string&& groupLine, const std::string&& searchString, std::string& out) {
        std::istringstream stream(input);
        bool foundGroupLine = false;

        while (std::getline(stream, out)) {
            // Check for the start of the desired qdisc
            if (out.find(groupLine) != std::string::npos) {
                foundGroupLine = true;  // Mark that we found the desired qdisc
            }

            // Look for the line starting with "prob" only if we are in the relevant qdisc section
            if (foundGroupLine && out.find(searchString) == 0) {
                return true;  // Return the first "prob" line found under the specific qdisc
            }

            // Reset when reaching another qdisc definition
            if (foundGroupLine && out.find(searchString) == 0 && out.find(groupLine) == std::string::npos) {
                foundGroupLine = false;  // Exit the qdisc section if a new one starts
            }
        }

        return false;  // Return empty string if no match found
    }

    bool FindLineThatContains(const std::string& input, const std::string& searchString, std::string& out) {
        std::istringstream stream(input);
        while (std::getline(stream, out)) {
            // Check for the start of the desired qdisc
            if (out.find(searchString) != std::string::npos) {
                return true;
            }
        }

        return false;
    }

    void RunCommand(const char* cmd, std::string& data) {
        std::array<char, 128> outputBuffer{0};

        data.clear();
#ifdef _WIN32
        std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
#else
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
#endif

        if (!pipe) {
            throw std::runtime_error("popen() failed!");
        }

        while (fgets(outputBuffer.data(), outputBuffer.size(), pipe.get()) != nullptr) {
            data += outputBuffer.data();
        }
    }

    std::vector<std::string> Split(const std::string& s, const std::string& delimiter) {
        std::vector<std::string> tokens;

        size_t prevFoundIdx = 0;
        size_t idx = 0;
        std::string token;
        while ((idx = s.find(delimiter, prevFoundIdx)) != std::string::npos) {
            token = s.substr(prevFoundIdx, idx - prevFoundIdx);
            tokens.push_back(token);
            prevFoundIdx = idx + delimiter.length();
        }
        tokens.push_back(s.substr(prevFoundIdx, s.size() - prevFoundIdx));

        return tokens;
    }

    std::string SubstringBack(const std::string& str, size_t count)
    {
        return str.substr(0, str.size() - count);
    }

private:
    const char* m_systemCommand = "sudo ip netns exec bng sudo tc -s -d qdisc show";
};

#endif //LINKSTATSMANAGER_H
