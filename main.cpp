#include <iostream>
#include "QuickDebug/LatencyMonitor.hpp"
#include "QuickDebug/QuickDebug.hpp"

#include <cstdlib>

#include <ftxui/dom/elements.hpp>  // for text, hbox, vbox, Element
#include <ftxui/component/component.hpp>  // for Button, Renderer
#include <ftxui/component/screen_interactive.hpp>  // for ScreenInteractive
#include <string>
#include "QuickDebug/Common.hpp"
#include "BackgroundTrafficManager.hpp"
#include "L4SMarkingManager.h"
#include "Dbg.h"
#include "Settings.h"

enum class Mode
{
    Idle,
    Shaping,
    Reset
};

using namespace std::chrono_literals;

BackgroundTrafficManager g_trafficManager;
bool g_isRunning = true;
Mode g_shapingMode;

QD::BitRate g_defaultBandwidth = QD::BitRate::Mbps(100);
uint32_t g_minBandwidth_kbps = 5'000;
uint32_t g_maxBandwidth_kbps = 20'000;
uint32_t g_stepSizeBandwidth_kbps = 100;
std::chrono::milliseconds g_maxShapingTime = 15000ms;
std::chrono::milliseconds g_stepInterval = 50ms;
std::chrono::milliseconds g_shapingTimeRemaining = 0ms;

uint32_t g_currentBandwidth_bps;
ftxui::ScreenInteractive g_screen = ftxui::ScreenInteractive::Fullscreen();
std::shared_ptr<std::string> btnStartStopText = std::make_shared<std::string>("Start");

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

int LimitThroughput(uint32_t bandwidth_bps)
{
    g_currentBandwidth_bps = bandwidth_bps;
    std::string command = "./update_tput.sh DP2 " + std::to_string(g_currentBandwidth_bps) + " 0";
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
                g_shapingTimeRemaining = std::chrono::milliseconds(g_maxShapingTime);
                uint32_t t = 0;
                while (g_shapingTimeRemaining > 0ms && g_shapingMode != Mode::Reset)
                {
                    auto bandwidth_bps = PingPong(
                        t++,
                        g_minBandwidth_kbps * 1000,
                        g_maxBandwidth_kbps * 1000,
                        g_stepSizeBandwidth_kbps * 1000
                    );
                    LimitThroughput(bandwidth_bps);
                    // std::cout << "Bandwidth: " << g_currentBandwidth_bps / 1000 << " kbps" << std::endl;
                    QD::QuickDebug::Plot("Middlebox Bandwidth (kbps)", g_currentBandwidth_bps / 1000);

                    std::this_thread::sleep_for(g_stepInterval);
                    g_shapingTimeRemaining -= g_stepInterval;
                }
                g_shapingMode = Mode::Reset;
                break;
            }
        case Mode::Reset:
                // std::cout << "Resetting..." << std::endl;
                LimitThroughput(g_defaultBandwidth.bps());
                g_shapingTimeRemaining = 0ms;
                // std::cout << "Bandwidth: " << g_currentBandwidth_bps / 1000 << " kbps" << std::endl;
                g_shapingMode = Mode::Idle;
                break;
            case Mode::Idle:
                break;
        }
    }
}

void ReadValue(const std::string& label, std::chrono::milliseconds& value)
{
    std::istringstream stream(label);

    int temp;
    if (stream >> temp)
    {
       value = std::chrono::milliseconds(temp);
    }
}

uint32_t ReadValue(std::string& in)
{
    std::istringstream stream(in);
    uint32_t temp;
    if (stream >> temp)
    {
        return temp;
    }
    return 0;
}



