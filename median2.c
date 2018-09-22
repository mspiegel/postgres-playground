#include <postgres.h>
#include <fmgr.h>
#include <catalog/namespace.h>
#include <nodes/value.h>
#include <utils/lsyscache.h>

#include "median.h"

void		median2_swap(Datum * a, Datum * b);
bool		boolean_func(Datum left, Datum right, FmgrInfo * finfo, Oid fncollation);
int32		median2_partition(Datum arr[], int32 left, int32 right, FmgrInfo * finfo, Oid fncollation);
Datum		quickselect(Datum arr[], int32 left, int32 right, int32 k, FmgrInfo * finfo, Oid fncollation);

PG_FUNCTION_INFO_V1(median2_transfn);
PG_FUNCTION_INFO_V1(median2_finalfn);

/*
 * Median state transfer function.
 *
 * This function is called for every value in the set that we are calculating
 * the median for. On first call, the aggregate state, if any, needs to be
 * initialized.
 */
Datum
median2_transfn(PG_FUNCTION_ARGS) {
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

void
median2_swap(Datum * a, Datum * b)
{

	Datum		t = *a;
	*a = *b;
	*b = t;
}

bool
boolean_func(Datum left, Datum right, FmgrInfo * finfo, Oid fncollation)
{
	return DatumGetBool(FunctionCall2Coll(finfo, fncollation, left, right));
}

int32
median2_partition(Datum arr[], int32 left, int32 right, FmgrInfo * finfo, Oid fncollation)
{
	Datum		val = arr[right];
	int32		i = left;
	for (int32 j = left; j <= right - 1; j++) {
		if (boolean_func(arr[j], val, finfo, fncollation)) {
			median2_swap(&arr[i], &arr[j]);
			i++;
		}
	}
	median2_swap(&arr[i], &arr[right]);
	return i;
}

Datum
quickselect(Datum arr[], int32 left, int32 right, int32 k, FmgrInfo * finfo, Oid fncollation)
{
	if (k > 0 && k <= right - left + 1) {

		int32		index = median2_partition(arr, left, right, finfo, fncollation);

		if (index - left == k - 1)
			return arr[index];

		if (index - left > k - 1)
			return quickselect(arr, left, index - 1, k, finfo, fncollation);

		return quickselect(arr, index + 1, right,
				   k - index + left - 1, finfo, fncollation);
	}

	elog(ERROR, "kth element selection is out of bounds");
	return 0;
}

/*
 * Median final function.
 *
 * This function is called after all values in the median set has been
 * processed by the state transfer function. It should perform any necessary
 * post processing and clean up any temporary state.
 */
Datum
median2_finalfn(PG_FUNCTION_ARGS) {
	MemoryContext	agg_context;
	MedianState    *median_state;
	FmgrInfo	lte_finfo;
	Oid		type_oid, lte_op, lte_regproc;
	Datum		median;

	if (!AggCheckCallContext(fcinfo, &agg_context))
		elog(ERROR, "median_finalfn called in non-aggregate context");

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	median_state = (MedianState *) PG_GETARG_POINTER(0);
	type_oid = median_state->type_oid;

	if (!OidIsValid(type_oid))
		elog(ERROR, "could not determine the type of the elements");

	lte_op = OpernameGetOprid(list_make1(makeString("<=")), type_oid, type_oid);
	if (!OidIsValid(lte_op))
		elog(ERROR, "could not find a <= operator for type %d", type_oid);
	lte_regproc = get_opcode(lte_op);
	if (!OidIsValid(lte_regproc))
		elog(ERROR, "could not find the procedure for the <= operator for type %d", type_oid);
	fmgr_info_cxt(lte_regproc, &lte_finfo, fcinfo->flinfo->fn_mcxt);

//TODO:	return the arithmetic mean of neighbors when even number of elements
		median = quickselect(median_state->data, 0, median_state->len, median_state->len / 2, &lte_finfo, fcinfo->fncollation);

	PG_RETURN_DATUM(median);
}
