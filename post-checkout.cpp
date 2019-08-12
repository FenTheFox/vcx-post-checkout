#include <fstream>
#include <iostream>
#include <string>
#include <typeinfo>

#ifdef __GNUC__
#include <cxxabi.h>
#endif

#include <rapidjson/document.h>
#include <rapidjson/encodedstream.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/error/en.h>

#include "fs.hpp"
#include "GitRepo.hpp"
#include "Vcxproj.hpp"
#include "Sln.hpp"

using namespace std::string_literals;

bool debug = false;

namespace
{
template<typename T>
inline std::string type_name(const T& ex)
{
#ifdef __GNUC__
	int status = 0;
	auto name = abi::__cxa_demangle(typeid(ex).name(), nullptr, nullptr, &status);
	if (name == nullptr)
		return typeid(ex).name();
	std::string res{name};
	free(name);
	return res;
#else
	return typeid(ex).name();
#endif
}
}

int main(int argc, char *argv[])
{
	std::ifstream cfg(".git/projects.json");
	if (!cfg) {
		std::cerr << "Couldn't open config file .git/projects.json" << std::endl;
		exit(1);
	}
	rapidjson::IStreamWrapper isw(cfg);
	rapidjson::AutoUTFInputStream<uint32_t, decltype(isw)> eis(isw);
	rapidjson::Document doc;
	if (doc.ParseStream<rapidjson::kParseTrailingCommasFlag | rapidjson::kParseCommentsFlag>(eis).HasParseError()) {
		std::cerr << "Parse error [" << rapidjson::GetParseError_En(doc.GetParseError()) << "] at character "
			<< doc.GetErrorOffset() << std::endl;
		exit(doc.GetParseError());
	}

	auto it = doc.FindMember("debug");
	if (it != doc.MemberEnd() && it->value.GetBool()) {
		debug = true;
		std::cout << fs::current_path() << std::endl;
	}

	it = doc.FindMember("skip");
	if (it != doc.MemberEnd() && it->value.GetBool()) {
		if (debug)
			std::cout << "Skipping" << std::endl;
		exit(0);
	}

	std::string default_defines, default_includes, default_make_cmd{"make -j"}, default_clean_tgt{"clean"},
		release_env;
	if (doc.HasMember("default_defines"))   default_defines   = doc["default_defines"].GetString();
	if (doc.HasMember("default_includes"))  default_includes  = doc["default_includes"].GetString();
	if (doc.HasMember("default_make_cmd"))  default_make_cmd  = doc["default_make_cmd"].GetString();
	if (doc.HasMember("default_clean_tgt")) default_clean_tgt = doc["default_clean_tgt"].GetString();
	if (doc.HasMember("release_env")) release_env = doc["release_env"].GetString();

	std::vector<Vcxproj> projs;
	for (auto &p : doc["projects"].GetArray()) {
		bool meta_dirs = !p.HasMember("meta_dirs") || p["meta_dirs"].GetBool();
		std::string defines{default_defines}, includes{default_includes}, make_cmd{default_make_cmd},
			clean_tgt{default_clean_tgt};
		if (p.HasMember("defines")) defines = p["defines"].GetString() + ";"s + default_defines;
		if (p.HasMember("includes")) includes = p["includes"].GetString() + ";"s + default_includes;
		if (p.HasMember("make_cmd")) make_cmd = p["make_cmd"].GetString();
		if (p.HasMember("clean_tgt")) clean_tgt = p["clean_tgt"].GetString();
		if (p.HasMember("recurse") && p["recurse"].GetBool()) {
			fs::path file{p["file"].GetString()};
			Vcxproj parent{file.stem().string()};

			projs.push_back(parent);
			for (fs::directory_iterator it{file.parent_path()}; it != fs::directory_iterator{}; ++it) {
				if (!fs::is_directory(it->symlink_status()))
					continue;

				std::string fname{it->path().filename().string()};
				projs.emplace_back(meta_dirs, it->path().string() + '/' + fname + ".vcxproj",
								   p["remote_dir"].GetString() + fname, defines, includes, make_cmd, clean_tgt,
								   release_env, parent.get_uuid());
			}
		} else {
			projs.emplace_back(meta_dirs, p["file"].GetString(), p["remote_dir"].GetString(), defines, includes,
							   make_cmd, clean_tgt, release_env);
		}
	}

	try {
		GitRepo repo;
		for (auto &p : projs) {
			if (p.is_folder())
				p.Update(repo.get_files(std::string(p.get_file().string()), false));
			else
				p.Update(repo.get_files(std::string(p.get_file().parent_path().string()), true));
			p.Save();
		}

		Sln sln(doc["solution"].GetString(), projs);
		sln.Save();
	}
	catch (const std::exception &ex) {
		std::cerr << type_name(ex) << " encountered: " << ex.what() << std::endl;
	}
}
