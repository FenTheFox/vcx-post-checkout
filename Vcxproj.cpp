#include "Vcxproj.hpp"

#include <iostream>
#include <set>
#include <string>

#include <pugixml.hpp>

#include "fs.hpp"
#include "uuid.hpp"

extern bool debug;

using namespace std::string_literals;

namespace
{
constexpr const char *INCLUDES[] = { ".h", ".hh", ".hpp" }, *SOURCES[] = { ".c", ".cc", ".cpp" },
                     *REMOTE_DIR = "RemoteProjectDir", *PREPROCESSOR_DEFINES = "NMakePreprocessorDefinitions",
                     *INCLUDE_PATH = "IncludePath", *MAKE_CMD = "RemoteBuildCommandLine",
                     *CLEAN_TGT = "RemoteCleanCommandLine",
                     *EMPTY_FILTERS = R"(<?xml version='1.0' encoding='UTF-8'?>
<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
	<ItemGroup>
		<ClCompile Include="Placeholder.cpp">
			<Filter>.</Filter>
		</ClCompile>
	</ItemGroup>
	<ItemGroup>
		<ClInclude Include="Placeholder.hpp">
			<Filter>.</Filter>
		</ClInclude>
	</ItemGroup>
	<ItemGroup>
		<None Include="Placeholder">
			<Filter>.</Filter>
		</None>
	</ItemGroup>
	<ItemGroup>
		<Filter Include="placeholder">
			<UniqueIdentifier>{de229b78-0ae7-4102-836d-27aaf769b9e1}</UniqueIdentifier>
		</Filter>
	</ItemGroup>
</Project>)";

const std::string EMPTY_PROJECT = R"(<?xml version='1.0' encoding='UTF-8'?>
<Project DefaultTargets="Build" ToolsVersion="15.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
	<ItemGroup Label="ProjectConfigurations">
		<ProjectConfiguration Include="Debug|x64">
			<Configuration>Debug</Configuration>
			<Platform>x64</Platform>
		</ProjectConfiguration>
		<ProjectConfiguration Include="Release|x64">
			<Configuration>Release</Configuration>
			<Platform>x64</Platform>
		</ProjectConfiguration>
	</ItemGroup>
	<PropertyGroup Label="Globals">
		<ProjectGuid>{__UUID__}</ProjectGuid>
		<Keyword>Linux</Keyword>
		<RootNamespace>__NAME__</RootNamespace>
		<MinimumVisualStudioVersion>15.0</MinimumVisualStudioVersion>
		<ApplicationType>Linux</ApplicationType>
		<ApplicationTypeRevision>1.0</ApplicationTypeRevision>
		<TargetLinuxPlatform>Generic</TargetLinuxPlatform>
		<LinuxProjectType>{FC1A4D80-50E9-41DA-9192-61C0DBAA00D2}</LinuxProjectType>
	</PropertyGroup>
	<Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props"/>
	<PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'" Label="Configuration">
		<UseDebugLibraries>true</UseDebugLibraries>
		<ConfigurationType>StaticLibrary</ConfigurationType>
	</PropertyGroup>
	<PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'" Label="Configuration">
		<UseDebugLibraries>false</UseDebugLibraries>
		<ConfigurationType>StaticLibrary</ConfigurationType>
	</PropertyGroup>
	<Import Project="$(VCTargetsPath)\Microsoft.Cpp.props"/>
	<ImportGroup Label="ExtensionSettings"/>
	<ImportGroup Label="Shared"/>
	<ImportGroup Label="PropertySheets"/>
	<PropertyGroup Label="UserMacros"/>
	<ItemGroup>
		<ClInclude Include="Placeholder.hpp"/>
	</ItemGroup>
	<ItemGroup>
		<ClCompile Include="Placeholder.cpp"/>
	</ItemGroup>
	<ItemGroup>
		<None Include="Placeholder"/>
	</ItemGroup>
	<ItemDefinitionGroup/>
	<Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets"/>
	<ImportGroup Label="ExtensionTargets"/>
</Project>)", UUID_("__UUID__"), NAME("__NAME__"), SRC("src"), INC("include");

inline std::string get_fpath(const std::string &f)
{
	return f.find('/') == std::string::npos ? f : "./" + f;
}

