#include "GitRepo.hpp"

#include <algorithm>
#include <exception>
#include <set>
#include <string>

#include <cstdio>

#define check_err(expr) do { int err = expr; \
	if (err < 0) { \
		const git_error *e = giterr_last(); \
		snprintf(err_buf_, sizeof err_buf_, "Error in %s at [%s:%d], %d/%d: %s", \
		         __FUNCTION__, __FILE__, __LINE__, err, e->klass, e->message); \
		throw std::runtime_error(err_buf_); \
	} \
} while (0)

constexpr const char GitRepo::BAD_PATH_CHARS[];

GitRepo::GitRepo()
{
	git_libgit2_init();
	git_repository *repo = nullptr;
	git_tree *tree = nullptr;
	check_err(git_repository_open(&repo, "."));
	check_err(git_revparse_single(reinterpret_cast<git_object**>(&tree), repo, "HEAD^{tree}"));
	check_err(git_tree_walk(tree, GIT_TREEWALK_PRE, &WalkCb, this));
	git_tree_free(tree);

	git_status_options opts = GIT_STATUS_OPTIONS_INIT;
	opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS | GIT_STATUS_OPT_UPDATE_INDEX;
	git_status_foreach_ext(repo, &opts, &StatusCb, this);
	git_repository_free(repo);
	git_libgit2_shutdown();
}

std::set<std::string> GitRepo::get_files(std::string &&rel_path, bool recursive)
{
	std::replace(rel_path.begin(), rel_path.end(), '\\', '/');
	if (rel_path.find("./") == 0)
		rel_path = rel_path.substr(2);
	if (rel_path.empty() || rel_path == ".")
		return paths_.get("", recursive);

	if (rel_path[rel_path.length() - 1] != '/')
		rel_path += '/';
	return paths_.get(rel_path, recursive);
}
