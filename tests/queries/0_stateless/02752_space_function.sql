SELECT 'const times, uint';
SELECT space(3::UInt8);
SELECT space(3::UInt16);
SELECT space(3::UInt32);
SELECT space(3::UInt64);
SELECT 'const times, int';
SELECT space(3::Int8);
SELECT space(3::Int16);
SELECT space(3::Int32);
SELECT space(3::Int64);

SELECT 'negative const times';
SELECT space(-3::Int8);
SELECT space(-3::Int16);
SELECT space(-3::Int32);
SELECT space(-3::Int64);

SELECT 'negative tests';
SELECT space('abc'); -- { serverError ILLEGAL_TYPE_OF_ARGUMENT }
SELECT space(['abc']); -- { serverError ILLEGAL_TYPE_OF_ARGUMENT }
SELECT space(('abc')); -- { serverError ILLEGAL_TYPE_OF_ARGUMENT }
SELECT space(NULL);

DROP TABLE IF EXISTS defaults;
CREATE TABLE defaults
(
    u8 UInt8,
    u16 UInt16,
    u32 UInt32,
    u64 UInt64,
    i8 Int8,
    i16 Int16,
    i32 Int32,
    i64 Int64
) ENGINE = Memory();

INSERT INTO defaults values (3, 12, 4, 56, 3, 12, -4, 56) (2, 10, 21, 20, 2, 10, -21, 20) (1, 4, 9, 5, 1, 4, -9, 5) (0, 5, 7, 7, 0, 5, -7, 7);

SELECT 'non-const times, uint';
SELECT space(u8), length(space(u8)) FROM defaults;
SELECT space(u16), length(space(u16)) FROM defaults;
SELECT space(u32), length(space(u32)) from defaults;
SELECT space(u64), length(space(u64)) FROM defaults;
SELECT 'non-const times, int';
SELECT space(i8), length(space(i8)) FROM defaults;
SELECT space(i16), length(space(i16)) FROM defaults;
SELECT space(i32), length(space(i32)) from defaults;
SELECT space(i64), length(space(i64)) FROM defaults;

DROP TABLE defaults;
