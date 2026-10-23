#include "stats.h"
#include "../hv/events.h"
#include "../io/io.h"
#include "../mem/mem.h"
#include "../vm/vm.h"

static void stats_observer(Hv *hv, const HvEvent *event, void *context) {
    (void)context;
    if (hv == NULL || event == NULL) return;
    hv->observed_events++;
    if (event->event != NULL && strcmp(event->event, "STATE_CHANGE") == 0) hv->observed_state_changes++;
}

bool stats_init(Hv *hv) {
    return hv_observe(hv, stats_observer, NULL);
}

static size_t active_count(const Hv *hv) {
    size_t count = 0U;
    if (hv == NULL) return 0U;
    for (size_t i = 0; i < hv->vm_count; ++i) {
        VmState state = hv->vms[i].state;
        if (state != VM_NEW && state != VM_TERMINATED) count++;
    }
    return count;
}

void stats_print(const Hv *hv) {
    uint64_t completed = 0U;
    if (hv == NULL) return;
    for (size_t i = 0; i < hv->vm_count; ++i) {
        if (hv->vms[i].state == VM_TERMINATED && hv->vms[i].finish != NO_TICK) completed++;
    }
    (void)printf("tick=%llu VMs=%lu ativas=%lu terminadas=%llu ctx_switches=%llu cpu_busy_ticks=%llu\n",
        (unsigned long long)hv->tick, (unsigned long)hv->vm_count,
        (unsigned long)active_count(hv), (unsigned long long)completed,
        (unsigned long long)hv->context_switches, (unsigned long long)hv->cpu_busy_ticks);
    mem_print_status(hv);
    io_print_status(hv);
    (void)printf("Observer: eventos=%llu mudancas_estado=%llu\n",
        (unsigned long long)hv->observed_events,
        (unsigned long long)hv->observed_state_changes);
}

void stats_report(Hv *hv) {
    uint64_t finished_count = 0U;
    double turnaround_sum = 0.0;
    double wait_sum = 0.0;
    double response_sum = 0.0;
    uint64_t total_io = 0U;
    double occupancy = 0.0;
    if (hv == NULL || hv->report_printed) return;
    hv->report_printed = true;
    for (size_t i = 0; i < hv->vm_count; ++i) {
        Vm *vm = &hv->vms[i];
        total_io += vm->io_requests;
        if (vm->state == VM_TERMINATED && vm->finish != NO_TICK && vm->finish >= vm->arrival) {
            finished_count++;
            turnaround_sum += (double)(vm->finish - vm->arrival);
            wait_sum += (double)vm->wait_ticks;
            response_sum += vm->first_dispatch == NO_TICK ? 0.0 : (double)(vm->first_dispatch - vm->arrival);
        }
    }
    if (hv->occupancy_samples > 0U && hv->frame_count > 0U) {
        occupancy = 100.0 * (double)hv->occupancy_sum /
                    ((double)hv->occupancy_samples * (double)hv->frame_count);
    }
    (void)printf("\n== MiniHV | escopo 02 | policy=%s | mem=PAG/LRU/GLOBAL | disk=C_LOOK ==\n", hv->cfg.policy);
    (void)printf("ticks=%llu pcpus=%d ctx_switches=%llu\n\n",
        (unsigned long long)hv->tick, hv->cfg.pcpus, (unsigned long long)hv->context_switches);
    (void)printf("VM                  estado                chegada fim    turnaround espera resposta cpu   faults io\n");
    for (size_t i = 0; i < hv->vm_count; ++i) {
        Vm *vm = &hv->vms[i];
        uint64_t finish = vm->finish == NO_TICK ? hv->tick : vm->finish;
        uint64_t turnaround = finish >= vm->arrival ? finish - vm->arrival : 0U;
        uint64_t response = vm->first_dispatch == NO_TICK || vm->first_dispatch < vm->arrival ? 0U : vm->first_dispatch - vm->arrival;
        const char *state = vm->state == VM_TERMINATED && vm->exit_reason[0] != '\0' ? vm->exit_reason : vm_state_name(vm->state);
        (void)printf("%-19s %-21s %-7llu %-7llu %-11llu %-7llu %-8llu %-5llu %-6llu %llu\n",
            vm->name, state, (unsigned long long)vm->arrival, (unsigned long long)finish,
            (unsigned long long)turnaround, (unsigned long long)vm->wait_ticks,
            (unsigned long long)response, (unsigned long long)vm->cpu_ticks,
            (unsigned long long)vm->page_faults, (unsigned long long)vm->io_requests);
    }
    (void)printf("\nCPU utilizacao: cpu0=%.1f%%\n",
        hv->tick == 0U ? 0.0 : 100.0 * (double)hv->cpu_busy_ticks / (double)hv->tick);
    (void)printf("Medias: turnaround=%.2f espera=%.2f resposta=%.2f throughput=%.4f VM/tick\n",
        finished_count == 0U ? 0.0 : turnaround_sum / (double)finished_count,
        finished_count == 0U ? 0.0 : wait_sum / (double)finished_count,
        finished_count == 0U ? 0.0 : response_sum / (double)finished_count,
        hv->tick == 0U ? 0.0 : (double)finished_count / (double)hv->tick);
    (void)printf("Memoria: page_faults=%llu substituicoes=%llu sujas=%llu ocupacao_media=%.1f%%\n",
        (unsigned long long)hv->page_faults, (unsigned long long)hv->replacements,
        (unsigned long long)hv->dirty_writebacks, occupancy);
    (void)printf("E/S total: requisicoes_VM=%llu\n", (unsigned long long)total_io);
    io_print_report(hv);
}
