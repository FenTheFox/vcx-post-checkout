#pragma once

#include <algorithm>
#include <random>
#include <string>

#include <uuid.h>

static inline std::string gen_uuid()
{
	std::random_device rd;
	auto seed_data = std::array<int, std::mt19937::state_size>{};
	std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
	std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
	std::mt19937 generator(seq);
	auto uuid_str = uuids::to_string(uuids::uuid_random_generator{generator}());
	std::transform(uuid_str.begin(), uuid_str.end(), uuid_str.begin(),
						[](unsigned char c){ return std::toupper(c); });
	return uuid_str;
}