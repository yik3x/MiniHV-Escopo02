#include "config.h"
#include "../util/util.h"

typedef enum { SEC_NONE, SEC_HOST, SEC_SCHED, SEC_MEM, SEC_IO, SEC_LOG, SEC_EXT, SEC_VM } Section;

static void copy_text(char *dest, size_t capacity, const char *value) {
    if (capacity == 0U) return;
    (void)snprintf(dest, capacity, "%s", value);
}

void config_defaults(Config *cfg) {
    if (cfg == NULL) return;
    memset(cfg, 0, sizeof(*cfg));
    cfg->pcpus = 1;
    cfg->max_vms = 4;
    cfg->max_ticks = 5000;
    cfg->seed = 2026U;
    copy_text(cfg->policy, sizeof(cfg->policy), "SJF");
    cfg->quantum = 4;
    copy_text(cfg->mem_model, sizeof(cfg->mem_model), "PAG");
    cfg->phys_words = 4096U;
    cfg->page_words = 64U;
    copy_text(cfg->replacement, sizeof(cfg->replacement), "LRU");
    copy_text(cfg->repl_scope, sizeof(cfg->repl_scope), "GLOBAL");
    cfg->pf_penalty = 3;
    copy_text(cfg->disk_algorithm, sizeof(cfg->disk_algorithm), "C_LOOK");
    cfg->disk_cylinders = 200;
    cfg->disk_head = 50;
    cfg->seek_rate = 25;
    copy_text(cfg->log_format, sizeof(cfg->log_format), "CSV");
    copy_text(cfg->log_level, sizeof(cfg->log_level), "INFO");
    copy_text(cfg->log_file, sizeof(cfg->log_file), "out/minihv.csv");
    cfg->ext_enabled = true;
    cfg->watchdog_limit = 200U;
}

static bool parse_bool_value(const char *value, bool *out) {
    if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0) { *out = true; return true; }
    if (strcmp(value, "false") == 0 || strcmp(value, "0") == 0) { *out = false; return true; }
    return false;
}

static bool parse_i32_checked(const char *path, size_t line, const char *key,
                              const char *value, int32_t *dest) {
    if (util_parse_i32(value, dest)) return true;
    (void)fprintf(stderr, "%s:%lu: chave '%s' deve ser um inteiro valido\n",
                  path, (unsigned long)line, key);
    return false;
}

static bool parse_u64_checked(const char *path, size_t line, const char *key,
                              const char *value, uint64_t *dest) {
    if (util_parse_u64(value, dest)) return true;
    (void)fprintf(stderr, "%s:%lu: chave '%s' deve ser um inteiro nao negativo\n",
                  path, (unsigned long)line, key);
    return false;
}

static bool parse_size_checked(const char *path, size_t line, const char *key,
                               const char *value, size_t *dest) {
    if (util_parse_size(value, dest)) return true;
    (void)fprintf(stderr, "%s:%lu: chave '%s' deve ser um tamanho valido\n",
                  path, (unsigned long)line, key);
    return false;
}

static bool unknown_key(const char *path, size_t line, const char *key) {
    (void)fprintf(stderr, "%s:%lu: chave desconhecida '%s'\n", path, (unsigned long)line, key);
    return false;
}

