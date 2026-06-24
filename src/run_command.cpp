#include <run_command.hpp>

bool uthef::run_command(std::string cmd, std::string &output) {
	char buffer[1024];
	FILE *pipe = popen(cmd.c_str(), "r");

	if (!pipe)
		return false;
	
	output.clear();

	while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
		output.append(buffer);
	}

	pclose(pipe);

	return true;
}
