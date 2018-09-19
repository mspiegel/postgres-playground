#include <postgres.h>
#include <fmgr.h>
#include <catalog/namespace.h>
#include <nodes/value.h>
#include <utils/lsyscache.h>

#include "heap.h"

void heapify(Heap heap, int32 index);

Heap heap_alloc(int32 capacity, FunctionCallInfo fcinfo, char *opname)
{
    Heap heap;
    Oid cmp_op, cmp_regproc;

    Assert(opname[1] == '\0');

    heap.len = 0;
    heap.capacity = capacity;
    heap.type_oid = InvalidOid;
    heap.fcinfo = fcinfo;
    heap.proc = palloc(sizeof(FmgrInfo));
    heap.data = palloc(capacity * sizeof(Datum));

    cmp_op = OpernameGetOprid(list_make1(makeString(opname)), heap.type_oid, heap.type_oid);
	if (!OidIsValid(cmp_op))
    {
		elog(ERROR, "could not find a %s operator for type %d", opname, heap.type_oid);
    }
    cmp_regproc = get_opcode(cmp_op);
    if (!OidIsValid(cmp_regproc))
    {
		elog(ERROR, "could not find the procedure for the %s operator for type %d", opname, heap.type_oid);
    }
    fmgr_info_cxt(cmp_regproc, heap.proc, fcinfo->flinfo->fn_mcxt);

    return heap;
}

void heap_free(Heap heap)
{
    pfree(heap.proc);
    pfree(heap.data);
}

Heap heap_insert(Heap heap, PolyDatum value)
{
    int32 cap, len;

    cap = heap.capacity;
    len = heap.len;

    if (value.is_null)
        return heap;
    
    if (len == 0)
    {
        heap.data[0] = value.datum;
        heap.type_oid = value.type_oid;
        heap.len = 1;
        return heap;
    }

    if (heap.type_oid != value.type_oid)
    {
        elog(ERROR, "attempt to calculate median on values of different types");
        return heap;
    }

    if (cap == len)
    {
        int32 newcap = 2 * cap;
        Datum *newdata = palloc(newcap * sizeof(Datum));
        memcpy(newdata, heap.data, cap * sizeof(Datum));
        pfree(heap.data);
        heap.data = newdata;
        heap.capacity = newcap;
        cap = newcap;
    }

    heap.data[len] = value.datum;
    heapify(heap, len);
    heap.len++;

    return heap;
}

PolyDatum heap_top(Heap heap)
{
    PolyDatum res;

    res.type_oid = heap.type_oid;

    if (heap.len == 0)
    {
        res.is_null = true;
    }
    else
    {
        res.is_null = false;
        res.datum = heap.data[0];
    }

    return res;
}

void heapify(Heap heap, int32 idx)
{
    while (idx > 0)
    {
        Datum tmp;
        int32 parent_idx = (idx - 1) / 2;
        bool cmp = DatumGetBool(FunctionCall2Coll(heap.proc, heap.fcinfo->fncollation, heap.data[parent_idx], heap.data[idx]));
        if (cmp)
            return;
        tmp = heap.data[parent_idx];
        heap.data[parent_idx] = heap.data[idx];
        heap.data[idx] = tmp;
        idx = parent_idx;
    }
}