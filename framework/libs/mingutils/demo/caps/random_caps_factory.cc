#include <string.h>
#include <stdlib.h>
#include "random_caps_factory.h"
#include "demo_defs.h"

#define MEMBER_TYPE_INTEGER 0
#define MEMBER_TYPE_FLOAT 1
#define MEMBER_TYPE_LONG 2
#define MEMBER_TYPE_DOUBLE 3
#define MEMBER_TYPE_STRING 4
#define MEMBER_TYPE_BINARY 5
#define MEMBER_TYPE_OBJECT 6
#define MEMBER_TYPE_MAX 7

#define VISIBLE_CHAR_NUMBER 95
#define FIRST_VISIBLE_CHAR ' '

using namespace std;

RandomCapsFactory::RandomCapsFactory(bool capi) {
	if (capi)
		c_this_caps = caps_create();
	else
		this_caps = Caps::new_instance();
	use_c_api = capi;
}

RandomCapsFactory::~RandomCapsFactory() {
	if (use_c_api)
		caps_destroy(c_this_caps);
	size_t i;
	for (i = 0; i < sub_objects.size(); ++i) {
		delete sub_objects[i];
	}
}

void RandomCapsFactory::gen_integer() {
	if (member_types.size() >= MAX_MEMBERS)
		return;
	int v = rand();
	integers.push_back(v);
	member_types.push_back(MEMBER_TYPE_INTEGER);
	if (use_c_api)
		caps_write_integer(c_this_caps, (int32_t)v);
	else
		this_caps->write((int32_t)v);
}

void RandomCapsFactory::gen_float() {
	if (member_types.size() >= MAX_MEMBERS)
		return;
	float v = (float)rand() / (float)RAND_MAX;
	floats.push_back(v);
	member_types.push_back(MEMBER_TYPE_FLOAT);
	if (use_c_api)
		caps_write_float(c_this_caps, v);
	else
		this_caps->write(v);
}

void RandomCapsFactory::gen_long() {
	if (member_types.size() >= MAX_MEMBERS)
		return;
	int64_t v = rand();
	longs.push_back(v);
	member_types.push_back(MEMBER_TYPE_LONG);
	if (use_c_api)
		caps_write_long(c_this_caps, v);
	else
		this_caps->write(v);
}

void RandomCapsFactory::gen_double() {
	if (member_types.size() >= MAX_MEMBERS)
		return;
	double v = (double)rand() / (double)RAND_MAX;
	doubles.push_back(v);
	member_types.push_back(MEMBER_TYPE_DOUBLE);
	if (use_c_api)
		caps_write_double(c_this_caps, v);
	else
		this_caps->write(v);
}

static char random_visible_char() {
	return rand() % VISIBLE_CHAR_NUMBER + FIRST_VISIBLE_CHAR;
}

void RandomCapsFactory::gen_string() {
	if (member_types.size() >= MAX_MEMBERS)
		return;
	int32_t mod = MAX_STRING_LENGTH - MIN_STRING_LENGTH + 1;
	int32_t len = rand() % mod + MIN_STRING_LENGTH;
	int32_t i;
	strings.resize(strings.size() + 1);
	vector<string>::reverse_iterator it = strings.rbegin();
	(*it).resize(len);
	for (i = 0; i < len; ++i) {
		(*it)[i] = random_visible_char();
	}
	member_types.push_back(MEMBER_TYPE_STRING);
	if (use_c_api)
		caps_write_string(c_this_caps, (*it).c_str());
	else
		this_caps->write((*it).c_str());
}

static uint8_t random_byte() {
	return rand() % 0x100;
}

void RandomCapsFactory::gen_binary() {
	if (member_types.size() >= MAX_MEMBERS)
		return;
	int32_t mod = MAX_BINARY_LENGTH - MIN_BINARY_LENGTH + 1;
	int32_t len = rand() % mod + MIN_BINARY_LENGTH;
	int32_t i;
	binarys.resize(binarys.size() + 1);
	vector<vector<uint8_t> >::reverse_iterator it = binarys.rbegin();
	(*it).resize(len);
	for (i = 0; i < len; ++i) {
		(*it)[i] = random_byte();
	}
	member_types.push_back(MEMBER_TYPE_BINARY);
	if (use_c_api)
		caps_write_binary(c_this_caps, (*it).data(), len);
	else
		this_caps->write(*it);
}

