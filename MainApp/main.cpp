// Main app is to dispatch the events to the plugins.

#include "../Interface.h"
#include <vector>
#include <dlfcn.h>
#include <iostream>

using namespace std;

std::vector<event_callback_t> event_callbacks;

void DispatchEvents()
{
    cout << "beging to dispatch events" << endl;
    for (auto& callback : event_callbacks)
    {
        callback(event_t{ "event_name", "event_data" });
    }
}

void RegisterEventCallback(event_callback_t event_callback)
{
    event_callbacks.push_back(event_callback);
}

int LoadPlugins()
{
    void* hModule = dlopen("./libPlugin.so", RTLD_NOW);

    // Load the plugins
    if (hModule)
    {
        cout << "dlopen success" << endl;
        void* sym = dlsym(hModule, "InitializePlugin");
        const char* dlsym_error = dlerror();
        if (dlsym_error)
        {
            cout << "dlsym failed: " << dlsym_error << endl;
            dlclose(hModule);
            return -1;
        }

        cout << "dlsym success" << endl;
        auto InitializePlugin = reinterpret_cast<void (*)(register_event_callback_t)>(sym);
        if (!InitializePlugin)
        {
            cout << "InitializePlugin cast failed: " << endl;
            dlclose(hModule);
            return -1;
        }

        cout << "About to call InitializePlugin..." << endl;
        InitializePlugin(reinterpret_cast<register_event_callback_t>(RegisterEventCallback));
        cout << "InitializePlugin called successfully" << endl;

        return 0;
    }

    // Print the error message from dlerror()
    cout << "dlopen failed: " << dlerror() << endl;

    return -1;
}

int main()
{
    cout << "main() called" << endl;
    // Load the plugins
    if (LoadPlugins() == 0)
    {
        cout << "LoadPlugins success" << endl;
        // Dispatch the events to the plugins
        // Later should be triggered by the ebpf.
        DispatchEvents();

        return 0;
    }

    return -1;
}