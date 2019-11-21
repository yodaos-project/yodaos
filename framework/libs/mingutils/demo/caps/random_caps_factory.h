#pragma once

#include <string>
#include <vector>
#include "caps.h"

class RandomCapsFactory {
public:
	RandomCapsFactory(bool capi);

	~RandomCapsFactory();

	void gen_integer();
	void gen_float();
	void gen_long();
	void gen_double();
	void gen_string();
	void gen_binary();
	void gen_object(uint32_t enable_sub_object);

	inline caps_t caps() const { return c_this_caps; }

	inline std::shared_ptr<Caps>& caps_ptr() { return this_caps; }

	// 进行序列化反序列化测试并检查结果
	int32_t check();

	bool c_check(caps_t caps);

	bool cpp_check(std::shared_ptr<Caps>& caps);

private:
	void gen_random_member(uint32_t enable_sub_object);

	int32_t c_check();

	int32_t cpp_check();

private:
	std::vector<uint8_t> member_types;
	std::vector<int32_t> integers;
	std::vector<float> floats;
	std::vector<int64_t> longs;
	std::vector<double> doubles;
	std::vector<std::string> strings;
	std::vector<std::vector<uint8_t> > binarys;
	std::vector<RandomCapsFactory*> sub_objects;
	std::shared_ptr<Caps> this_caps;
	caps_t c_this_caps = 0;
	bool use_c_api = false;
};
