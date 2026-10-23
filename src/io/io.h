#ifndef MINIHV_IO_H
#define MINIHV_IO_H
#include "../common.h"
bool io_init(Hv *hv);
void io_set_interrupt_handler(Hv *hv, HvIoInterruptHandler handler, void *context);
void io_destroy(Hv *hv);
bool io_submit(Hv *hv, DeviceType device, int vm_id, int cylinder,
               uint32_t words, int32_t value, size_t page);
void io_tick(Hv *hv);
void io_cancel_vm(Hv *hv, int vm_id);
void io_print_status(const Hv *hv);
void io_print_report(const Hv *hv);
#endif
