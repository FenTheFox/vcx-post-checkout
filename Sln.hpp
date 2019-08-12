#pragma once

#include <string>
#include <vector>

#include "Vcxproj.hpp"

class Sln
{
	const std::string fname_;
	const std::vector<Vcxproj> &projs_;

public:
	Sln(std::string &&fname, const std::vector<Vcxproj> &projs) : fname_(fname), projs_(projs) {}

	void Save();
};