static bool read_key(Config *cfg, Section section, const char *key, const char *value,
                     const char *path, size_t line, VmConfig *vm) {
    int32_t i32;
    uint64_t u64;
    size_t size_value;
    bool bool_value;
    if (section == SEC_HOST) {
        if (strcmp(key, "pcpus") == 0) {
            if (!parse_i32_checked(path, line, key, value, &i32)) return false;
            if (i32 < 1 || i32 > 32) { (void)fprintf(stderr, "%s:%lu: pcpus deve estar entre 1 e 32\n", path, (unsigned long)line); return false; }
            cfg->pcpus = i32; return true;
        }
        if (strcmp(key, "max_vms") == 0) {
            if (!parse_i32_checked(path, line, key, value, &i32)) return false;
            if (i32 < 1 || i32 > MAX_VMS) { (void)fprintf(stderr, "%s:%lu: max_vms deve estar entre 1 e %d\n", path, (unsigned long)line, MAX_VMS); return false; }
            cfg->max_vms = i32; return true;
        }
        if (strcmp(key, "max_ticks") == 0) {
            if (!parse_u64_checked(path, line, key, value, &u64)) return false;
            if (u64 == 0U) { (void)fprintf(stderr, "%s:%lu: max_ticks deve ser maior que zero\n", path, (unsigned long)line); return false; }
            cfg->max_ticks = u64; return true;
        }
        if (strcmp(key, "seed") == 0) {
            if (!parse_u64_checked(path, line, key, value, &u64)) return false;
            if (u64 > UINT32_MAX) { (void)fprintf(stderr, "%s:%lu: seed fora da faixa uint32\n", path, (unsigned long)line); return false; }
            cfg->seed = (uint32_t)u64; return true;
        }
    } else if (section == SEC_SCHED) {
        if (strcmp(key, "policy") == 0) { copy_text(cfg->policy, sizeof(cfg->policy), value); return true; }
        if (strcmp(key, "quantum") == 0) {
            if (!parse_i32_checked(path, line, key, value, &i32)) return false;
            if (i32 < 1 || i32 > 50) { (void)fprintf(stderr, "%s:%lu: quantum deve estar entre 1 e 50\n", path, (unsigned long)line); return false; }
            cfg->quantum = i32; return true;
        }
    } else if (section == SEC_MEM) {
        if (strcmp(key, "model") == 0) { copy_text(cfg->mem_model, sizeof(cfg->mem_model), value); return true; }
        if (strcmp(key, "phys_words") == 0) {
            if (!parse_size_checked(path, line, key, value, &size_value)) return false;
            if (size_value == 0U || size_value > 10000000U) { (void)fprintf(stderr, "%s:%lu: phys_words fora da faixa permitida\n", path, (unsigned long)line); return false; }
            cfg->phys_words = size_value; return true;
        }
        if (strcmp(key, "page_words") == 0) {
            if (!parse_size_checked(path, line, key, value, &size_value)) return false;
            if (size_value == 0U || size_value > 65536U) { (void)fprintf(stderr, "%s:%lu: page_words fora da faixa permitida\n", path, (unsigned long)line); return false; }
            cfg->page_words = size_value; return true;
        }
        if (strcmp(key, "replacement") == 0) { copy_text(cfg->replacement, sizeof(cfg->replacement), value); return true; }
        if (strcmp(key, "repl_scope") == 0) { copy_text(cfg->repl_scope, sizeof(cfg->repl_scope), value); return true; }
        if (strcmp(key, "pf_penalty") == 0) {
            if (!parse_i32_checked(path, line, key, value, &i32)) return false;
            if (i32 < 1 || i32 > 100000) { (void)fprintf(stderr, "%s:%lu: pf_penalty deve ser positivo\n", path, (unsigned long)line); return false; }
            cfg->pf_penalty = i32; return true;
        }
    } else if (section == SEC_IO) {
        if (strcmp(key, "disk_algorithm") == 0) { copy_text(cfg->disk_algorithm, sizeof(cfg->disk_algorithm), value); return true; }
        if (strcmp(key, "disk_cylinders") == 0) {
            if (!parse_i32_checked(path, line, key, value, &i32)) return false;
            if (i32 < 1 || i32 > 1000000) { (void)fprintf(stderr, "%s:%lu: disk_cylinders fora da faixa permitida\n", path, (unsigned long)line); return false; }
            cfg->disk_cylinders = i32; return true;
        }
        if (strcmp(key, "disk_head") == 0) {
            if (!parse_i32_checked(path, line, key, value, &i32)) return false;
            cfg->disk_head = i32; return true;
        }
        if (strcmp(key, "seek_rate") == 0) {
            if (!parse_i32_checked(path, line, key, value, &i32)) return false;
            if (i32 < 1) { (void)fprintf(stderr, "%s:%lu: seek_rate deve ser positivo\n", path, (unsigned long)line); return false; }
            cfg->seek_rate = i32; return true;
        }
    } else if (section == SEC_LOG) {
        if (strcmp(key, "format") == 0) { copy_text(cfg->log_format, sizeof(cfg->log_format), value); return true; }
        if (strcmp(key, "level") == 0) { copy_text(cfg->log_level, sizeof(cfg->log_level), value); return true; }
        if (strcmp(key, "file") == 0) { copy_text(cfg->log_file, sizeof(cfg->log_file), value); return true; }
    } else if (section == SEC_EXT) {
        if (strcmp(key, "enabled") == 0) {
            if (!parse_bool_value(value, &bool_value)) { (void)fprintf(stderr, "%s:%lu: ext.enabled deve ser true ou false\n", path, (unsigned long)line); return false; }
            cfg->ext_enabled = bool_value; return true;
        }
        if (strcmp(key, "limit") == 0) {
            if (!parse_u64_checked(path, line, key, value, &u64)) return false;
            if (u64 == 0U || u64 > UINT64_MAX / 2U) { (void)fprintf(stderr, "%s:%lu: limit deve ser positivo\n", path, (unsigned long)line); return false; }
            cfg->watchdog_limit = u64; return true;
        }
    } else if (section == SEC_VM && vm != NULL) {
        if (strcmp(key, "name") == 0) { copy_text(vm->name, sizeof(vm->name), value); return true; }
        if (strcmp(key, "program") == 0) { copy_text(vm->program, sizeof(vm->program), value); return true; }
        if (strcmp(key, "arrival") == 0) { return parse_u64_checked(path, line, key, value, &vm->arrival); }
        if (strcmp(key, "priority") == 0) {
            if (!parse_i32_checked(path, line, key, value, &i32)) return false;
            if (i32 < 0 || i32 > 9) { (void)fprintf(stderr, "%s:%lu: priority deve estar entre 0 e 9\n", path, (unsigned long)line); return false; }
            vm->priority = i32; return true;
        }
        if (strcmp(key, "mem_words") == 0) {
            if (!parse_size_checked(path, line, key, value, &size_value)) return false;
            if (size_value == 0U || size_value > 10000000U) { (void)fprintf(stderr, "%s:%lu: mem_words fora da faixa permitida\n", path, (unsigned long)line); return false; }
            vm->mem_words = size_value; return true;
        }
    }
    return unknown_key(path, line, key);
}

