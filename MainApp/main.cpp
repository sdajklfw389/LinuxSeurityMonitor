#include <cstring>
#include <cstddef>
#include <vector>
#include <iostream>

static_assert(::memcmp != nullptr, "2: memcmp not in global namespace");


// System headers
#include <dlfcn.h>
#include <linux/perf_event.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>

// Your project headers
#include "../Interface.h"

using namespace std;

std::vector<event_callback_t> event_callbacks;

static void HandleEvent(void *ctx, int cpu, void *data, __u32 size)
{
    std::cout << "Event received! Size: " << size << std::endl;
    const event_t* event = static_cast<const event_t*>(data);
    // Dispatch to plugins
    for (auto& callback : event_callbacks)
    {
        callback(*event);
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
        // Open and load ebpf program
        struct bpf_object *obj = bpf_object__open("probe.bpf.o");
        if (!obj)
        {
            std::cerr << "Failed to open ebpf program" << std::endl;
            return -1;
        }

        std::cout << "Successfully opened BPF object" << std::endl;
        // Load the ebpf program
        if (bpf_object__load(obj))
        {
            std::cerr << "Failed to load ebpf program" << std::endl;
            return -1;
        }
        std::cout << "Successfully loaded ebpf program" << std::endl;

        // After loading the BPF object
        struct bpf_program *prog = bpf_object__find_program_by_name(obj, "trace_execve");
        if (!prog) {
            std::cerr << "Failed to find BPF program 'trace_execve'" << std::endl;
            return -1;
        }
        std::cout << "Found BPF program" << std::endl;

        // Attach the program to the tracepoint
        struct bpf_link *link = bpf_program__attach(prog);
        if (!link) {
            std::cerr << "Failed to attach BPF program: " << errno << std::endl;
            return -1;
        }
        std::cout << "Successfully attached BPF program to tracepoint" << std::endl;

        // Set up perf buffer to receive events
        struct perf_buffer *pb = perf_buffer__new(
            bpf_map__fd(bpf_object__find_map_by_name(obj, "events")),
            8,  // pages
            HandleEvent,
            NULL,  // lost_cb
            NULL,  // ctx
            NULL   // opts
        );

        std::cout << "Successfully set up perf buffer" << std::endl;
        // Main event loop
        while (true) {
            perf_buffer__poll(pb, 100 /* timeout, ms */);
        }

        return 0;
    }

    return -1;
}