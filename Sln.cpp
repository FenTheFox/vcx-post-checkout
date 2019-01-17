#include "Sln.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "Vcxproj.hpp"

void Sln::Save()
{
	if (std::none_of(projs_.begin(), projs_.end(), [](const Vcxproj &p) {return p.is_new(); }) &&
		boost::filesystem::exists(fname_))
		return;

	std::ofstream file(fname_);
	if (!file.is_open())
		throw std::runtime_error("Could not open solution file for writing");

	file << HEADER << std::endl;
	for (const auto &proj : projs_) {
		std::string end = proj.get_uuid();
		if (proj.is_folder()) {
			end += "\n\tProjectSection(SolutionItems) = preProject\n";
			for (auto &f : proj.get_files()) {
				end += "\t\t" + f.string() + " = " + f.string();
			}
			end += "\tEndProjectSection";
		}
		file << boost::format(PROJ_FMT) % proj.get_name() % proj.get_file().string() % end << std::endl;
	}
	file << MID;

	std::ostringstream nested;
	for (const auto &proj : projs_) {
		if (!proj.is_folder())
			file << boost::format(CFG_FMT) % proj.get_uuid();
		else
			nested << boost::format("\n%s = %s") % proj.get_uuid() % proj.get_parent();
	}

	file << boost::format(FOOTER_FMT) % nested.str() %
		boost::algorithm::to_upper_copy(boost::uuids::to_string(boost::uuids::random_generator()())) << std::endl;
	file.close();
}