bool config_validate_scope02(const Config *cfg) {
    if (cfg->pcpus != 1) { (void)fprintf(stderr, "Escopo 02 exige pcpus=1\n"); return false; }
    if (strcmp(cfg->policy, "SJF") != 0 && strcmp(cfg->policy, "FCFS") != 0) {
        (void)fprintf(stderr, "Escopo 02 suporta policy=SJF e a referencia FCFS\n"); return false;
    }
    if (strcmp(cfg->mem_model, "PAG") != 0 || strcmp(cfg->replacement, "LRU") != 0 || strcmp(cfg->repl_scope, "GLOBAL") != 0) {
        (void)fprintf(stderr, "Escopo 02 exige mem.model=PAG, replacement=LRU e repl_scope=GLOBAL\n"); return false;
    }
    if (cfg->page_words == 0U || cfg->phys_words < cfg->page_words || cfg->phys_words % cfg->page_words != 0U) {
        (void)fprintf(stderr, "Memoria fisica deve ser multipla positiva de page_words\n"); return false;
    }
    if (strcmp(cfg->disk_algorithm, "C_LOOK") != 0) { (void)fprintf(stderr, "Escopo 02 exige disk_algorithm=C_LOOK\n"); return false; }
    if (strcmp(cfg->log_format, "CSV") != 0) { (void)fprintf(stderr, "Escopo 02 exige log.format=CSV\n"); return false; }
    if (strcmp(cfg->log_level, "DEBUG") != 0 && strcmp(cfg->log_level, "INFO") != 0 &&
        strcmp(cfg->log_level, "WARN") != 0 && strcmp(cfg->log_level, "ERROR") != 0) {
        (void)fprintf(stderr, "log.level deve ser DEBUG, INFO, WARN ou ERROR\n"); return false;
    }
    if (cfg->disk_head < 0 || cfg->disk_head >= cfg->disk_cylinders) {
        (void)fprintf(stderr, "disk_head precisa estar entre 0 e disk_cylinders-1\n"); return false;
    }
    if (cfg->vm_count == 0U) { (void)fprintf(stderr, "A configuracao precisa ter ao menos uma secao [vm]\n"); return false; }
    for (size_t i = 0; i < cfg->vm_count; ++i) {
        if (cfg->vms[i].name[0] == '\0' || cfg->vms[i].program[0] == '\0') {
            (void)fprintf(stderr, "Secao [vm] %lu precisa de name e program\n", (unsigned long)i);
            return false;
        }
        if (strcmp(cfg->mem_model, "CONT") == 0 && cfg->vms[i].mem_words > cfg->phys_words) {
            (void)fprintf(stderr, "VM %s excede a memoria fisica no modelo CONT\n", cfg->vms[i].name); return false;
        }
        for (size_t j = 0; j < i; ++j) {
            if (strcmp(cfg->vms[i].name, cfg->vms[j].name) == 0) {
                (void)fprintf(stderr, "Nome de VM duplicado: %s\n", cfg->vms[i].name); return false;
            }
        }
    }
    return true;
}