void RandomCapsFactory::gen_object(uint32_t enable_sub_object) {
	if (member_types.size() >= MAX_MEMBERS)
		return;
	int32_t sub_member_size = rand() % MAX_MEMBERS;
	int32_t i;
	RandomCapsFactory* sub = new RandomCapsFactory(use_c_api);
	for (i = 0; i < sub_member_size; ++i) {
		sub->gen_random_member(enable_sub_object);
	}
	if (use_c_api)
		caps_write_object(c_this_caps, sub->caps());
	else
		this_caps->write(sub->caps_ptr());
	member_types.push_back(MEMBER_TYPE_OBJECT);
	sub_objects.push_back(sub);
}

void RandomCapsFactory::gen_random_member(uint32_t enable_sub_object) {
	int32_t mod = enable_sub_object ? MEMBER_TYPE_MAX : MEMBER_TYPE_MAX - 1;
	int32_t type = rand() % mod;

	switch (type) {
		case MEMBER_TYPE_INTEGER:
			gen_integer();
			break;
		case MEMBER_TYPE_FLOAT:
			gen_float();
			break;
		case MEMBER_TYPE_LONG:
			gen_long();
			break;
		case MEMBER_TYPE_DOUBLE:
			gen_double();
			break;
		case MEMBER_TYPE_STRING:
			gen_string();
			break;
		case MEMBER_TYPE_BINARY:
			gen_binary();
			break;
		case MEMBER_TYPE_OBJECT:
			gen_object(enable_sub_object - 1);
			break;
	}
}

int32_t RandomCapsFactory::check() {
	int32_t r;
	if (use_c_api)
		r = c_check();
	else
		r = cpp_check();
	return r;
}

int32_t RandomCapsFactory::c_check() {
	int32_t size = caps_serialize(c_this_caps, nullptr, 0);
	int8_t* buf = new int8_t[size];
	caps_t rcaps;
	int32_t i;
	bool b;

	// check: serialize, parse
	caps_serialize(c_this_caps, buf, size);
	if (caps_parse(buf, size, &rcaps) != CAPS_SUCCESS) {
		delete[] buf;
		return 1;
	}
	b = c_check(rcaps);
	caps_destroy(rcaps);
	delete[] buf;
	return b ? 0 : 1;
}

bool RandomCapsFactory::c_check(caps_t caps) {
	int32_t ci = 0;
	int32_t cl = 0;
	int32_t cf = 0;
	int32_t cd = 0;
	int32_t cS = 0;
	int32_t cB = 0;
	int32_t cO = 0;
	int32_t iv;
	float fv;
	int64_t lv;
	double dv;
	const char* Sv;
	const void* Bv;
	uint32_t Blen;
	caps_t Ov;
	int32_t i;
	int32_t r = CAPS_SUCCESS;

	for (i = 0; i < member_types.size(); ++i) {
		r = CAPS_SUCCESS;
		switch (member_types[i]) {
			case MEMBER_TYPE_INTEGER:
				r = caps_read_integer(caps, &iv);
				if (r != CAPS_SUCCESS || iv != integers[ci++]) {
					r = -10000;
				}
				break;
			case MEMBER_TYPE_FLOAT:
				r = caps_read_float(caps, &fv);
				if (r != CAPS_SUCCESS || fv != floats[cf++]) {
					r = -10001;
				}
				break;
			case MEMBER_TYPE_LONG:
				r = caps_read_long(caps, &lv);
				if (r != CAPS_SUCCESS || lv != longs[cl++]) {
					r = -10002;
				}
				break;
			case MEMBER_TYPE_DOUBLE:
				r = caps_read_double(caps, &dv);
				if (r != CAPS_SUCCESS || dv != doubles[cd++]) {
					r = -10003;
				}
				break;
			case MEMBER_TYPE_STRING:
				r = caps_read_string(caps, &Sv);
				if (r != CAPS_SUCCESS || strcmp(Sv, strings[cS++].c_str())) {
					r = -10004;
				}
				break;
			case MEMBER_TYPE_BINARY:
				r = caps_read_binary(caps, &Bv, &Blen);
				if (r != CAPS_SUCCESS) {
					r = -10005;
					break;
				}
				if (Blen != binarys[cB].size()
						|| memcmp(Bv, binarys[cB].data(), Blen)) {
					r = -10005;
				}
				++cB;
				break;
			case MEMBER_TYPE_OBJECT:
				r = caps_read_object(caps, &Ov);
				if (r != CAPS_SUCCESS) {
						r = -10006;
						break;
				}
				RandomCapsFactory* t = sub_objects[cO++];
				if (!t->c_check(Ov)) {
						r = -10006;
				}
				caps_destroy(Ov);
				break;
		}
		if (r != CAPS_SUCCESS) {
			printf("check failed, error = %d\n", r);
			break;
		}
	}
	return r == CAPS_SUCCESS;
}

