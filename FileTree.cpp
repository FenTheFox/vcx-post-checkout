#include "FileTree.hpp"

#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

void Directory::insert(std::string &&path)
{
	auto idx = path.find('/');
	if (idx == std::string::npos)
		children.push_back(std::make_shared<File>(path));
	else if (idx == path.length() - 1)
		children.push_back(std::make_shared<Directory>(path.substr(0, path.length() - 1)));
	else {
		std::string p = path.substr(0, idx);
		for (const auto& n : children) {
			if (n->is_directory() && n->name == p) {
				static_cast<Directory&>(*n).insert(path.substr(idx + 1));
				return;
			}
		}
		auto d = std::make_shared<Directory>(p);
		d->insert(path.substr(idx + 1));
		children.push_back(d);
	}
}

std::set<std::string> Directory::get()
{
	std::set<std::string> result;
	result.insert(name + "/");

	for (auto &n : children) {
		if (n->is_directory())
			for (const auto &o : static_cast<Directory&>(*n).get())
				result.insert(name + "/" + o);
		else
			result.insert(name + "/" + n->name);
	}
	return result;
}

std::set<std::string> Directory::get(const std::string &subdir, bool recursive)
{
	std::set<std::string> result;
	if (subdir.empty()) {
		for (auto &n : children) {
			if (recursive && n->is_directory())
				for (const auto &o : static_cast<Directory&>(*n).get())
					result.insert(o);
			else if (!n->is_directory())
				result.insert(n->name);
		}
		return result;
	}

	std::string child = subdir.substr(0, subdir.find('/')),
		sub = child.length() + 1 < subdir.length() ? subdir.substr(child.length() + 1) : "";
	for (auto &n : children) {
		if (n->is_directory() && n->name == child)
			return static_cast<Directory&>(*n).get(sub, recursive);
	}
	return std::set<std::string>{};
}
