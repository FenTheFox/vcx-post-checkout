#pragma once

#include <string>
#include <set>

#include <boost/filesystem.hpp>
#include <pugixml.hpp>

namespace fs = boost::filesystem;

class Vcxproj
{
	bool updated_ = false, new_ = false;
	fs::path file_;
	std::string remote_dir_, defines_, includes_, make_cmd_, clean_cmd_, release_env_, uuid_;
	pugi::xml_document proj_, fltr_;
	std::set<fs::path> files_, folders_;

public:
	Vcxproj(std::string &&file, std::string &&remote_dir, std::string &&defines, std::string &&includes,
			std::string &&make_cmd, std::string &&clean_tgt, std::string &&release_env) :
		file_(file), remote_dir_(remote_dir), defines_(defines), includes_(includes),
		make_cmd_("cd $(RemoteProjectDir) && " + make_cmd), clean_cmd_(make_cmd_ + " " + clean_tgt),
		release_env_(release_env)
	{}

	Vcxproj(Vcxproj &&other) :
		updated_(other.updated_), file_(std::move(other.file_)), remote_dir_(std::move(other.remote_dir_)),
		defines_(std::move(other.defines_)), includes_(std::move(other.includes_)),
		make_cmd_(std::move(other.make_cmd_)), clean_cmd_(std::move(other.clean_cmd_)), uuid_(std::move(other.uuid_)),
		files_(std::move(other.files_)), folders_(std::move(folders_))
	{
		proj_.reset(other.proj_);
		fltr_.reset(other.fltr_);
	}

	Vcxproj(const Vcxproj &oth) :
		updated_(oth.updated_), file_(oth.file_), remote_dir_(oth.remote_dir_), defines_(oth.defines_),
		includes_(oth.includes_), make_cmd_(oth.make_cmd_), clean_cmd_(oth.clean_cmd_), uuid_(oth.uuid_),
		files_(oth.files_), folders_(folders_)
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
};
