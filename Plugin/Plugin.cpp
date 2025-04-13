#include "../Interface.h"
#include <iostream>

using namespace std;

void CallBack(event_t event)
{
    // Process the event
    std::cout << "Event: " << event.name << " " << event.data << std::endl;
}

extern "C"
{
    void InitializePlugin(register_event_callback_t register_callback)
    {
        // Initialize the plugin
        cout << "InitializePlugin beging called" << endl;
        register_callback(CallBack);
    }
}