std::tuple<ftxui::Component, ftxui::Component> BuildBandwidthShaper()
{
    using namespace ftxui;
    auto input0 = std::make_shared<std::string>(std::to_string(g_defaultBandwidth.Kbps()));
    auto input1 = std::make_shared<std::string>(std::to_string(g_maxBandwidth_kbps));
    auto input2 = std::make_shared<std::string>(std::to_string(g_minBandwidth_kbps));
    auto input3 = std::make_shared<std::string>(std::to_string(g_stepInterval.count()));
    auto input4 = std::make_shared<std::string>(std::to_string(g_maxShapingTime.count()));

    auto field0 = Input(input0.get());
    auto field1 = Input(input1.get());
    auto field2 = Input(input2.get());
    auto field3 = Input(input3.get());
    auto field4 = Input(input4.get());

    auto buttonSave = Button("Save", [=] {
        g_defaultBandwidth = QD::BitRate::Kbps(ReadValue(*input0));
        g_maxBandwidth_kbps = ReadValue(*input1);
        g_minBandwidth_kbps = ReadValue(*input2);
        g_stepSizeBandwidth_kbps = ReadValue(*input3);
        g_maxShapingTime = std::chrono::milliseconds(ReadValue(*input4));

        g_shapingMode = Mode::Reset;
    });

    auto buttonShapingStartStop = Button(btnStartStopText.get(), [=] {
        if (g_shapingMode == Mode::Idle)
        {
            *btnStartStopText = "Stop";
            g_shapingMode = Mode::Shaping;
        }
        else if (g_shapingMode == Mode::Shaping)
        {
            *btnStartStopText = "Start";
            g_shapingMode = Mode::Reset;
        }
    });

    auto l4sEnabled = std::make_shared<bool>(false);
    auto checkboxL4S = Checkbox("Set L4S Marks", l4sEnabled.get(), {
        .on_change = [=]
        {
            if (*l4sEnabled)
            {
                L4SMarkingManager::Enable();
            }
            else
            {
                L4SMarkingManager::Disable();
            }
        }
    });

    auto container = Container::Vertical({
        field0,
        field1,
        field2,
        field3,
        field4,
        buttonSave,
        buttonShapingStartStop,
        checkboxL4S
    });

    auto renderer = Renderer([=] {
        std::ostringstream out;
        out << std::fixed << std::setprecision(1) << g_shapingTimeRemaining.count() / 1000.0f;
        auto remainingTime = out.str();
        auto l4sEnabled_local = l4sEnabled; // HACK: This keeps l4sEnabled from being deallocated

        auto labelBandwidth = std::make_shared<std::string>("Current Bandwidth: " + std::to_string(g_currentBandwidth_bps / 1000) + " kbps");
        auto labelRemainingTime = std::make_shared<std::string>("Remaining Time: " + remainingTime + " s");
        return vbox({
            text("Bandwidth Shaper"),
            separator(),
            vbox({
                hbox(text("Default Bandwidth: "), field0->Render()) | flex,
                hbox(text("Min Bandwidth: "), field1->Render()) | flex,
                hbox(text("Max Bandwidth: "), field2->Render()) | flex,
                hbox(text("Step Bandwidth: "), field3->Render()) | flex,
                hbox(text("Time (ms): "), field4->Render()) | flex,
                separator(),
                buttonSave->Render(),
                buttonShapingStartStop->Render(),
            }),
            separator(),
            vbox({
               text(*labelBandwidth),
               text(*labelRemainingTime),
               text("") | flex,
            }) | flex ,
            separator(),
            checkboxL4S->Render()
        });
    });

    return std::make_tuple(container, renderer);
}