inline std::string get_ffilter(const fs::path &f, bool meta_dirs)
{
	if (f.has_parent_path() && f.parent_path() != ".") {
		auto path = f.parent_path().string().substr(2);
		std::replace(path.begin(), path.end(), '/', '\\');
		return path;
	}

	auto ends_with = [&f](const char *c) { return f.extension() == c; };
	if (meta_dirs && std::any_of(INCLUDES, INCLUDES + sizeof INCLUDES / sizeof INCLUDES[0], ends_with))
		return INC;
	if (meta_dirs && std::any_of(SOURCES, SOURCES + sizeof SOURCES / sizeof INCLUDES[0], ends_with))
		return SRC;
	return "";
}
} // namespace

void Vcxproj::Update(std::set<std::string> &&new_files)
{
	std::set<fs::path> files, folders;
	for (auto& f : new_files) {
		if (f[f.length() - 1] == '/')
			folders.insert(f.substr(0, f.length() - 1));
		else
			files.insert(get_fpath(f));
	}

	if (folder_) {
		for (auto &p : files)
			if (!p.has_parent_path())
				files_.insert(file_ / p);
		return;
	}

	bool f_include = false, f_src = false;
	if (proj_.first_child().empty()) {
		auto res = proj_.load_file(file_.c_str());
		if (!res && res.status != pugi::status_file_not_found)
			throw std::runtime_error(res.description());

		if (!res) {
			new_ = true;
			auto str = EMPTY_PROJECT;
			size_t pos = str.find(UUID_);
			str.replace(pos, UUID_.size(), gen_uuid());
			pos = str.find(NAME);
			str.replace(pos, NAME.size(), file_.stem().string());
			res = proj_.load_string(str.c_str());
			if (!res)
				throw std::runtime_error(res.description());
		}

		res = fltr_.load_file((file_.string() + ".filters").c_str());
		if (!res && res.status != pugi::status_file_not_found)
			throw std::runtime_error(res.description());

		if (!res) {
			res = fltr_.load_string(EMPTY_FILTERS);
			if (!res)
				throw std::runtime_error(res.description());
		}

		for (auto& n : proj_.select_nodes("//ItemGroup[not(@Label)]/*[@Include]/@Include"))
			files_.insert(n.attribute().value());
		for (auto& n : fltr_.select_nodes("//Filter/@Include")) {
			auto val = n.attribute().value();
			if (val == INC)
				f_include = true;
			else if (val == SRC)
				f_src = true;
			folders_.insert(val);
		}

		uuid_ = pugi::xpath_query("//ProjectGuid/text()").evaluate_string(proj_);
	}

	for (const auto &xn : proj_.select_nodes("//PropertyGroup[@Condition]")) {
		auto n = xn.node();

		// Update remote directory
		auto node = n.child(REMOTE_DIR);
		if (!node) node = n.append_child(REMOTE_DIR);
		updated_ |= node.text().get() != remote_dir_;
		node.text().set(remote_dir_.c_str());

		// Update local include path(s)
		node = n.child(INCLUDE_PATH);
		if (!node) node = n.append_child(INCLUDE_PATH);
		updated_ |= node.text().get() != includes_;
		node.text().set(includes_.c_str());

		// Update preprocessor defines
		node = n.child(PREPROCESSOR_DEFINES);
		if (!node) node = n.append_child(PREPROCESSOR_DEFINES);
		updated_ |= node.text().get() != defines_;
		node.text().set(defines_.c_str());

		// Update make command
		node = n.child(MAKE_CMD);
		if (!node) node = n.append_child(MAKE_CMD);
		auto make_cmd = make_cmd_;
		if (!release_env_.empty() &&
			n.attribute("Condition").as_string() == "'$(Configuration)|$(Platform)'=='Release|x64'"s)
			make_cmd += " " + release_env_;

		updated_ |= node.text().get() != make_cmd;
		node.text().set(make_cmd.c_str());

		// Update clean target
		node = n.child(CLEAN_TGT);
		if (!node) node = n.append_child(CLEAN_TGT);
		updated_ |= node.text().get() != clean_cmd_;
		node.text().set(clean_cmd_.c_str());
	}

	pugi::xpath_variable_set xv_set;
	auto var = xv_set.add("path", pugi::xpath_type_string);
	pugi::xpath_query q_file("//*[@Include=string($path)]", &xv_set);

	auto include = std::make_pair(proj_.select_node("//ItemGroup/ClInclude/..").node(),
								  fltr_.select_node("//ItemGroup/ClInclude/..").node()),
		compile = std::make_pair(proj_.select_node("//ItemGroup/ClCompile/..").node(),
								 fltr_.select_node("//ItemGroup/ClCompile/..").node()),
		other = std::make_pair(proj_.select_node("//ItemGroup/None/..").node(),
							   fltr_.select_node("//ItemGroup/None/..").node());
	auto filter = fltr_.select_node("//Filter/../ItemGroup").node();
	auto item_grps = proj_.select_nodes("//ItemGroup[not(@Label)]");
	if (!include.first) include.first = item_grps[0].node();
	if (!compile.first) compile.first = item_grps[1].node();
	if (!other.first) other.first = item_grps[2].node();
	item_grps = fltr_.select_nodes("//ItemGroup");
	if (!include.second) include.second = item_grps[0].node();
	if (!compile.second) compile.second = item_grps[1].node();
	if (!other.second) other.second = item_grps[2].node();
	if (!filter) filter = item_grps[3].node();

	std::set<fs::path> add, rm;
	std::set_difference(files.begin(), files.end(), files_.begin(), files_.end(), std::inserter(add, add.begin()));
	std::set_difference(files_.begin(), files_.end(), files.begin(), files.end(), std::inserter(rm, rm.begin()));
	updated_ |= !add.empty() || !rm.empty();

	if (debug)
		std::cout << file_ << std::endl << "FILES:" << std::endl;
	for (auto &f : rm) {
		if (debug)
			std::cout << "\trm: " << f << std::endl;
		var->set(f.string().c_str());
		auto n = proj_.select_node(q_file).node();
		n.parent().remove_child(n);
		n = fltr_.select_node(q_file).node();
		n.parent().remove_child(n);
		files_.erase(f);
	}
	for (auto &f : add) {
		if (debug)
			std::cout << "\tadd: " << f << std::endl;
		auto ends_with = [&](const char *c) { return f.extension() == c; };
		decltype(include) nodes;
		const char *name;
		if (std::any_of(INCLUDES, INCLUDES + sizeof INCLUDES / sizeof INCLUDES[0], ends_with)) {
			nodes = include; name = "ClInclude";
		} else if (std::any_of(SOURCES, SOURCES + sizeof SOURCES / sizeof SOURCES[0], ends_with)) {
			nodes = compile; name = "ClCompile";
		} else {
			nodes = other; name = "None";
		}
		nodes.first.append_child(name).append_attribute("Include").set_value(f.string().c_str());
		auto child = nodes.second.append_child(name);
		child.append_attribute("Include").set_value(f.string().c_str());
		auto filter = get_ffilter(f, meta_dirs_);
		if (!filter.empty())
			child.append_child("Filter").text().set(filter.c_str());
		if (filter == INC)
			f_include = true;
		else if (filter == SRC)
			f_src = true;
		files_.insert(f);
	}

	add.clear();
	rm.clear();
	std::set_difference(folders.begin(), folders.end(), folders_.begin(), folders_.end(), std::inserter(add, add.begin()));
	std::set_difference(folders_.begin(), folders_.end(), folders.begin(), folders.end(), std::inserter(rm, rm.begin()));
	if (f_include) {
		if (folders_.find(INC) == folders_.end())
			add.insert(INC);
		rm.erase(INC);
	}
	if (f_src) {
		if (folders_.find(SRC) == folders_.end())
			add.insert(SRC);
		rm.erase(SRC);
	}

	updated_ |= !add.empty() || !rm.empty();
	if (debug)
		std::cout << "FOLDERS:" << std::endl;
	for (auto &f : rm) {
		std::string folder = f.string();
		std::replace(folder.begin(), folder.end(), '/', '\\');
		if (debug)
			std::cout << "\trm: " << folder << std::endl;
		var->set(folder.c_str());
		auto n = fltr_.select_node(q_file).node();
		n.parent().remove_child(n);
		folders_.erase(f);
	}
	for (auto &f : add) {
		std::string folder = f.string();
		std::replace(folder.begin(), folder.end(), '/', '\\');
		if (debug)
			std::cout << "\tadd: " << folder << std::endl;
		auto node = filter.append_child("Filter");
		node.append_attribute("Include").set_value(folder.c_str());
		node.append_child("UniqueIdentifier").text().set(("{" + gen_uuid() + "}").c_str());
		folders_.insert(f);
	}
}
