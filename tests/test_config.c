#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/config/config.h"

int main(void) {
    Config cfg;
    assert(config_load(&cfg, "conf/acceptance-ta01.conf"));
    assert(cfg.pcpus == 1);
    assert(cfg.max_vms == 4);
    assert(cfg.phys_words == 4096U);
    assert(cfg.page_words == 64U);
    assert(strcmp(cfg.policy, "SJF") == 0);
    assert(strcmp(cfg.replacement, "LRU") == 0);
    assert(strcmp(cfg.disk_algorithm, "C_LOOK") == 0);
    assert(cfg.vm_count == 1U);
    assert(!config_load(&cfg, "conf/invalid-key.conf"));
    (void)puts("test_config: OK");
    return 0;
}
