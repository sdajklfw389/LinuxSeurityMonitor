struct event_t {
    int pid;           // Process ID
    int uid;           // User ID
    char comm[16];       // Process name
};

using event_callback_t = void (*)(event_t);
using register_event_callback_t = void (*)(event_callback_t);
