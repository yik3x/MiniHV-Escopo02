#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "cli/cli.h"
#include "hv/hv.h"

static void print_usage(const char *program) {
    (void)printf("Uso: %s [-c arquivo] [-i] [-p SJF|FCFS]\n", program);
    (void)printf("MiniHV e um simulador de hypervisor para o escopo 02; nao virtualiza hardware real.\n");
}

int main(int argc, char **argv) {
    const char *config_path = "conf/hv.conf";
    const char *policy_override = NULL;
    bool interactive = false;
    Hv *hv;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-c") == 0) {
            if (i + 1 >= argc) { print_usage(argv[0]); return 2; }
            config_path = argv[++i];
        } else if (strcmp(argv[i], "-i") == 0) interactive = true;
        else if (strcmp(argv[i], "-p") == 0) {
            if (i + 1 >= argc) { print_usage(argv[0]); return 2; }
            policy_override = argv[++i];
            if (strcmp(policy_override, "SJF") != 0 && strcmp(policy_override, "FCFS") != 0) {
                (void)fprintf(stderr, "Politica invalida: use SJF ou FCFS.\n"); return 2;
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]); cli_print_help(); return 0;
        } else {
            (void)fprintf(stderr, "Argumento desconhecido: %s\n", argv[i]); print_usage(argv[0]); return 2;
        }
    }
    hv = hv_create(config_path);
    if (hv == NULL) return 1;
    if (policy_override != NULL) {
        if (!hv_set_policy(hv, policy_override)) { hv_destroy(hv); return 1; }
    }
    if (interactive) cli_run_interactive(hv);
    else { hv_run(hv); hv_report(hv); }
    hv_destroy(hv);
    return 0;
}
