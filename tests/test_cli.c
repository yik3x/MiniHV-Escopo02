#include <stdio.h>
#include "../src/cli/cli.h"

int main(void) {
    cli_print_help();
    (void)puts("test_cli: OK");
    return 0;
}