std::tuple<ftxui::Component, ftxui::Component> BuildBackgroundTraffic() {
    using namespace ftxui;

    auto container = Container::Vertical({

    });

    auto renderer = Renderer(container, [=]() {
        Elements elements;
        container->DetachAllChildren();
        for (int i = 0; i < g_trafficManager.GetActive().size(); i++)
        {
            const BackgroundTrafficControl& trafficControl = g_trafficManager.GetActive()[i];

            auto btn = Button("X", [=]
            {
                g_trafficManager.RemoveTraffic(trafficControl.Port);
            });

            container->Add(btn);
            elements.push_back(hbox({
                vcenter(text(trafficControl.Config.IsUDP ? "UDP" : "TCP")),
                vcenter(text(", ")),
                vcenter(text(trafficControl.Config.IsL4S ? "L4S" : "CLA")),
                vcenter(text(" @ ")),
                vcenter(text(trafficControl.Config.Bandwidth.PrettyPrint(QD::BitRateFormat::Mbps))),
                vcenter(text(" on ")),
                vcenter(text(std::to_string(trafficControl.Port))),
                filler() | flex,
                btn->Render() | flex_shrink
            }));
        }
        return vbox(std::move(elements)); // Return a vertical box containing the vector elements
    });

    return std::make_tuple(container, renderer);
}

auto selected1 = std::make_shared<int>(0);
auto radiobox1_list = std::make_shared<std::vector<std::string>>(std::initializer_list<std::string>({
    "UDP",
    "TCP"
}));

auto selected2 = std::make_shared<int>(0);
auto radiobox2_list = std::make_shared<std::vector<std::string>>(std::initializer_list<std::string>({
    "L4S",
    "CLA"
}));
auto trafficBandwidth = std::make_shared<std::string>("10");
std::tuple<ftxui::Component, ftxui::Component> BuildTrafficControl()
{
    return [=]()
    {
        using namespace ftxui;

        auto checkbox1 = Radiobox(radiobox1_list.get(), selected1.get());
        auto checkbox2 = Radiobox(radiobox2_list.get(), selected2.get());
        auto inputBandwidth = Input(trafficBandwidth.get());
        auto addTrafficBtn = Button("Add Traffic", [=] {
            g_trafficManager.AddTraffic({
                .IsUDP = *selected1 == 0,
                .IsL4S = *selected2 == 0,
                .Bandwidth = QD::BitRate::Mbps(ReadValue(*trafficBandwidth)),
           });
        });

        auto [bgContainer, bgRenderer] = BuildBackgroundTraffic();
        auto container = Container::Vertical({
            addTrafficBtn,
            Container::Horizontal({
                checkbox1,
                checkbox2,
            }),
            inputBandwidth,
            bgContainer
        });


        auto renderer = Renderer(container, [=]()
        {
            return vbox( {
                text("Background traffic"),
                separator(),
                hbox({
                    checkbox1->Render() | flex,
                    separator(),
                    checkbox2->Render() | flex
                }),
                hbox({
                    text("Bandwidth (mbps): "),
                    inputBandwidth->Render(),
                }),
                separator(),
                addTrafficBtn->Render(),
                separator(),
                text("Active traffic"),
                bgRenderer->Render()
            });
        });

        return std::make_tuple(container, renderer);
    }();
}

int main()
{
    QD::QuickDebug::Startup();


    std::thread shaperThread;
    shaperThread = std::thread(ShapingThread);
    shaperThread.detach();

    using namespace ftxui;

    auto [bandwidthShaperSettingsContainer, bandwidthShaperSettingsRenderer] = BuildBandwidthShaper();
    auto [trafficControlContainer, trafficControlRenderer] = BuildTrafficControl();

    auto mainContainer = Container::Horizontal({
        bandwidthShaperSettingsContainer,
        trafficControlContainer,
    });

    auto mainRenderer = Renderer(mainContainer, [&] {
        return hbox({
            bandwidthShaperSettingsRenderer->Render() | flex,  // Center input fields and button
            separator(),
            trafficControlRenderer->Render() | flex,  // Right button
        });
    });

    std::thread update_thread([]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            if (g_shapingMode == Mode::Idle)
            {
                *btnStartStopText = "Start";
            }
            else if (g_shapingMode == Mode::Shaping)
            {
                *btnStartStopText = "Stop";
            }
            g_screen.PostEvent(Event::Custom);
        }
    });
    update_thread.detach();

    g_screen.Loop(mainRenderer);
}