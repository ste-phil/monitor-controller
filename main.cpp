#include <iostream>
#include "QuickDebug/LatencyMonitor.hpp"
#include "QuickDebug/QuickDebug.hpp"

#include <cstdlib>

#include <ftxui/dom/elements.hpp>  // for text, hbox, vbox, Element
#include <ftxui/component/component.hpp>  // for Button, Renderer
#include <ftxui/component/screen_interactive.hpp>  // for ScreenInteractive
#include <string>

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

class NullBuffer : public std::streambuf {
public:
    int overflow(int c) override {
        return c;  // Discards the character
    }
};

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
                    LimitThroughput(bandwidth_bps);
                    std::cout << "Bandwidth: " << g_currentBandwidth_bps / 1000 << " kbps" << std::endl;
                    QD::QuickDebug::Plot("Middlebox Bandwidth (kbps)", g_currentBandwidth_bps / 1000);

                    std::this_thread::sleep_for(g_stepInterval);
                    elapsedTime += g_stepInterval;
                }
                g_shapingMode = Mode::Reset;
                break;
            }
            case Mode::Reset:
                std::cout << "Resetting..." << std::endl;
                LimitThroughput(g_unlimitedBandwidth_mbps * 1000 * 1000);
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
    std::cout << label <<  "(Current Value: " << value.count() << ")" << std::endl;

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
void ReadValue(std::string& in, T& value)
{
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

    using namespace ftxui;

    // Create strings to store the input data
    std::string input1, input2, input3, input4;

    // Create input components for the four fields
    auto input_field1 = Input(&input1, std::to_string(g_maxBandwidth_kbps));
    auto input_field2 = Input(&input2, std::to_string(g_minBandwidth_kbps));
    auto input_field3 = Input(&input3, std::to_string(g_stepInterval.count()));
    auto input_field4 = Input(&input4, std::to_string(g_maxShapingTime.count()));

    // Button label and action for the center button
    std::string center_save_button = "Save";
    auto center_button = Button(&center_save_button, [&] {
        ReadValue(input1, g_maxBandwidth_kbps);
        ReadValue(input2, g_minBandwidth_kbps);
        ReadValue(input3, g_stepSizeBandwidth_kbps);
        ReadValue(input4, g_currentBandwidth_bps);
    });

    // Group the input fields and center button into a container
    auto input_container = Container::Vertical({
        input_field1,
        input_field2,
        input_field3,
        input_field4,
        center_button,  // Add the center button
    });

    // Render the input fields and center button inside a bordered box
    auto input_renderer = Renderer(input_container, [&] {
        return vbox({
            text("Bandwidth Shaper"),
            separator(),
            vbox({
                hbox(text("Min Bandwidth: "), input_field1->Render()) | flex,
                hbox(text("Max Bandwidth: "), input_field2->Render()) | flex,
                hbox(text("Step Bandwidth: "), input_field3->Render()) | flex,
                hbox(text("Time (ms): "), input_field4->Render()) | flex,
                separator(),
                center_button->Render(),  // Render the center button
            }),  // Add a border around the input fields and button
        });
    });

    // Button for the left column
    std::string shaping_start_stop_btn_text = "Start";
    auto shaping_start_stop_btn = Button(&shaping_start_stop_btn_text, [&] {
        shaping_start_stop_btn_text = shaping_start_stop_btn_text == "Start" ? "Stop" : "Start";
    });

    auto shaping_control_group_renderer = Renderer(shaping_start_stop_btn, [&] {
       return vbox({
           text("Shaping Control"),
           separator(),
           vbox({
               text("Bandwidth: " + std::to_string(g_currentBandwidth_bps / 1000) + " kbps")| flex,
               separator(),
               shaping_start_stop_btn->Render(),  // Render the center button
           }) | flex ,  // Add a border around the input fields and button
       });
   });


    // Button for the right column
    std::string right_button_label = "Right Button";
    auto right_button = Button(&right_button_label, [&] {
        std::cout << "Right button clicked!" << std::endl;
    });

    // Create a main container with three columns: left button, inputs, and right button
    auto main_container = Container::Horizontal({
        input_container,  // Center input fields and button
        shaping_control_group_renderer,  // Left column button
        right_button,  // Right column button
    });

    // Render the full layout with the left, center, and right components
    auto main_renderer = Renderer(main_container, [&] {
        return hbox({
            input_renderer->Render() | flex,  // Center input fields and button
            separator(),
            shaping_control_group_renderer->Render() | flex,  // Left button
            separator(),
            right_button->Render() | flex,  // Right button
        });
    });

    // Create a screen and run the interactive loop
    auto screen = ScreenInteractive::Fullscreen();
    screen.Loop(main_renderer);

    //
    // g_isRunning = true;
    // while (g_isRunning)
    // {
    //     std::cout << "Range: [" << g_minBandwidth_kbps << ", " << g_maxBandwidth_kbps << "] kbps, Stepsize: "
    //     << g_stepSizeBandwidth_kbps << " kbps, Runtime: " << g_maxShapingTime.count() << std::endl;
    //
    //     std::cout << "1: Configure Parameters" << std::endl;
    //     std::cout << "2: Start Shaping" << std::endl;
    //     std::cout << "3: Stop Shaping" << std::endl;
    //     std::cout << "Select option: ";
    //     std::string input;
    //     std::getline(std::cin, input);
    //
    //     if (input == "1")
    //     {
    //         ReadValue("Min Bandwidth: ", g_minBandwidth_kbps);
    //         ReadValue("Max Bandwidth: ", g_maxBandwidth_kbps);
    //         ReadValue("Stepsize Bandwidth: ", g_stepSizeBandwidth_kbps);
    //         ReadValue("Shaping Runtime (ms): ", g_maxShapingTime);
    //     }
    //     else if (input == "2")
    //     {
    //         g_shapingMode = Mode::Shaping;
    //     }
    //     else if (input == "3")
    //     {
    //         g_shapingMode = Mode::Reset;
    //     }
    // }
}


