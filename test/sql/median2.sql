CREATE TABLE intvals2(val int, color text);

-- Test empty table
SELECT median2(val) FROM intvals2;

-- Integers with odd number of values
INSERT INTO intvals2 VALUES
       (1, 'a'),
       (2, 'c'),
       (9, 'b'),
       (7, 'c'),
       (2, 'd'),
       (-3, 'd'),
       (2, 'e');

SELECT * FROM intvals2 ORDER BY val;
SELECT median2(val) FROM intvals2;

-- Integers with NULLs and even number of values
INSERT INTO intvals2 VALUES
       (99, 'a'),
       (NULL, 'a'),
       (NULL, 'e'),
       (NULL, 'b'),
       (7, 'c'),
       (0, 'd');

SELECT * FROM intvals2 ORDER BY val;
SELECT median2(val) FROM intvals2;

-- Text values
CREATE TABLE textvals2(val text, color int);

INSERT INTO textvals2 VALUES
       ('erik', 1),
       ('mat', 3),
       ('rob', 8),
       ('david', 9),
       ('lee', 2);

SELECT * FROM textvals2 ORDER BY val;
SELECT median2(val) FROM textvals2;

-- Test large table with timestamps
CREATE TABLE timestampvals2 (val timestamptz);

INSERT INTO timestampvals2(val)
SELECT TIMESTAMP 'epoch' + (i * INTERVAL '1 second')
FROM generate_series(0, 100000) as T(i);

SELECT median2(val) FROM timestampvals2;
