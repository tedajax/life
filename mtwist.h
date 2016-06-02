#ifndef MTWIST_H
#define MTWIST_H

#include <stdint.h>

/// Defines random number generator that uses Mersenne Twister (MT19937-32/64)

#define MT_32_W 32
#define MT_32_N 624
#define MT_32_M 397
#define MT_32_R 31
#define MT_32_A 0x9908B0DF
#define MT_32_U 11
#define MT_32_D 0xFFFFFFFF
#define MT_32_S 7
#define MT_32_B 0x9D2C5680
#define MT_32_T 15
#define MT_32_C 0xEFC60000
#define MT_32_L 18
#define MT_32_F 0x6C078965

#define MT_32_MAX 4294967295 //(2^32)-1
#define MT_32_HIGH_BIT 0x80000000
#define MT_32_LOW_BITS 0x7fffffff
#define MT_32_FLOAT_CHUNKS 16777216

#define MT_64_W 64
#define MT_64_N 312
#define MT_64_M 156
#define MT_64_R 31
#define MT_64_A 0xB5026F5AA96619E9
#define MT_64_U 29
#define MT_64_D 0x5555555555555555
#define MT_64_S 17
#define MT_64_B 0x71D67FFFEDA60000
#define MT_64_T 37
#define MT_64_C 0xFFF7EEE000000000
#define MT_64_L 43
#define MT_64_F 0x5851F42D4C957F2D

#define MT_64_MAX 18446744073709551615 //(2^64)-1
#define MT_32_HIGH_BIT 0x8000000000000000
#define MT_32_LOW_BITS 0x7fffffffffffffff
#define MT_64_FLOAT_CHUNKS 16777216

#define MT_BIT_VAR(bits, var) MT_##bits##_##var

#define MT_VAR(var) MT_BIT_VAR(32, var)

typedef struct {
    int32_t mt[MT_VAR(N)];
    int index;
} MTGen32;

void mt_32_seed(MTGen32* self, int32_t seed);
int32_t mt_32_next(MTGen32* self, int32_t n);
float mt_32_nextf(MTGen32* selfn);
int32_t mt_32_range(MTGen32* self, int32_t min, int32_t max);
float mt_32_rangef(MTGen32* self, float min, float max);

int32_t _mt_32_next(MTGen32* self);
void _mt_32_twist(MTGen32* self);

void mt_32_seed(MTGen32* self, int32_t seed) {
    self->index = MT_VAR(N);
    self->mt[0] = seed;
    for (int i = 1; i < MT_VAR(N); ++i) {
        self->mt[i] = MT_VAR(F) * (self->mt[i - 1] ^ self->mt[i - 1] >> (MT_VAR(W) - 2)) + i;
    }
}

int32_t mt_32_next(MTGen32* self, int32_t n) {
    int32_t x;
    do {
        x = _mt_32_next(self);
    } while (x >= (MT_VAR(MAX) - MT_VAR(MAX) % n));
    return x % n;
}

float mt_32_nextf(MTGen32* self) {
    const int chunks = MT_VAR(FLOAT_CHUNKS);
    int32_t i = mt_32_next(self, chunks);
    return (float)i / (float)chunks;
}

int32_t mt_32_range(MTGen32* self, int32_t min, int32_t max) {
    int32_t n = max - min;
    return mt_32_next(self, n) + min;
}

float mt_32_rangef(MTGen32* self, float min, float max) {
    float d = max - min;
    return mt_32_nextf(self) * d + min;
}

int32_t _mt_32_next(MTGen32* self) {
    if (self->index >= MT_VAR(N)) {
        _mt_32_twist(self);
    }

    int32_t y = self->mt[self->index];

    y = y ^ y >> MT_VAR(U) & MT_VAR(D);
    y = y ^ y << MT_VAR(S) & MT_VAR(B);
    y = y ^ y << MT_VAR(T) & MT_VAR(C);
    y = y ^ y >> MT_VAR(L);

    self->index++;

    return y;
}

void _mt_32_twist(MTGen32* self) {
    for (int i = 0; i < MT_VAR(N); ++i) {
        int32_t y = ((self->mt[i] & MT_VAR(HIGH_BIT_MASK)) +
                     (self->mt[(i + 1) % MT_VAR(N)] & MT_VAR(LOW_BITS_MASK)));
        self->mt[i] = self->mt[(i + MT_VAR(M)) % MT_VAR(N)] ^ y >> 1;

        if (y % 2 != 0) {
            self->mt[i] = self->mt[i] ^ MT_VAR(A);
        }
    }
    self->index = 0;
}

#undef MT_VAR
#define MT_VAR(var) MT_BIT_VAR(64, var)

typedef struct
{
    int64_t mt[MT_VAR(N)];
    int index;
} MTGen64;

void mt_64_seed(MTGen64* self, int64_t seed);
int64_t mt_64_next(MTGen64* self, int64_t n);
float mt_64_nextf(MTGen64* selfn);
int64_t mt_64_range(MTGen64* self, int64_t min, int64_t max);
float mt_64_rangef(MTGen64* self, float min, float max);

int64_t _mt_64_next(MTGen64* self);
void _mt_64_twist(MTGen64* self);

void mt_64_seed(MTGen64* self, int64_t seed)
{
    self->index = MT_VAR(N);
    self->mt[0] = seed;
    for (int i = 1; i < MT_VAR(N); ++i) {
        self->mt[i] = MT_VAR(F) * (self->mt[i - 1] ^ self->mt[i - 1] >> (MT_VAR(W) - 2)) + i;
    }
}

int64_t mt_64_next(MTGen64* self, int64_t n)
{
    int64_t x;
    do {
        x = _mt_64_next(self);
    } while (x >= (MT_VAR(MAX) - MT_VAR(MAX) % n));
    return x % n;
}

float mt_64_nextf(MTGen64* self)
{
    const int chunks = MT_VAR(FLOAT_CHUNKS);
    int64_t i = mt_64_next(self, chunks);
    return (float)i / (float)chunks;
}

int64_t mt_64_range(MTGen64* self, int64_t min, int64_t max)
{
    int64_t n = max - min;
    return mt_64_next(self, n) + min;
}

float mt_64_rangef(MTGen64* self, float min, float max)
{
    float d = max - min;
    return mt_64_nextf(self) * d + min;
}

int64_t _mt_64_next(MTGen64* self)
{
    if (self->index >= MT_VAR(N)) {
        _mt_64_twist(self);
    }

    int64_t y = self->mt[self->index];

    y = y ^ y >> MT_VAR(U) & MT_VAR(D);
    y = y ^ y << MT_VAR(S) & MT_VAR(B);
    y = y ^ y << MT_VAR(T) & MT_VAR(C);
    y = y ^ y >> MT_VAR(L);

    self->index++;

    return y;
}

void _mt_64_twist(MTGen64* self)
{
    for (int i = 0; i < MT_VAR(N); ++i) {
        int64_t y = ((self->mt[i] & MT_VAR(HIGH_BIT_MASK)) +
            (self->mt[(i + 1) % MT_VAR(N)] & MT_VAR(LOW_BITS_MASK)));
        self->mt[i] = self->mt[(i + MT_VAR(M)) % MT_VAR(N)] ^ y >> 1;

        if (y % 2 != 0) {
            self->mt[i] = self->mt[i] ^ MT_VAR(A);
        }
    }
    self->index = 0;
}

#undef MT_VAR
#undef MT_BIT_VAR

#endif
