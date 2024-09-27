//
// Created by PhilippSteininger on 27/09/2024.
//

#ifndef BACKGROUNDTRAFFICMANAGER_H
#define BACKGROUNDTRAFFICMANAGER_H

#include "Settings.h"

struct BackgroundTrafficConfig
{
  bool IsUDP;
  bool IsL4S;
  QD::BitRate Bandwidth;
};

struct BackgroundTrafficControl
{
  BackgroundTrafficConfig Config;
  uint16_t Port;
};

class BackgroundTrafficManager {
public:
  BackgroundTrafficManager(){}
  ~BackgroundTrafficManager()
  {
    for (const auto& element : m_backgroundTraffic)
    {
      RemoveTraffic(element.Port);
    }
  }

  void AddTraffic(BackgroundTrafficConfig config)
  {
    std::lock_guard guard(m_lock);

    uint16_t port = m_basePort + m_backgroundTrafficCounter++;

    auto exists = std::ranges::any_of(m_backgroundTraffic, [&](BackgroundTrafficControl x)
    {
      return x.Port == port;
    });

    if (!exists)
    {
      m_backgroundTraffic.push_back({
       .Config = config,
       .Port = port,
      });
      StartServer(port, config.IsL4S);
      StartClient(port, config.IsUDP, config.IsL4S, config.Bandwidth.bps());
    }
  }

  void RemoveTraffic(uint16_t port)
  {
    std::lock_guard guard(m_lock);

    auto it = std::ranges::find_if(m_backgroundTraffic, [&](BackgroundTrafficControl x)
    {
      return x.Port == port;
    });

    if (it != m_backgroundTraffic.end())
    {
      m_backgroundTraffic.erase(it);
      KillClient(port);
      KillServer(port);
    }
  }

  const std::vector<BackgroundTrafficControl>& GetActive() const
  {
    return m_backgroundTraffic;
  }

private:
  void StartServer(uint16_t port, bool useL4S)
  {
    auto cmd = "./start_iperf_server.sh " +
        Settings::TrafficSinkIp + " " +
        std::to_string(port) + " " +
        (useL4S ? "true " : "false ");

    system(cmd.c_str());
  }

  void KillServer(uint16_t port)
  {
    auto cmd = "./stop_iperf_server.sh " + Settings::TrafficSinkIp + " " + std::to_string(port);
    system(cmd.c_str());
  }

  void StartClient(uint16_t port, bool useUdp, bool useL4S, uint64_t bandwidth)
  {
    auto cmd = "./start_iperf_client.sh "
        + Settings::TrafficGeneratorIp + " "
        + Settings::TrafficSinkIp + " "
        + std::to_string(port) + " "
        + (useL4S ? "true " : "false ")
        + (useUdp ? "true " : "false ")
        + std::to_string(bandwidth);
    system(cmd.c_str());
  }

  void KillClient(uint16_t port)
  {
    auto cmd = "./stop_iperf_client.sh " + Settings::TrafficGeneratorIp + " " + std::to_string(port);
    system(cmd.c_str());
  }

private:
  const uint16_t m_basePort = 7777;
  std::mutex m_lock;
  uint16_t m_backgroundTrafficCounter = 0;
  std::vector<BackgroundTrafficControl> m_backgroundTraffic;
};

#endif //BACKGROUNDTRAFFICMANAGER_H
