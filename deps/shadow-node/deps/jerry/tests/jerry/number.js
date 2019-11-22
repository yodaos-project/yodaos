function assertThrow(pred) {
  var isThrow = true;
  try {
    eval(pred);
  } catch (e) {
    isThrow = false;
  }
  assert(!isThrow);
}

function assertNotThrow(pred) {
  var isNotThrow = true;
  try {
    eval(pred);
  } catch (e) {
    isNotThrow = false;
  }
  assert(isNotThrow);
}

assert(0xbf == 191);
assert(0Xbf == 191);
assert(0xbf - 10 == 181);
assertThrow('0xbl');
assertNotThrow('0xbf;');
assertNotThrow('0xbf;');
assertNotThrow('i = 0xbf');

assert(0o37 == 31);
assert(0O37 == 31);
assert(0o37 - 10 == 21);
assertThrow('0o38');
assertThrow('0O38');
assertNotThrow('0o37;');
assertNotThrow('0O37;');
assertNotThrow('i = 0o37');

assert(0b101 == 5);
assert(0B101 == 5);
assert(0b101 - 2 == 3);
assertThrow('0b12');
assertThrow('0B12');
assertNotThrow('0b101;');
assertNotThrow('0B101;');
assertNotThrow('i = 0b101');
