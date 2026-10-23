#include "cli.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void trim_command(char *text) {
    char *start;
    size_t length;
    if (text == NULL) return;
    start = text;
    while (*start != '\0' && isspace((unsigned char)*start)) start++;
    if (start != text) memmove(text, start, strlen(start) + 1U);
    length = strlen(text);
    while (length > 0U && isspace((unsigned char)text[length - 1U])) text[--length] = '\0';
}

void cli_print_help(void) {
    (void)printf("Comandos MiniHV:\n");
    (void)printf("  step                 avanca um tick\n");
    (void)printf("  run [n]              executa n ticks; sem n, executa ate o fim\n");
    (void)printf("  ps                   lista VMs e estados\n");
    (void)printf("  mem                  mostra o uso da memoria fisica\n");
    (void)printf("  io                   mostra filas e metricas dos dispositivos\n");
    (void)printf("  stats                mostra estatisticas parciais\n");
    (void)printf("  pause <vm>           pausa VM em READY ou RUNNING\n");
    (void)printf("  resume <vm>          retoma VM pausada\n");
    (void)printf("  kill <vm>            encerra VM\n");
    (void)printf("  help                 mostra esta ajuda\n");
    (void)printf("  quit                 encerra o modo interativo\n");
    (void)printf("VM pode ser identificada pelo nome ou id numerico.\n");
}

static bool parse_count(const char *text, uint64_t *count) {
    char *end = NULL;
    unsigned long long n;
    if (text == NULL || *text == '\0' || *text == '-') return false;
    n = strtoull(text, &end, 10);
    if (end == text || *end != '\0') return false;
    *count = (uint64_t)n;
    return true;
}

static void run_ticks(Hv *hv, uint64_t count) {
    uint64_t done = 0U;
    while (done < count && hv_step(hv)) done++;
    (void)printf("avancados %llu tick(s); relogio=%llu\n",
        (unsigned long long)done, (unsigned long long)hv_current_tick(hv));
    if (hv_current_tick(hv) >= hv_max_ticks(hv) || hv_all_done(hv)) {
        hv_run(hv);
        hv_report(hv);
    }
}

void cli_run_interactive(Hv *hv) {
    char line[256];
    bool keep_running = true;
    cli_print_help();
    while (keep_running) {
        char *command;
        char *arg1;
        char *arg2;
        (void)printf("minihv> ");
        if (fgets(line, sizeof(line), stdin) == NULL) break;
        trim_command(line);
        command = strtok(line, " \t\r\n");
        if (command == NULL) continue;
        arg1 = strtok(NULL, " \t\r\n");
        arg2 = strtok(NULL, " \t\r\n");
        if (strcmp(command, "help") == 0) cli_print_help();
        else if (strcmp(command, "step") == 0 && arg1 == NULL) run_ticks(hv, 1U);
        else if (strcmp(command, "run") == 0) {
            if (arg2 != NULL) { (void)printf("Uso: run [n]\n"); cli_print_help(); continue; }
            if (arg1 == NULL) { hv_run(hv); hv_report(hv); }
            else {
                uint64_t count;
                if (!parse_count(arg1, &count)) { (void)printf("Numero de ticks invalido.\n"); cli_print_help(); }
                else run_ticks(hv, count);
            }
        }
        else if (strcmp(command, "ps") == 0 && arg1 == NULL) hv_print_ps(hv);
        else if (strcmp(command, "mem") == 0 && arg1 == NULL) hv_print_mem(hv);
        else if (strcmp(command, "io") == 0 && arg1 == NULL) hv_print_io(hv);
        else if (strcmp(command, "stats") == 0 && arg1 == NULL) hv_print_stats(hv);
        else if (strcmp(command, "pause") == 0 && arg1 != NULL && arg2 == NULL) (void)hv_pause_vm(hv, arg1);
        else if (strcmp(command, "resume") == 0 && arg1 != NULL && arg2 == NULL) (void)hv_resume_vm(hv, arg1);
        else if (strcmp(command, "kill") == 0 && arg1 != NULL && arg2 == NULL) (void)hv_kill_vm(hv, arg1);
        else if (strcmp(command, "quit") == 0 && arg1 == NULL) { keep_running = false; }
        else { (void)printf("Comando invalido.\n"); cli_print_help(); }
    }
    if (!hv_report_was_printed(hv)) hv_report(hv);
}
