#pragma once

#include <stdint.h>

#ifdef  __cplusplus
extern "C" {
#endif

extern int __b64_ntop(uint8_t const *src, size_t srclength, char *target, size_t targsize);
extern int __b64_pton(char const *src, uint8_t *target, size_t targsize);

#ifdef  __cplusplus
}
#endif