typedef struct {
    const char* name;
    const char* data;
} event_t;

using event_callback_t = void (*)(event_t);
using register_event_callback_t = void (*)(event_callback_t);
