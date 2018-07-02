#pragma once

#include <string>
#include <vector>

#include "Vcxproj.hpp"

class Sln
{
	static constexpr const char *HEADER = R"(Microsoft Visual Studio Solution File, Format Version 12.00
# Visual Studio 15
VisualStudioVersion = 15.0.26730.16
MinimumVisualStudioVersion = 10.0.40219.1)",
	*PROJ_FMT = R"(Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "%s", "%s", "%s"
EndProject)",
	*MID = R"(Project("{2150E333-8FDC-42A3-9474-1A3956D46DE8}") = "Solution Items", "Solution Items", "{6B0498A5-FDC9-4025-9FEC-C3B5C19616A4}"
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
	*CFG_FMT = R"(
		%1%.Debug|x64.ActiveCfg = Debug|x64
		%1%.Debug|x64.Build.0 = Debug|x64
		%1%.Release|x64.ActiveCfg = Release|x64
		%1%.Release|x64.Build.0 = Release|x64)",
	*FOOTER_FMT = R"(
	EndGlobalSection
	GlobalSection(SolutionProperties) = preSolution
		HideSolutionNode = FALSE
	EndGlobalSection
	GlobalSection(ExtensibilityGlobals) = postSolution
		SolutionGuid = %s
	EndGlobalSection
EndGlobal)";

	const std::string fname_;
	const std::vector<Vcxproj> &projs_;

public:
	Sln(std::string &&fname, const std::vector<Vcxproj> &projs) : fname_(fname), projs_(projs) {}

	void Save();
};
