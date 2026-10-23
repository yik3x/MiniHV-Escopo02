#include "cpu.h"
#include "../io/io.h"
#include "../log/log.h"
#include "../mem/mem.h"
#include "../vm/vm.h"
#include "../util/util.h"

static int32_t wrap32(int64_t value) {
    int64_t wrapped = value % INT64_C(4294967296);
    if (wrapped < 0) wrapped += INT64_C(4294967296);
    if (wrapped >= INT64_C(2147483648)) wrapped -= INT64_C(4294967296);
    return (int32_t)wrapped;
}

static bool valid_target(const Vm *vm, int target) {
    return target >= 0 && (size_t)target < vm->program_count;
}

static void invalid_target(Hv *hv, Vm *vm, int target) {
    char detail[96];
    (void)snprintf(detail, sizeof(detail), "target=%d program_size=%lu", target, (unsigned long)vm->program_count);
    log_event(hv, "ERROR", "cpu", vm->name, "INVALID_JUMP", detail);
    vm_exit(hv, vm, "INVALID_INSTRUCTION");
}

static bool submit_io(Hv *hv, Vm *vm, DeviceType device, int cylinder, uint32_t words, int32_t value) {
    if (!vm_set_state(hv, vm, VM_BLOCKED)) return false;
    if (!io_submit(hv, device, vm->id, cylinder, words, value, 0U)) {
        vm_exit(hv, vm, "KILLED");
        return false;
    }
    return true;
}

void cpu_step(Hv *hv, Vm *vm) {
    Instruction *instruction;
    MemResult mem_result;
    int64_t current_pc;
    int64_t next_pc;
    int64_t address;
    int32_t value;
    bool io_instruction = false;
    char detail[128];
    if (hv == NULL || vm == NULL || vm->state != VM_RUNNING) return;
    current_pc = vm->pc;
    if (current_pc < 0 || (uint64_t)current_pc >= (uint64_t)vm->program_count) {
        invalid_target(hv, vm, current_pc > INT32_MAX ? INT32_MAX : (int)current_pc);
        return;
    }
    hv->cpu_busy_ticks++;
    vm->cpu_ticks++;
    vm->burst_ticks++;
    instruction = &vm->program[(size_t)current_pc];
    if (mem_fetch_instruction(hv, vm, current_pc) != MEM_OK) {
        if (vm->state != VM_TERMINATED) hv_emit_instruction(hv, vm, false);
        return;
    }
    next_pc = current_pc + 1;
    switch (instruction->op) {
        case OP_MOV:
            vm->regs[instruction->rd] = instruction->imm;
            vm->pc = next_pc;
            break;
        case OP_ADD:
            vm->regs[instruction->rd] = wrap32((int64_t)vm->regs[instruction->rd] + vm->regs[instruction->rs]);
            vm->pc = next_pc;
            break;
        case OP_ADDI:
            vm->regs[instruction->rd] = wrap32((int64_t)vm->regs[instruction->rd] + instruction->imm);
            vm->pc = next_pc;
            break;
        case OP_SUB:
            vm->regs[instruction->rd] = wrap32((int64_t)vm->regs[instruction->rd] - vm->regs[instruction->rs]);
            vm->pc = next_pc;
            break;
        case OP_LOAD:
            address = instruction->indirect ? vm->regs[instruction->rs] : instruction->imm;
            mem_result = mem_read_word(hv, vm, address, &value);
            if (mem_result == MEM_PENDING) break;
            if (mem_result == MEM_ERROR) return;
            vm->regs[instruction->rd] = value;
            vm->pc = next_pc;
            break;
        case OP_STORE:
            address = instruction->indirect ? vm->regs[instruction->rd] : instruction->imm;
            mem_result = mem_write_word(hv, vm, address, vm->regs[instruction->rs]);
            if (mem_result == MEM_PENDING) break;
            if (mem_result == MEM_ERROR) return;
            vm->pc = next_pc;
            break;
        case OP_JMP:
            if (!valid_target(vm, instruction->target)) { invalid_target(hv, vm, instruction->target); return; }
            vm->pc = instruction->target;
            break;
        case OP_JZ:
        case OP_JNZ: {
            bool take = instruction->op == OP_JZ ? vm->regs[instruction->rs] == 0 : vm->regs[instruction->rs] != 0;
            if (take && !valid_target(vm, instruction->target)) { invalid_target(hv, vm, instruction->target); return; }
            vm->pc = take ? instruction->target : next_pc;
            break;
        }
        case OP_OUT:
            vm->pc = next_pc;
            io_instruction = true;
            hv_emit_instruction(hv, vm, true);
            (void)submit_io(hv, vm, DEV_CONSOLE, 0, 0U, vm->regs[instruction->rs]);
            (void)snprintf(detail, sizeof(detail), "value=%d device=CONSOLE", vm->regs[instruction->rs]);
            log_event(hv, "INFO", "cpu", vm->name, "OUT", detail);
            break;
        case OP_DISK: {
            int cylinder = vm->regs[instruction->rs] % hv->cfg.disk_cylinders;
            if (cylinder < 0) cylinder += hv->cfg.disk_cylinders;
            vm->pc = next_pc;
            io_instruction = true;
            hv_emit_instruction(hv, vm, true);
            (void)submit_io(hv, vm, DEV_DISK, cylinder, 0U, 0);
            (void)snprintf(detail, sizeof(detail), "cylinder=%d algorithm=C_LOOK", cylinder);
            log_event(hv, "INFO", "cpu", vm->name, "DISK", detail);
            break;
        }
        case OP_NET:
            vm->pc = next_pc;
            io_instruction = true;
            hv_emit_instruction(hv, vm, true);
            (void)submit_io(hv, vm, DEV_NET, 0, (uint32_t)instruction->imm, 0);
            (void)snprintf(detail, sizeof(detail), "words=%d", instruction->imm);
            log_event(hv, "INFO", "cpu", vm->name, "NET", detail);
            break;
        case OP_HALT:
            vm_exit(hv, vm, "HALT");
            break;
        case OP_INVALID:
        default:
            vm_exit(hv, vm, "INVALID_INSTRUCTION");
            break;
    }
    if (!io_instruction && vm->state != VM_TERMINATED) hv_emit_instruction(hv, vm, false);
}
