#include "io.h"
#include "../log/log.h"
#include "../util/queue.h"

bool io_init(Hv *hv) {
    if (hv == NULL) return false;
    for (int i = 0; i < DEV_COUNT; ++i) {
        hv->devices[i].active_request = -1;
        hv->io_queues[i] = queue_create();
        if (hv->io_queues[i] == NULL) return false;
    }
    hv->devices[DEV_DISK].head = hv->cfg.disk_head;
    return true;
}

void io_set_interrupt_handler(Hv *hv, HvIoInterruptHandler handler, void *context) {
    if (hv == NULL) return;
    hv->io_interrupt_handler = handler;
    hv->io_interrupt_context = context;
}

void io_destroy(Hv *hv) {
    if (hv == NULL) return;
    for (int i = 0; i < DEV_COUNT; ++i) {
        queue_destroy(hv->io_queues[i]);
        hv->io_queues[i] = NULL;
    }
}

bool io_submit(Hv *hv, DeviceType device, int vm_id, int cylinder,
               uint32_t words, int32_t value, size_t page) {
    int slot = -1;
    char detail[160];
    if (hv == NULL || device < 0 || device >= DEV_COUNT) return false;
    for (size_t i = 0; i < MAX_IO_REQUESTS; ++i) {
        if (!hv->requests[i].used) { slot = (int)i; break; }
    }
    if (slot < 0) {
        log_event(hv, "ERROR", "io", vm_id >= 0 ? hv->vms[vm_id].name : "-", "IO_QUEUE_FULL", device_name(device));
        return false;
    }
    IoRequest *request = &hv->requests[slot];
    memset(request, 0, sizeof(*request));
    request->used = true;
    request->active = false;
    request->device = device;
    request->vm_id = vm_id;
    request->sequence = hv->io_sequence++;
    request->enqueued_tick = hv->tick;
    request->cylinder = cylinder;
    request->words = words;
    request->value = value;
    request->page = page;
    if (!queue_push(hv->io_queues[device], request)) {
        request->used = false;
        log_event(hv, "ERROR", "io", vm_id >= 0 ? hv->vms[vm_id].name : "-", "IO_QUEUE_NOMEM", device_name(device));
        return false;
    }
    if (vm_id >= 0 && (size_t)vm_id < hv->vm_count) hv->vms[vm_id].io_requests++;
    (void)snprintf(detail, sizeof(detail), "device=%s", device_name(device));
    if (device == DEV_DISK) (void)snprintf(detail, sizeof(detail), "device=DISK cylinder=%d", cylinder);
    else if (device == DEV_NET) (void)snprintf(detail, sizeof(detail), "device=NET words=%u", words);
    else if (device == DEV_SWAP) (void)snprintf(detail, sizeof(detail), "device=SWAP page=%lu", (unsigned long)page);
    log_event(hv, "INFO", "io", vm_id >= 0 ? hv->vms[vm_id].name : "-", "IO_REQ", detail);
    return true;
}

static int compare_fifo(const void *left, const void *right, void *context) {
    const IoRequest *a = (const IoRequest *)left;
    const IoRequest *b = (const IoRequest *)right;
    (void)context;
    if (a->sequence < b->sequence) return -1;
    if (a->sequence > b->sequence) return 1;
    return 0;
}

static int compare_c_look(const void *left, const void *right, void *context) {
    const IoRequest *a = (const IoRequest *)left;
    const IoRequest *b = (const IoRequest *)right;
    const Hv *hv = (const Hv *)context;
    int head = hv->devices[DEV_DISK].head;
    bool a_up = a->cylinder >= head;
    bool b_up = b->cylinder >= head;
    if (a_up != b_up) return a_up ? -1 : 1;
    if (a->cylinder < b->cylinder) return -1;
    if (a->cylinder > b->cylinder) return 1;
    return compare_fifo(left, right, context);
}

static int find_pending(Hv *hv, DeviceType device) {
    QueueCompare compare = device == DEV_DISK ? compare_c_look : compare_fifo;
    IoRequest *selected = (IoRequest *)queue_take_best(hv->io_queues[device], compare, hv);
    if (selected == NULL) return -1;
    return (int)(selected - hv->requests);
}

static uint32_t latency_for(Hv *hv, IoRequest *r) {
    if (r->device == DEV_CONSOLE) return 2U;
    if (r->device == DEV_NET) return 2U + (r->words + 7U) / 8U;
    if (r->device == DEV_SWAP) return (uint32_t)hv->cfg.pf_penalty;
    if (r->device == DEV_DISK) {
        int head = hv->devices[DEV_DISK].head;
        int cylinders = hv->cfg.disk_cylinders;
        int distance = r->cylinder >= head ? r->cylinder - head : (cylinders - head) + r->cylinder;
        r->distance = distance;
        return 1U + (uint32_t)((distance + hv->cfg.seek_rate - 1) / hv->cfg.seek_rate);
    }
    return 1U;
}