int32_t RandomCapsFactory::cpp_check() {
	int32_t size = this_caps->serialize(nullptr, 0);
	int8_t* buf = new int8_t[size];
	shared_ptr<Caps> rcaps;
	int32_t i;
	bool b;

	// check: serialize, parse
	this_caps->serialize(buf, size);
	if (Caps::parse(buf, size, rcaps, false) != CAPS_SUCCESS) {
		delete[] buf;
		return 1;
	}
	b = cpp_check(rcaps);
	if (!b) {
		delete[] buf;
		return 1;
	}

	i = Caps::parse(buf, size, rcaps);
	delete[] buf;
	if (i != CAPS_SUCCESS)
		return 2;
	b = cpp_check(rcaps);
	return b ? 0 : 1;
}

static bool binary_equal(vector<uint8_t>& b1, vector<uint8_t>& b2) {
  if (b1.size() != b2.size())
    return false;
  return memcmp(b1.data(), b2.data(), b1.size()) == 0;
}

bool RandomCapsFactory::cpp_check(shared_ptr<Caps>& caps) {
	int32_t ci = 0;
	int32_t cl = 0;
	int32_t cf = 0;
	int32_t cd = 0;
	int32_t cS = 0;
	int32_t cB = 0;
	int32_t cO = 0;
	int32_t iv;
	float fv;
	int64_t lv;
	double dv;
	string Sv;
	vector<uint8_t> Bv;
	shared_ptr<Caps> Ov;
	int32_t i;
	int32_t r = CAPS_SUCCESS;

	for (i = 0; i < member_types.size(); ++i) {
		r = CAPS_SUCCESS;
		switch (member_types[i]) {
			case MEMBER_TYPE_INTEGER:
				r = caps->read(iv);
				if (r != CAPS_SUCCESS || iv != integers[ci++]) {
					r = -10000;
				}
				break;
			case MEMBER_TYPE_FLOAT:
				r = caps->read(fv);
				if (r != CAPS_SUCCESS || fv != floats[cf++]) {
					r = -10001;
				}
				break;
			case MEMBER_TYPE_LONG:
				r = caps->read(lv);
				if (r != CAPS_SUCCESS || lv != longs[cl++]) {
					r = -10002;
				}
				break;
			case MEMBER_TYPE_DOUBLE:
				r = caps->read(dv);
				if (r != CAPS_SUCCESS || dv != doubles[cd++]) {
					r = -10003;
				}
				break;
			case MEMBER_TYPE_STRING:
				r = caps->read(Sv);
				if (r != CAPS_SUCCESS || Sv != strings[cS++]) {
					r = -10004;
				}
				break;
			case MEMBER_TYPE_BINARY:
				r = caps->read(Bv);
				if (r != CAPS_SUCCESS || !binary_equal(Bv, binarys[cB++])) {
					r = -10005;
				}
				break;
			case MEMBER_TYPE_OBJECT:
				r = caps->read(Ov);
				if (r != CAPS_SUCCESS) {
						r = -10006;
						break;
				}
				RandomCapsFactory* t = sub_objects[cO++];
				if (!t->cpp_check(Ov)) {
						r = -10006;
				}
				break;
		}
		if (r != CAPS_SUCCESS) {
			printf("check failed, error = %d\n", r);
			break;
		}
	}
	return r == CAPS_SUCCESS;
}
