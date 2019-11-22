function assertTrue(pred) { assert(pred); }
function assertFalse(pred) { assert(!pred); }

assertTrue(Number.isFinite(0));
assertTrue(Number.isFinite(Number.MIN_VALUE));
assertTrue(Number.isFinite(Number.MAX_VALUE));
assertFalse(Number.isFinite(Number.NaN));
assertFalse(Number.isFinite(Number.POSITIVE_INFINITY));
assertFalse(Number.isFinite(Number.NEGATIVE_INFINITY));
assertFalse(Number.isFinite(new Number(0)));
assertFalse(Number.isFinite(1 / 0));
assertFalse(Number.isFinite(-1 / 0));
assertFalse(Number.isFinite({}));
assertFalse(Number.isFinite([]));
assertFalse(Number.isFinite('s'));
assertFalse(Number.isFinite(null));
assertFalse(Number.isFinite(undefined));

assertFalse(Number.isNaN(0));
assertFalse(Number.isNaN(Number.MIN_VALUE));
assertFalse(Number.isNaN(Number.MAX_VALUE));
assertTrue(Number.isNaN(Number.NaN));
assertFalse(Number.isNaN(Number.POSITIVE_INFINITY));
assertFalse(Number.isNaN(Number.NEGATIVE_INFINITY));
assertFalse(Number.isNaN(new Number(0)));
assertFalse(Number.isNaN(1 / 0));
assertFalse(Number.isNaN(-1 / 0));
assertFalse(Number.isNaN({}));
assertFalse(Number.isNaN([]));
assertFalse(Number.isNaN('s'));
assertFalse(Number.isNaN(null));
assertFalse(Number.isNaN(undefined));

assertFalse(Number.isInteger({}));
assertFalse(Number.isInteger([]));
assertFalse(Number.isInteger('s'));
assertFalse(Number.isInteger(null));
assertFalse(Number.isInteger(undefined));
assertFalse(Number.isInteger(new Number(2)));
assertTrue(Number.isInteger(0));
assertFalse(Number.isInteger(Number.MIN_VALUE));
assertTrue(Number.isInteger(Number.MAX_VALUE));
assertFalse(Number.isInteger(Number.NaN));
assertFalse(Number.isInteger(Number.POSITIVE_INFINITY));
assertFalse(Number.isInteger(Number.NEGATIVE_INFINITY));
assertFalse(Number.isInteger(1 / 0));
assertFalse(Number.isInteger(-1 / 0));

assertFalse(Number.isSafeInteger({}));
assertFalse(Number.isSafeInteger([]));
assertFalse(Number.isSafeInteger('s'));
assertFalse(Number.isSafeInteger(null));
assertFalse(Number.isSafeInteger(undefined));
assertFalse(Number.isSafeInteger(new Number(2)));
assertTrue(Number.isSafeInteger(0));
assertFalse(Number.isSafeInteger(Number.MIN_VALUE));
assertFalse(Number.isSafeInteger(Number.MAX_VALUE));
assertFalse(Number.isSafeInteger(Number.NaN));
assertFalse(Number.isSafeInteger(Number.POSITIVE_INFINITY));
assertFalse(Number.isSafeInteger(Number.NEGATIVE_INFINITY));
assertFalse(Number.isSafeInteger(1 / 0));
assertFalse(Number.isSafeInteger(-1 / 0));

var near_upper = Math.pow(2, 52);
assertTrue(Number.isSafeInteger(near_upper));
assertFalse(Number.isSafeInteger(2 * near_upper));
assertTrue(Number.isSafeInteger(2 * near_upper - 1));
assertTrue(Number.isSafeInteger(2 * near_upper - 2));
assertFalse(Number.isSafeInteger(2 * near_upper + 1));
assertFalse(Number.isSafeInteger(2 * near_upper + 2));
assertFalse(Number.isSafeInteger(2 * near_upper + 7));

var near_lower = -near_upper;
assertTrue(Number.isSafeInteger(near_lower));
assertFalse(Number.isSafeInteger(2 * near_lower));
assertTrue(Number.isSafeInteger(2 * near_lower + 1));
assertTrue(Number.isSafeInteger(2 * near_lower + 2));
assertFalse(Number.isSafeInteger(2 * near_lower - 1));
assertFalse(Number.isSafeInteger(2 * near_lower - 2));
assertFalse(Number.isSafeInteger(2 * near_lower - 7));
