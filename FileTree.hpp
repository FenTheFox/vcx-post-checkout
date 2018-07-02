#pragma once

#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

struct File
{
	std::string name;

	File(const std::string &n = "") : name(n) {}

	virtual bool is_directory() const { return false; }
	virtual std::string to_string(std::string &&prefix = "") const { return prefix + name; }

	std::ostream& operator<<(std::ostream &os)
	{
		os << to_string();
		return os;
	}
};

struct Directory : public File
{
	std::vector<std::shared_ptr<File>> children;

	using File::File;
	bool is_directory() const override { return true; }
	std::string to_string(std::string &&prefix) const override
	{
		std::string result = prefix + name + "/";
		for (auto& n : children) {
			result += "\n" + n->to_string("   " + prefix);
		}
		return result;
	}

	void insert(std::string &&path);

	std::set<std::string> get();

	std::set<std::string> get(const std::string &subdir);
};
