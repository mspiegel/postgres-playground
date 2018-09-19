#ifndef TIMESCALEDB_HEAP_H
#define TIMESCALEDB_HEAP_H

#include "polydatum.h"

typedef struct Heap
{
    FunctionCallInfo fcinfo;
    FmgrInfo *proc;

    Oid type_oid;
    int32 capacity;
    int32 len;
    Datum *data;
} Heap;

Heap heap_alloc(int32 capacity, FunctionCallInfo fcinfo, char *opname);
void heap_free(Heap heap);
Heap heap_insert(Heap heap, PolyDatum value);
PolyDatum heap_top(Heap heap);

#endif /* TIMESCALEDB_HEAP_H */