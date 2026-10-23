#include "../common.h"
#include "../hv/events.h"

const char *vm_state_name(VmState state) {
    static const char *names[VM_STATE_COUNT] = {
        "NEW", "READY", "RUNNING", "BLOCKED", "PAUSED", "TERMINATED"
    };
    if (state < 0 || state >= VM_STATE_COUNT) return "UNKNOWN";
    return names[state];
}

const char *device_name(DeviceType type) {
    static const char *names[DEV_COUNT] = {"CONSOLE", "DISK", "NET", "SWAP"};
    if (type < 0 || type >= DEV_COUNT) return "UNKNOWN";
    return names[type];
}

void util_trim(char *text) {
    size_t length;
    char *begin;
    if (text == NULL) return;
    begin = text;
    while (*begin != '\0' && isspace((unsigned char)*begin)) begin++;
    if (begin != text) memmove(text, begin, strlen(begin) + 1U);
    length = strlen(text);
    while (length > 0U && isspace((unsigned char)text[length - 1U])) {
        text[--length] = '\0';
    }
}

bool util_parse_i32(const char *text, int32_t *value) {
    char *end = NULL;
    long long parsed;
    if (text == NULL || value == NULL || *text == '\0') return false;
    parsed = strtoll(text, &end, 10);
    if (end == text || *end != '\0' || parsed < INT32_MIN || parsed > INT32_MAX) return false;
    *value = (int32_t)parsed;
    return true;
}

bool util_parse_u64(const char *text, uint64_t *value) {
    char *end = NULL;
    unsigned long long parsed;
    if (text == NULL || value == NULL || *text == '\0' || *text == '-') return false;
    parsed = strtoull(text, &end, 10);
    if (end == text || *end != '\0') return false;
    *value = (uint64_t)parsed;
    return true;
}

bool util_parse_size(const char *text, size_t *value) {
    uint64_t parsed;
    if (value == NULL || !util_parse_u64(text, &parsed) || parsed > (uint64_t)SIZE_MAX) return false;
    *value = (size_t)parsed;
    return true;
}

void hv_emit_instruction(Hv *hv, Vm *vm, bool io_instruction) {
    HvEvent event;
    if (hv == NULL || vm == NULL) return;
    event.tick = hv->tick;
    event.level = "DEBUG";
    event.module = "cpu";
    event.vm = vm->name;
    event.event = "CPU_INSTRUCTION";
    event.detail = io_instruction ? "io=true" : "io=false";
    event.vm_id = vm->id;
    event.instruction = true;
    event.io_instruction = io_instruction;
    hv_emit(hv, &event);
}
