CREATE OR REPLACE FUNCTION _median_transfn(state internal, val anyelement)
RETURNS internal
AS 'MODULE_PATHNAME', 'median_transfn'
LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION _median_finalfn(state internal, val anyelement)
RETURNS anyelement
AS 'MODULE_PATHNAME', 'median_finalfn'
LANGUAGE C IMMUTABLE;

DROP AGGREGATE IF EXISTS median (ANYELEMENT);
CREATE AGGREGATE median (ANYELEMENT)
(
    sfunc = _median_transfn,
    stype = internal,
    finalfunc = _median_finalfn,
    finalfunc_extra
);

CREATE OR REPLACE FUNCTION _median2_transfn(state internal, val anyelement)
RETURNS internal
AS 'MODULE_PATHNAME', 'median_transfn'
LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION _median2_finalfn(state internal, val anyelement)
RETURNS anyelement
AS 'MODULE_PATHNAME', 'median_finalfn'
LANGUAGE C IMMUTABLE;

DROP AGGREGATE IF EXISTS median2 (ANYELEMENT);
CREATE AGGREGATE median2 (ANYELEMENT)
(
    sfunc = _median2_transfn,
    stype = internal,
    finalfunc = _median2_finalfn,
    finalfunc_extra
);