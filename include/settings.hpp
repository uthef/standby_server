#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <unordered_map>
#include <string>

namespace uthef {

class settings {
private:
	typedef std::unordered_map<std::string, std::string> group;
	typedef std::unordered_map<std::string, group> group_map;
	
	std::string file_path;
	group_map groups;
public:
	settings(std::string file_path);
	void load();
	std::string get(std::string key, std::string section = "");
	group_map *get_groups();
private:
	void abort();
	std::string trim(std::string str);
};

}

#endif
