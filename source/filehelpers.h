#include <string>

std::string get_file_contents(const std::string &filename);

std::string get_first_four_chars(const std::string &filename);

std::string getDirectory(const std::string &fname);

bool hasEnding (const std::string &fullString, const std::string &ending);

bool isCartFile (const std::string &fullString);

bool isCPostFile (const std::string &fullString);

bool isHiddenFile (const std::string &fullString);

bool isAbsolutePath (const std::string &path);

std::string getFileExtension(const std::string &path);
