#include "log.h"
#include "../hv/events.h"

static int level_rank(const char *level) {
    if (strcmp(level, "DEBUG") == 0) return 0;
    if (strcmp(level, "INFO") == 0) return 1;
    if (strcmp(level, "WARN") == 0) return 2;
    if (strcmp(level, "ERROR") == 0) return 3;
    return 1;
}

static void csv_field(FILE *file, const char *value) {
    const char *p;
    bool quote = false;
    if (value == NULL) value = "-";
    for (p = value; *p != '\0'; ++p) {
        if (*p == ',' || *p == '"' || *p == '\n' || *p == '\r') { quote = true; break; }
    }
    if (quote) (void)fputc('"', file);
    for (p = value; *p != '\0'; ++p) {
        if (*p == '"') (void)fputc('"', file);
        (void)fputc((unsigned char)*p, file);
    }
    if (quote) (void)fputc('"', file);
}

static void log_observer(Hv *hv, const HvEvent *event, void *context) {
    FILE *file;
    (void)context;
    if (hv == NULL || event == NULL || hv->log_file == NULL || event->level == NULL) return;
    if (level_rank(event->level) < level_rank(hv->cfg.log_level)) return;
    file = hv->log_file;
    (void)fprintf(file, "%llu,", (unsigned long long)event->tick);
    csv_field(file, event->level); (void)fputc(',', file);
    csv_field(file, event->module); (void)fputc(',', file);
    csv_field(file, event->vm); (void)fputc(',', file);
    csv_field(file, event->event); (void)fputc(',', file);
    csv_field(file, event->detail);
    (void)fputc('\n', file);
    (void)fflush(file);
}

bool log_open(Hv *hv) {
    if (hv == NULL) return false;
    hv->log_file = fopen(hv->cfg.log_file, "w");
    if (hv->log_file == NULL) {
        (void)fprintf(stderr, "Nao foi possivel abrir o log CSV '%s'. Confira se a pasta existe.\n", hv->cfg.log_file);
        return false;
    }
    (void)fprintf(hv->log_file, "tick,level,module,vm,event,detail\n");
    if (!hv_observe(hv, log_observer, NULL)) {
        (void)fclose(hv->log_file);
        hv->log_file = NULL;
        (void)fprintf(stderr, "Nao foi possivel registrar o observador de log\n");
        return false;
    }
    return true;
}

void log_close(Hv *hv) {
    if (hv != NULL && hv->log_file != NULL) {
        (void)fclose(hv->log_file);
        hv->log_file = NULL;
    }
}

void log_event(Hv *hv, const char *level, const char *module, const char *vm,
               const char *event, const char *detail) {
    HvEvent emitted;
    if (hv == NULL || level == NULL) return;
    emitted.tick = hv->tick;
    emitted.level = level;
    emitted.module = module;
    emitted.vm = vm;
    emitted.event = event;
    emitted.detail = detail;
    emitted.vm_id = -1;
    emitted.instruction = false;
    emitted.io_instruction = false;
    hv_emit(hv, &emitted);
}
