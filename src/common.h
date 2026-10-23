#ifndef MINIHV_COMMON_H
#define MINIHV_COMMON_H

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util/prng.h"
#include "hv/hv.h"

#define MINIHV_SCOPE 2
#define MAX_VMS 16
#define MAX_NAME 64
#define MAX_PATH_LEN 260
#define MAX_INSTRUCTIONS 2048
#define MAX_LINE_LEN 1024
#define MAX_IO_REQUESTS 4096
#define VM_REG_COUNT 4
#define NO_TICK UINT64_MAX

typedef enum {
    VM_NEW = 0,
    VM_READY,
    VM_RUNNING,
    VM_BLOCKED,
    VM_PAUSED,
    VM_TERMINATED,
    VM_STATE_COUNT
} VmState;

typedef enum {
    OP_MOV = 0, OP_ADD, OP_ADDI, OP_SUB, OP_LOAD, OP_STORE,
    OP_JMP, OP_JZ, OP_JNZ, OP_OUT, OP_DISK, OP_NET, OP_HALT,
    OP_INVALID
} OpCode;

typedef struct {
    OpCode op;
    int rd;
    int rs;
    int32_t imm;
    int target;
    bool indirect;
} Instruction;

typedef struct {
    char name[MAX_NAME];
    char program[MAX_PATH_LEN];
    uint64_t arrival;
    int priority;
    size_t mem_words;
    int cap;
} VmConfig;

typedef struct {
    int pcpus;
    int max_vms;
    uint64_t max_ticks;
    uint32_t seed;
    char policy[16];
    int quantum;
    char mem_model[16];
    size_t phys_words;
    size_t page_words;
    char replacement[16];
    char repl_scope[16];
    int pf_penalty;
    char disk_algorithm[16];
    int disk_cylinders;
    int disk_head;
    int seek_rate;
    char log_format[16];
    char log_level[16];
    char log_file[MAX_PATH_LEN];
    bool ext_enabled;
    uint64_t watchdog_limit;
    VmConfig vms[MAX_VMS];
    size_t vm_count;
} Config;

typedef struct {
    bool present;
    bool pending;
    bool referenced;
    bool dirty;
    int frame;
    uint64_t load_tick;
    uint64_t last_access;
    uint64_t accesses;
} PageEntry;

typedef struct {
    bool used;
    bool pending;
    int owner_vm;
    size_t page;
    uint64_t load_tick;
    uint64_t last_access;
} Frame;

typedef struct {
    int id;
    char name[MAX_NAME];
    char program_path[MAX_PATH_LEN];
    VmState state;
    char exit_reason[32];
    int32_t regs[VM_REG_COUNT];
    int64_t pc;
    Instruction *program;
    size_t program_count;
    size_t mem_words;
    size_t page_count;
    int32_t *backing;
    PageEntry *pages;
    uint64_t arrival;
    uint64_t first_dispatch;
    uint64_t finish;
    uint64_t wait_ticks;
    uint64_t cpu_ticks;
    uint64_t page_faults;
    uint64_t io_requests;
    uint64_t burst_ticks;
    uint64_t ready_order;
    uint64_t cpu_since_io;
    int priority;
    int last_cpu;
    double tau;
    bool watchdog_warned;
    bool admitted;
    bool queued;
} Vm;

typedef enum {
    DEV_CONSOLE = 0,
    DEV_DISK,
    DEV_NET,
    DEV_SWAP,
    DEV_COUNT
} DeviceType;

typedef struct {
    bool used;
    bool active;
    DeviceType device;
    int vm_id;
    uint64_t sequence;
    uint64_t enqueued_tick;
    int cylinder;
    uint32_t words;
    int32_t value;
    size_t page;
    uint32_t remaining;
    int64_t distance;
} IoRequest;

typedef struct {
    int active_request;
    uint64_t requests_served;
    uint64_t wait_total;
    uint64_t displacement_total;
    int head;
} IoDevice;

struct SchedPolicy;
struct MemPolicy;
struct Queue;
struct List;

typedef void (*HvIoInterruptHandler)(Hv *hv, int vm_id, DeviceType device, size_t page, void *context);

struct Hv {
    Config cfg;
    Vm vms[MAX_VMS];
    size_t vm_count;
    int32_t *physical_memory;
    Frame *frames;
    size_t frame_count;
    IoRequest requests[MAX_IO_REQUESTS];
    IoDevice devices[DEV_COUNT];
    struct Queue *io_queues[DEV_COUNT];
    struct Queue *ready_queue;
    struct List *observers;
    HvIoInterruptHandler io_interrupt_handler;
    void *io_interrupt_context;
    Prng prng;
    uint64_t observed_events;
    uint64_t observed_state_changes;
    FILE *log_file;
    uint64_t tick;
    uint64_t ready_sequence;
    uint64_t io_sequence;
    uint64_t context_switches;
    uint64_t cpu_busy_ticks;
    uint64_t occupancy_sum;
    uint64_t occupancy_samples;
    uint64_t page_faults;
    uint64_t replacements;
    uint64_t dirty_writebacks;
    int running_vm;
    int last_dispatched_vm;
    const struct SchedPolicy *sched_policy_impl;
    const struct MemPolicy *mem_policy_impl;
    bool initialized;
    bool report_printed;
};

const char *vm_state_name(VmState state);
const char *device_name(DeviceType type);
void util_trim(char *text);
bool util_parse_i32(const char *text, int32_t *value);
bool util_parse_u64(const char *text, uint64_t *value);
bool util_parse_size(const char *text, size_t *value);

#endif