static void complete_request(Hv *hv, int slot) {
    IoRequest completed = hv->requests[slot];
    IoDevice *device = &hv->devices[completed.device];
    char detail[160];
    Vm *vm = completed.vm_id >= 0 && (size_t)completed.vm_id < hv->vm_count ? &hv->vms[completed.vm_id] : NULL;
    device->requests_served++;
    if (completed.device == DEV_DISK) {
        device->displacement_total += (uint64_t)completed.distance;
        device->head = completed.cylinder;
    }
    (void)snprintf(detail, sizeof(detail), "device=%s", device_name(completed.device));
    if (completed.device == DEV_DISK) (void)snprintf(detail, sizeof(detail), "device=DISK cylinder=%d distance=%lld", completed.cylinder, (long long)completed.distance);
    if (completed.device == DEV_SWAP) (void)snprintf(detail, sizeof(detail), "device=SWAP page=%lu", (unsigned long)completed.page);
    log_event(hv, "INFO", "io", vm == NULL ? "-" : vm->name, "IO_DONE", detail);
    if (completed.device == DEV_CONSOLE && vm != NULL) {
        (void)printf("[t=%04llu] %s: %d\n", (unsigned long long)hv->tick, vm->name, completed.value);
    }
    hv->requests[slot].used = false;
    hv->requests[slot].active = false;
    device->active_request = -1;
    if (vm != NULL && hv->io_interrupt_handler != NULL) {
        hv->io_interrupt_handler(hv, completed.vm_id, completed.device,
                                 completed.page, hv->io_interrupt_context);
    }
}

void io_tick(Hv *hv) {
    if (hv == NULL) return;
    for (int d = 0; d < DEV_COUNT; ++d) {
        IoDevice *device = &hv->devices[d];
        int slot = device->active_request;
        if (slot < 0) {
            slot = find_pending(hv, (DeviceType)d);
            if (slot < 0) continue;
            IoRequest *request = &hv->requests[slot];
            request->active = true;
            request->remaining = latency_for(hv, request);
            device->active_request = slot;
            device->wait_total += hv->tick - request->enqueued_tick;
            char detail[160];
            if (request->device == DEV_DISK) {
                (void)snprintf(detail, sizeof(detail), "algorithm=C_LOOK cylinder=%d distance=%lld latency=%u",
                               request->cylinder, (long long)request->distance, request->remaining);
            } else {
                (void)snprintf(detail, sizeof(detail), "device=%s latency=%u", device_name(request->device), request->remaining);
            }
            log_event(hv, "DEBUG", "io", request->vm_id >= 0 ? hv->vms[request->vm_id].name : "-", "IO_START", detail);
        }
        if (device->active_request >= 0) {
            IoRequest *request = &hv->requests[device->active_request];
            if (request->remaining > 0U) request->remaining--;
            if (request->remaining == 0U) complete_request(hv, device->active_request);
        }
    }
}

void io_cancel_vm(Hv *hv, int vm_id) {
    if (hv == NULL) return;
    for (size_t i = 0; i < MAX_IO_REQUESTS; ++i) {
        IoRequest *request = &hv->requests[i];
        if (request->used && request->vm_id == vm_id) {
            if (request->active) hv->devices[request->device].active_request = -1;
            else (void)queue_remove(hv->io_queues[request->device], request);
            request->used = false;
            request->active = false;
        }
    }
}

void io_print_status(const Hv *hv) {
    if (hv == NULL) return;
    for (int d = 0; d < DEV_COUNT; ++d) {
        const IoDevice *device = &hv->devices[d];
        unsigned long pending = 0UL;
        const char *active_name = "none";
        if (device->active_request >= 0) {
            const IoRequest *r = &hv->requests[device->active_request];
            active_name = r->vm_id >= 0 ? hv->vms[r->vm_id].name : "-";
        }
        for (size_t i = 0; i < MAX_IO_REQUESTS; ++i) {
            if (hv->requests[i].used && hv->requests[i].device == (DeviceType)d && !hv->requests[i].active) pending++;
        }
        (void)printf("%-7s ativo=%s pendentes=%lu atendidas=%llu espera_media=%.2f",
            device_name((DeviceType)d), active_name, pending,
            (unsigned long long)device->requests_served,
            device->requests_served == 0U ? 0.0 : (double)device->wait_total / (double)device->requests_served);
        if (d == DEV_DISK) (void)printf(" cabeca=%d deslocamento=%llu", device->head, (unsigned long long)device->displacement_total);
        (void)printf("\n");
    }
}

void io_print_report(const Hv *hv) {
    const IoDevice *disk;
    if (hv == NULL) return;
    disk = &hv->devices[DEV_DISK];
    (void)printf("Disco: requisicoes=%llu deslocamento_total=%llu cilindros espera_media=%.2f ticks\n",
        (unsigned long long)disk->requests_served,
        (unsigned long long)disk->displacement_total,
        disk->requests_served == 0U ? 0.0 : (double)disk->wait_total / (double)disk->requests_served);
    for (int d = 0; d < DEV_COUNT; ++d) {
        const IoDevice *dev = &hv->devices[d];
        (void)printf("E/S %-7s: requisicoes=%llu espera_media=%.2f ticks\n",
            device_name((DeviceType)d), (unsigned long long)dev->requests_served,
            dev->requests_served == 0U ? 0.0 : (double)dev->wait_total / (double)dev->requests_served);
    }
}
