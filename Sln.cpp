#include "Sln.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <fmt/format.h>

#include "fs.hpp"
#include "uuid.hpp"
#include "Vcxproj.hpp"

namespace
{
constexpr const char HEADER[] = R"(Microsoft Visual Studio Solution File, Format Version 12.00
# Visual Studio 15
VisualStudioVersion = 15.0.26730.16
MinimumVisualStudioVersion = 10.0.40219.1)",
PROJ_FMT[] = R"(Project("{{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}}") = "{:s}", "{:s}", "{:s}"
EndProject)",
FOLDER_FMT[] = R"(Project("{{2150E333-8FDC-42A3-9474-1A3956D46DE8}}") = "{:s}", "{:s}", "{:s}"{:s}
EndProject)",
MID[] = R"(Project("{2150E333-8FDC-42A3-9474-1A3956D46DE8}") = "Solution Items", "Solution Items", "{6B0498A5-FDC9-4025-9FEC-C3B5C19616A4}"
	ProjectSection(SolutionItems) = preProject
		.git\projects.json = .git\projects.json
	EndProjectSection
EndProject
Global
	GlobalSection(SolutionConfigurationPlatforms) = preSolution
		Debug|x64 = Debug|x64
		Release|x64 = Release|x64
	EndGlobalSection
	GlobalSection(ProjectConfigurationPlatforms) = postSolution)",
CFG_FMT[] = R"(
		{0:s}.Debug|x64.ActiveCfg = Debug|x64
		{0:s}.Debug|x64.Build.0 = Debug|x64
		{0:s}.Release|x64.ActiveCfg = Release|x64
		{0:s}.Release|x64.Build.0 = Release|x64)",
FOOTER_FMT[] = R"(
	EndGlobalSection
	GlobalSection(SolutionProperties) = preSolution
		HideSolutionNode = FALSE
	EndGlobalSection
	GlobalSection(NestedProjects) = preSolution{:s}
	EndGlobalSection
	GlobalSection(ExtensibilityGlobals) = postSolution
		SolutionGuid = {:s}
	EndGlobalSection
EndGlobal)";
}

void Sln::Save()
{
	if (std::none_of(projs_.begin(), projs_.end(), [](const Vcxproj &p) {return p.is_new(); }) && fs::exists(fname_))
		return;

	std::ofstream file(fname_);
	if (!file.is_open())
		throw std::runtime_error("Could not open solution file for writing");

	file << HEADER << std::endl;
	for (const auto &proj : projs_) {
		std::string fname{proj.get_file().string()};
		std::replace(fname.begin(), fname.end(), '/', '\\');
		if (proj.is_folder()) {
			std::string end{"\n\tProjectSection(SolutionItems) = preProject\n"};
			for (auto &f : proj.get_files()) {
				std::string file{f.string()};
				std::replace(file.begin(), file.end(), '/', '\\');
				end += fmt::format("\t\t{0:s} = {0:s}\n", file);
			}
			end += "\tEndProjectSection";
			file << fmt::format(FOLDER_FMT, proj.get_name(), fname, proj.get_uuid(), end)
			     << std::endl;
		} else
			file << fmt::format(PROJ_FMT, proj.get_name(), fname, proj.get_uuid())
			     << std::endl;
	}
	file << MID;

	std::ostringstream nested;
	for (const auto &proj : projs_) {
		if (!proj.is_folder()) {
			file << fmt::format(CFG_FMT, proj.get_uuid());
			if (!proj.get_parent().empty())
				nested << fmt::format("\n\t\t{:s} = {:s}", proj.get_uuid(), proj.get_parent());
		}
	}

	file << fmt::format(FOOTER_FMT, nested.str(), gen_uuid()) << std::endl;
	file.close();
}
