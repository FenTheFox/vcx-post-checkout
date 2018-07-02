#pragma once

#include <algorithm>
#include <set>
#include <string>

#include <git2.h>

#include "FileTree.hpp"

class GitRepo
{
	static constexpr const char BAD_PATH_CHARS[] = { '\x00', '"', '*', ':', '<', '>', '?', '|' };

	char err_buf_[2048];
	Directory paths_;

	void AddPath(std::string &&path)
	{
		if (std::all_of(BAD_PATH_CHARS, BAD_PATH_CHARS + sizeof BAD_PATH_CHARS / sizeof BAD_PATH_CHARS[0],
						[&](char c) { return path.find(c) == std::string::npos; })) {
			paths_.insert(std::move(path));
		}
	}

	static int WalkCb(const char *root, const git_tree_entry *entry, void *payload)
	{
		if (git_tree_entry_filemode(entry) != GIT_FILEMODE_LINK)
			static_cast<GitRepo*>(payload)->AddPath(root + std::string(git_tree_entry_name(entry)) +
				(git_tree_entry_filemode(entry) == GIT_FILEMODE_TREE ? "/" : ""));
		return 0;
	}

	static int StatusCb(const char *path, uint32_t status_flags, void *payload)
	{
		if (status_flags & GIT_STATUS_INDEX_NEW || status_flags & GIT_STATUS_WT_NEW)
			static_cast<GitRepo*>(payload)->AddPath(path);
		return 0;
	}

public:
	GitRepo();

	std::set<std::string> get_files(std::string &&rel_path);
};
