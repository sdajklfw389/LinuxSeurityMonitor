#include "vmlinux.h"
// BPF helper headers
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

// Define the event structure
struct event_t {
    __u32 pid;
    __u32 uid;
    char comm[16];
};

// Define a map to share data with userspace
struct {
    __uint(type, BPF_MAP_TYPE_PERF_EVENT_ARRAY);
    __uint(key_size, sizeof(int));
    __uint(value_size, sizeof(int));
} events SEC(".maps");

// Example: track process executions
SEC("tracepoint/syscalls/sys_enter_execve")
int trace_execve(struct trace_event_raw_sys_enter *ctx)
{
    struct event_t event = {};  // Initialize event structure
    
    // Get process info
    event.pid = bpf_get_current_pid_tgid() >> 32;
    event.uid = bpf_get_current_uid_gid() >> 32;
    bpf_get_current_comm(&event.comm, sizeof(event.comm));

    bpf_printk("Caught execve: pid = %d, comm = %s\n", event.pid, event.comm);
    // Send event to userspace
    bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU, 
                         &event, sizeof(event));
    return 0;
}

char LICENSE[] SEC("license") = "GPL";
