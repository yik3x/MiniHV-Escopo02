#include "prng.h"
#include <stddef.h>

void prng_seed(Prng *prng, uint32_t seed) {
    if (prng != NULL) prng->state = seed & UINT32_C(0x7fffffff);
}

uint32_t prng_next(Prng *prng) {
    if (prng == NULL) return 0U;
    prng->state = (uint32_t)(((uint64_t)UINT32_C(1103515245) * prng->state + UINT32_C(12345)) & UINT64_C(0x7fffffff));
    return prng->state;
}
