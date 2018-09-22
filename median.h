#ifndef TIMESCALEDB_MEDIAN_H
#define TIMESCALEDB_MEDIAN_H

#include <postgres.h>

typedef struct MedianState
{
    int32 cap;
    Oid type_oid;
    int32 len;
    Datum data[FLEXIBLE_ARRAY_MEMBER];
} MedianState;

#define MEDIAN_STATE_HEADER (sizeof(MedianState))
#define MEDIAN_STATE_CAPACITY(state) ((VARSIZE(state) - MEDIAN_STATE_HEADER ) / sizeof(Datum))

#endif /* TIMESCALEDB_MEDIAN_H */