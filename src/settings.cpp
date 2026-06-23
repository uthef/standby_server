#include <settings.hpp>
#include <fstream>
#include <iostream>

using namespace uthef;

settings::settings(std::string file_path) : file_path(file_path) {
	
}

void settings::load() {
	groups.clear();	

	std::ifstream file_stream(file_path, std::ios_base::in);

	if (!file_stream.is_open()) {
		perror("Configuration file error");
		exit(-1);
	}
	
	enum { BEGIN = 0, PAIR = 1, GROUP = 2, GROUP_END = 3 };

	std::string key, value, group;
	int state = BEGIN;
	
	while (!file_stream.eof()) {
		int c = file_stream.get();

		if (c == -1)
			break;

		if (state == BEGIN && (c == ' ' || c == '\t'))
			continue;

		if (state == BEGIN && c == '=') {
			if (key.empty()) {
				abort();
			}

			state = 1;
			continue;
		}

		if (c == '[') {
			if (state == BEGIN && key.empty()) {
				group.clear();
				state = GROUP;
				continue;
			}
		}

		if (c == ']' && state == GROUP) {
			state = GROUP_END;
			continue;
		}

		if (c == '\n') {
			if ((state == BEGIN && !key.empty()) || state == GROUP) {
				abort();
			}

			if (!key.empty() && !value.empty()) {
				group = trim(group);
				groups[group][trim(key)] = value;
			}

			key.clear();
			value.clear();
			state = BEGIN;

			continue;
		}
		
		switch (state) {
			case BEGIN:
				key.push_back(c);
				break;
			case PAIR:
				value.push_back(c);
				break;
			case GROUP:
				group.push_back(c);
				break;
			case GROUP_END:
				if (c != ' ' && c != '\t')
					abort();
				break;
		}
	}

	file_stream.close();
}

std::string settings::trim(std::string str) {
	int i = 0, j = str.size();
	bool stop_left = false,
		 stop_right = false;

	for (i = 0, j = str.size() - 1; j > i && (!stop_left || !stop_right); ) {
		char leftc = str.at(i),
			 rightc = str.at(j);

		if (!stop_left && leftc != '\n' && leftc != '\t' && leftc != ' ' && leftc != '\r') {
			stop_left = true;

			if (stop_right)
				break;
		}

		if (!stop_right && rightc != '\n' && rightc != '\t' && rightc != ' ' && rightc != '\r') {
			stop_right = true;

			if (stop_left)
				break;
		}

		if (!stop_left)
			i++;

		if (!stop_right)
			j--;
	}

	return str.substr(i, j - i + 1);
}

std::string settings::get(std::string key, std::string section) {
	auto group = groups.find(section);

	if (group == groups.end())
		return "";

	auto value = group->second.find(key);

	if (value == group->second.end())
		return "";

	return value->second;
}

settings::group_map *settings::get_groups() {
	return &groups;
}

void settings::abort() {
	std::cerr << "Malformed configuration file" << std::endl;
	exit(-1);
}
