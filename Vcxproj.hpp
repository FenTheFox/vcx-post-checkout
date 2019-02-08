#pragma once

#include <string>
#include <set>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <pugixml.hpp>

namespace fs = boost::filesystem;

class Vcxproj
{
	bool updated_ = false, new_ = false, meta_dirs_, folder_;
	fs::path file_;
	std::string remote_dir_, defines_, includes_, make_cmd_, clean_cmd_, release_env_, uuid_, parent_uuid_;
	pugi::xml_document proj_, fltr_;
	std::set<fs::path> files_, folders_;

public:
	Vcxproj(const std::string &name) :
		folder_(true), file_(name),
		uuid_("{" + boost::algorithm::to_upper_copy(boost::uuids::to_string(boost::uuids::random_generator{}())) + "}")
	{}

	Vcxproj(bool meta_dirs, const std::string &file, const std::string &remote_dir, const std::string &defines,
		    const std::string &includes, const std::string &make_cmd, const std::string &clean_tgt,
		    const std::string &release_env, const std::string &p_uuid = "") :
		meta_dirs_(meta_dirs), folder_(false), file_(file), remote_dir_(remote_dir), defines_(defines),
		includes_(includes), make_cmd_("cd $(RemoteProjectDir) && " + make_cmd),
		clean_cmd_(make_cmd_ + " " + clean_tgt), release_env_(release_env), parent_uuid_(p_uuid)
	{}

	Vcxproj(Vcxproj &&other) noexcept :
		updated_(other.updated_), new_(other.new_), meta_dirs_(other.meta_dirs_), folder_(other.folder_),
		file_(std::move(other.file_)), remote_dir_(std::move(other.remote_dir_)), defines_(std::move(other.defines_)),
		includes_(std::move(other.includes_)), make_cmd_(std::move(other.make_cmd_)),
		clean_cmd_(std::move(other.clean_cmd_)), uuid_(std::move(other.uuid_)), parent_uuid_(other.parent_uuid_),
		files_(std::move(other.files_)), folders_(std::move(other.folders_))
	{
		proj_.reset(other.proj_);
		fltr_.reset(other.fltr_);
	}

	Vcxproj(const Vcxproj &oth) :
		updated_(oth.updated_), new_(oth.new_), meta_dirs_(oth.meta_dirs_), folder_(oth.folder_), file_(oth.file_),
		remote_dir_(oth.remote_dir_), defines_(oth.defines_), includes_(oth.includes_), make_cmd_(oth.make_cmd_),
		clean_cmd_(oth.clean_cmd_), uuid_(oth.uuid_), parent_uuid_(oth.parent_uuid_), files_(oth.files_),
		folders_(oth.folders_)
	{
		proj_.reset(oth.proj_);
		fltr_.reset(oth.fltr_);
	}

	void Update(std::set<std::string> &&new_files);

	void Save()
	{
		if (updated_) {
			proj_.save_file(file_.c_str());
			fltr_.save_file((file_.string() + ".filters").c_str());
		}
	}

	bool is_new() const { return new_; }
	fs::path get_file() const { return file_; }
	std::string get_name() const { return file_.stem().string(); }
	const std::string& get_uuid() const { return uuid_; }

	bool is_folder() const { return folder_; }
	const std::set<fs::path>& get_files() const { return files_; }
	const std::string& get_parent() const { return parent_uuid_; }
};
