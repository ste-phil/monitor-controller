#include <iostream>
#include "QuickDebug/LatencyMonitor.hpp"
#include "QuickDebug/QuickDebug.hpp"

#include <cstdlib>

enum class Mode
{
    Idle,
    Shaping,
    Reset
};

using namespace std::chrono_literals;

bool g_isRunning;
Mode g_shapingMode;

const uint32_t g_unlimitedBandwidth_mbps = 100;
uint32_t g_minBandwidth_kbps = 5'000;
uint32_t g_maxBandwidth_kbps = 20'000;
uint32_t g_stepSizeBandwidth_kbps = 100;
std::chrono::milliseconds g_maxShapingTime = 15000ms;
std::chrono::milliseconds g_stepInterval = 50ms;

uint32_t g_currentBandwidth_bps;

uint32_t PingPong(uint32_t t, uint32_t minBandwidth, uint32_t maxBandwidth, uint32_t stepSize)
{
    auto range = maxBandwidth - minBandwidth;
    auto cycleLength = 2 * range; // One full ping-pong cycle (up and down)

    // Calculate the ping-pong value
    auto position = t * stepSize % cycleLength;

    // Use the absolute value to simulate the "bounce" effect
    int32_t diff = static_cast<int32_t>(range) - static_cast<int32_t>(position);
    auto pingPongValue = minBandwidth + range - std::abs(diff);

    return pingPongValue;
}

int LimitThroughput(uint32_t bandwidth_mbps)
{
    g_currentBandwidth_bps = bandwidth_mbps * 1000 * 1000;
    std::string command = "./update_tput.sh DP2" + std::to_string(bandwidth_mbps) + " 0ms";
    auto result = system(command.c_str());
    return result;
}

void ShapingThread()
{
    while (g_isRunning)
    {
        switch (g_shapingMode)
        {
            case Mode::Shaping:
            {
                auto elapsedTime = std::chrono::milliseconds(0);
                uint32_t t = 0;
                while (elapsedTime < g_maxShapingTime && g_shapingMode != Mode::Reset)
                {
                    auto bandwidth_bps = PingPong(
                        t++,
                        g_minBandwidth_kbps * 1000,
                        g_maxBandwidth_kbps * 1000,
                        g_stepSizeBandwidth_kbps * 1000
                    );
                    std::cout << "Bandwidth: " << bandwidth_bps / 1000 << " kbps" << std::endl;
                    LimitThroughput(bandwidth_bps);
                    QD::QuickDebug::Plot("Middlebox Bandwidth (kbps)", g_currentBandwidth_bps / 1000);

                    std::this_thread::sleep_for(g_stepInterval);
                    elapsedTime += g_stepInterval;
                }
                g_shapingMode = Mode::Reset;
                break;
            }
            case Mode::Reset:
                std::cout << "Resetting..." << std::endl;
                LimitThroughput(g_unlimitedBandwidth_mbps);
                std::cout << "Bandwidth: " << g_currentBandwidth_bps / 1000 << " kbps" << std::endl;
                g_shapingMode = Mode::Idle;
                break;
            case Mode::Idle:
                break;
        }
    }
}

void ReadValue(const std::string& label, std::chrono::milliseconds& value)
{
    std::cout << value <<  "(Current Value: " << value.count() << ")" << std::endl;

    std::string in;
    std::getline(std::cin, in);
    std::istringstream stream(in);

    int temp;
    if (stream >> temp)
    {
       value = std::chrono::milliseconds(temp);
    }
}

template<class T>
void ReadValue(const std::string& label, T& value)
{
    std::cout << label <<  "(Current Value: " << value << ")" << std::endl;

    std::string in;
    std::getline(std::cin, in);
    std::istringstream stream(in);

    T temp;
    if (stream >> temp)
    {
        value = temp;
    }
}

int main()
{
    QD::QuickDebug::Startup();


    std::thread shaperThread;
    shaperThread = std::thread(ShapingThread);
    shaperThread.detach();

    g_isRunning = true;
    while (g_isRunning)
    {
        std::cout << "Range: [" << g_minBandwidth_kbps << ", " << g_maxBandwidth_kbps << "] kbps, Stepsize: "
        << g_stepSizeBandwidth_kbps << " kbps, Runtime: " << g_maxShapingTime << std::endl;

        std::cout << "1: Configure Parameters" << std::endl;
        std::cout << "2: Start Shaping" << std::endl;
        std::cout << "3: Stop Shaping" << std::endl;
        std::cout << "Select option: ";
        std::string input;
        std::getline(std::cin, input);

        if (input == "1")
        {
            g_shapingMode = Mode::Reset;
            ReadValue("Min Bandwidth: ", g_minBandwidth_kbps);
            ReadValue("Max Bandwidth: ", g_maxBandwidth_kbps);
            ReadValue("Stepsize Bandwidth: ", g_stepSizeBandwidth_kbps);
            ReadValue("Shaping Runtime (ms): ", g_maxShapingTime);
        }
        else if (input == "2")
        {
            g_shapingMode = Mode::Shaping;
        }
        else if (input == "3")
        {
            g_shapingMode = Mode::Reset;
        }
    }
}


