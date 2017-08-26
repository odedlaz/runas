#include <string>

std::string getpath(const std::string &path, bool searchInPath);

// not a reference!
const std::string realpath(std::string &path);

