#include "../AutoOperateLib/AutoOperateLib.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <vector>
#include <map>

using namespace std;
vector<AO_Window> hidenWindows;
vector<AO_Window> showWindows;


std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}
void Func1()
{
    for (AO_Window& window : hidenWindows)
    {
        SetWindowVisibility(window, false);
    }
    for (AO_Window& window : showWindows)
    {
        BringWindowToFront(window);
    }
}
void Func2()
{
    for (AO_Window& window : hidenWindows)
    {
        SetWindowVisibility(window, true);
    }
}
int main(int argc, char* argv[])
{
    std::ifstream file("config.ini");
    if (!file.is_open()) {
        std::cerr << "Unable to open file config.ini" << std::endl;
        return 1;
    }

    std::string line;
    std::map<std::string, std::vector<std::string>> configMap;
    std::string currentSection;

    while (std::getline(file, line)) {
        // Trim whitespace from the start and end of the line
        line.erase(0, line.find_first_not_of(" \t\n\r\f\v"));
        line.erase(line.find_last_not_of(" \t\n\r\f\v") + 1);

        if (line.empty() || line[0] == ';') {
            // Skip empty lines and comments
            continue;
        }
        else if (line[0] == '[' && line.back() == ']') {
            // Section header
            currentSection = line.substr(1, line.size() - 2);
            configMap[currentSection] = std::vector<std::string>();
        }
        else {
            // Key-Value pair
            std::string::size_type pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);

                // Convert key to lowercase to handle case insensitivity
                std::transform(key.begin(), key.end(), key.begin(), ::tolower);

                // Split the value by ';'
                std::vector<std::string> values = split(value, ';');

                // Store the key-value pair in the map under the current section
                configMap[key] = values;
            }
        }
    }

    file.close();
    const string currentProcessName = GetCurrentProcessName();


    configMap["hidenprocesssubname"].push_back(currentProcessName.substr(0, currentProcessName.find_last_of('.')));

    for (const string& processName : configMap["hidenprocesssubname"])
    {
        const std::vector<AO_Window> windows = GetProcessWindows(processName);
        hidenWindows.insert(hidenWindows.end(), windows.begin(), windows.end());
    }
    for (const string& processName : configMap["hidenwindowsubname"])
    {
        const std::vector<AO_Window> windows = GetWindows(processName);
        hidenWindows.insert(hidenWindows.end(), windows.begin(), windows.end());
    }
    for (const string& processName : configMap["showprocesssubname"])
    {
        const std::vector<AO_Window> windows = GetProcessWindows(processName);
        showWindows.insert(showWindows.end(), windows.begin(), windows.end());
    }
    for (const string& processName : configMap["showwindowssubname"])
    {
        const std::vector<AO_Window> windows = GetWindows(processName);
        showWindows.insert(showWindows.end(), windows.begin(), windows.end());
    }

    AO_HotkeyManager manager;
    manager.RegisterHotkey(1, MOD_ALT, '1', &Func1);
    manager.RegisterHotkey(2, MOD_ALT, '2', &Func2);

    while (true)
    {
        WaitForS(60);
    }
    manager.UnregisterHotkey(1);
    manager.UnregisterHotkey(2);
    return 0;
}