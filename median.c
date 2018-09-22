#include <postgres.h>
#include <fmgr.h>
#include <catalog/namespace.h>
#include <nodes/value.h>
#include <utils/lsyscache.h>

#include "median.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

typedef struct SortInfo {
	FmgrInfo	lt, eq;
	Oid		fncollation;
}		SortInfo;

int		cmp_datums(const void *p1, const void *p2, void *context);

PG_FUNCTION_INFO_V1(median_transfn);
PG_FUNCTION_INFO_V1(median_finalfn);

/*
 * Median state transfer function.
 *
 * This function is called for every value in the set that we are calculating
 * the median for. On first call, the aggregate state, if any, needs to be
 * initialized.
 */
Datum
median_transfn(PG_FUNCTION_ARGS) {
	MemoryContext	agg_context;
	MedianState    *median_state;
	bytea	       *state = (PG_ARGISNULL(0) ? NULL : PG_GETARG_BYTEA_P(0));
	Datum		val_datum = (PG_ARGISNULL(1) ? 0 : PG_GETARG_DATUM(1));

	if (!AggCheckCallContext(fcinfo, &agg_context))
		elog(ERROR, "median_transfn called in non-aggregate context");

	if (PG_ARGISNULL(1))
		PG_RETURN_BYTEA_P(state);

	if (state == NULL) {
		Size		arrsize = sizeof(Datum) * 8;
		state = MemoryContextAllocZero(agg_context, MEDIAN_STATE_HEADER + arrsize);
		SET_VARSIZE(state, MEDIAN_STATE_HEADER + arrsize);
		((MedianState *) state)->type_oid = get_fn_expr_argtype(fcinfo->flinfo, 1);
	}

	median_state = (MedianState *) state;

	if (median_state->len == MEDIAN_STATE_CAPACITY(state)) {
		Size		oldsize = sizeof(Datum) * median_state->len;
		Size		newsize = oldsize * 2;
		state = repalloc(state, MEDIAN_STATE_HEADER + newsize);
		SET_VARSIZE(state, MEDIAN_STATE_HEADER + newsize);
		median_state = (MedianState *) state;
	}

	median_state->data[median_state->len] = val_datum;
	median_state->len++;

	PG_RETURN_BYTEA_P(state);
}

int
cmp_datums(const void *p1, const void *p2, void *context)
{
//TODO:	replace calls to "<" and "=" with a single call to compare() function
		SortInfo * sort_info = (SortInfo *) context;
	Datum		left = *((Datum *) p1);
	Datum		right = *((Datum *) p2);
	bool		lt, eq;

	lt = DatumGetBool(FunctionCall2Coll(&sort_info->lt, sort_info->fncollation, left, right));
	if (lt)
		return -1;

	eq = DatumGetBool(FunctionCall2Coll(&sort_info->eq, sort_info->fncollation, left, right));
	if (eq)
		return 0;

	return 1;
}

/*
 * Median final function.
 *
 * This function is called after all values in the median set has been
 * processed by the state transfer function. It should perform any necessary
 * post processing and clean up any temporary state.
 */
Datum
median_finalfn(PG_FUNCTION_ARGS) {
	MemoryContext	agg_context;
	MedianState    *median_state;
	SortInfo	sort_info;
	Oid		type_oid, lt_op, lt_regproc, eq_op, eq_regproc;


	if (!AggCheckCallContext(fcinfo, &agg_context))
		elog(ERROR, "median_finalfn called in non-aggregate context");

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	median_state = (MedianState *) PG_GETARG_POINTER(0);
	type_oid = median_state->type_oid;

	if (!OidIsValid(type_oid))
		elog(ERROR, "could not determine the type of the elements");

	lt_op = OpernameGetOprid(list_make1(makeString("<")), type_oid, type_oid);
	if (!OidIsValid(lt_op))
		elog(ERROR, "could not find a < operator for type %d", type_oid);
	lt_regproc = get_opcode(lt_op);
	if (!OidIsValid(lt_regproc))
		elog(ERROR, "could not find the procedure for the < operator for type %d", type_oid);
	fmgr_info_cxt(lt_regproc, &sort_info.lt, fcinfo->flinfo->fn_mcxt);

	eq_op = OpernameGetOprid(list_make1(makeString("=")), type_oid, type_oid);
	if (!OidIsValid(eq_op))
		elog(ERROR, "could not find a = operator for type %d", type_oid);
	eq_regproc = get_opcode(eq_op);
	if (!OidIsValid(eq_regproc))
		elog(ERROR, "could not find the procedure for the = operator for type %d", type_oid);
	fmgr_info_cxt(eq_regproc, &sort_info.eq, fcinfo->flinfo->fn_mcxt);

	sort_info.fncollation = fcinfo->fncollation;
	qsort_r(median_state->data, median_state->len, sizeof(Datum), cmp_datums, &sort_info);

//TODO:	return the arithmetic mean of neighbors when even number of elements
		PG_RETURN_DATUM(median_state->data[median_state->len / 2]);
}
