/*******************************************************************************
 *   Taras Shchybovyk
 *   (c) 2018 Taras Shchybovyk
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ********************************************************************************/

#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdbool.h>
#include <stdint.h>

bool b58enc(uint8_t *data, uint32_t binsz, char *b58, uint32_t *b58sz);

void array_hexstr(char *strbuf, const void *bin, unsigned int len);

char *i64toa(int64_t i, char b[]);
char *ui64toa(uint64_t i, char b[]);

bool tlvTryDecode(uint8_t *buffer, uint32_t bufferLength, uint32_t *fieldLenght, bool *valid);

unsigned char check_canonical(uint8_t *rs);

int ecdsa_der_to_sig(const uint8_t *der, uint8_t *sig);

void rng_rfc6979(unsigned char *rnd,
                 unsigned char *h1,
                 unsigned char *x,
                 unsigned int x_len,
                 const unsigned char *q,
                 unsigned int q_len,
                 unsigned char *V,
                 unsigned char *K);

#endif
