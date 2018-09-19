#ifndef TIMESCALEDB_POLYDATUM_H
#define TIMESCALEDB_POLYDATUM_H

#include <postgres.h>

/* A  PolyDatum represents a polymorphic datum */
/* From https://github.com/timescale/timescaledb/blob/2ec065b53823e50dd1ac1d7cf925ae5f90e293ea/src/agg_bookend.c#L29 */
typedef struct PolyDatum
{
	Oid			type_oid;
	bool		is_null;
	Datum		datum;
} PolyDatum;


#endif  /* TIMESCALEDB_POLYDATUM_H */