bool config_load(Config *cfg, const char *path) {
    FILE *file;
    char line_buffer[MAX_LINE_LEN];
    size_t line_number = 0U;
    Section section = SEC_NONE;
    VmConfig *current_vm = NULL;
    if (cfg == NULL || path == NULL) return false;
    config_defaults(cfg);
    file = fopen(path, "r");
    if (file == NULL) { (void)fprintf(stderr, "Nao foi possivel abrir a configuracao: %s\n", path); return false; }
    while (fgets(line_buffer, sizeof(line_buffer), file) != NULL) {
        char *line = line_buffer;
        char *equal;
        char *value_end;
        line_number++;
        util_trim(line);
        if (*line == '\0' || *line == ';' || *line == '#') continue;
        if (*line == '[') {
            size_t len = strlen(line);
            if (len < 3U || line[len - 1U] != ']') {
                (void)fprintf(stderr, "%s:%lu: secao INI invalida\n", path, (unsigned long)line_number);
                (void)fclose(file); return false;
            }
            line[len - 1U] = '\0';
            util_trim(line + 1);
            if (strcmp(line + 1, "host") == 0) section = SEC_HOST;
            else if (strcmp(line + 1, "sched") == 0) section = SEC_SCHED;
            else if (strcmp(line + 1, "mem") == 0) section = SEC_MEM;
            else if (strcmp(line + 1, "io") == 0) section = SEC_IO;
            else if (strcmp(line + 1, "log") == 0) section = SEC_LOG;
            else if (strcmp(line + 1, "ext") == 0) section = SEC_EXT;
            else if (strcmp(line + 1, "vm") == 0) {
                if (cfg->vm_count >= MAX_VMS) {
                    (void)fprintf(stderr, "%s:%lu: numero maximo de VMs (%d) excedido\n", path, (unsigned long)line_number, MAX_VMS);
                    (void)fclose(file); return false;
                }
                section = SEC_VM;
                current_vm = &cfg->vms[cfg->vm_count++];
                memset(current_vm, 0, sizeof(*current_vm));
                current_vm->priority = 5;
                current_vm->mem_words = 256U;
                current_vm->cap = 100;
            } else {
                (void)fprintf(stderr, "%s:%lu: secao desconhecida '%s'\n", path, (unsigned long)line_number, line + 1);
                (void)fclose(file); return false;
            }
            continue;
        }
        equal = strchr(line, '=');
        if (equal == NULL || section == SEC_NONE) {
            (void)fprintf(stderr, "%s:%lu: esperava chave = valor dentro de uma secao\n", path, (unsigned long)line_number);
            (void)fclose(file); return false;
        }
        *equal = '\0';
        util_trim(line);
        value_end = equal + 1;
        while (*value_end != '\0') {
            if (*value_end == ';' || *value_end == '#') { *value_end = '\0'; break; }
            value_end++;
        }
        util_trim(equal + 1);
        if (*line == '\0' || *(equal + 1) == '\0') {
            (void)fprintf(stderr, "%s:%lu: chave ou valor vazio\n", path, (unsigned long)line_number);
            (void)fclose(file); return false;
        }
        if (!read_key(cfg, section, line, equal + 1, path, line_number, current_vm)) {
            (void)fclose(file); return false;
        }
    }
    if (ferror(file)) { (void)fprintf(stderr, "Erro ao ler configuracao: %s\n", path); (void)fclose(file); return false; }
    (void)fclose(file);
    if (!config_validate_scope02(cfg)) return false;
    return true;
}
