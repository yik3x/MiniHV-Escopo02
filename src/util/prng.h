#ifndef MINIHV_PRNG_H
#define MINIHV_PRNG_H
#include <stdint.h>
typedef struct {
    uint32_t state;
} Prng;
void prng_seed(Prng *prng, uint32_t seed);
uint32_t prng_next(Prng *prng);
#endif
