#include <string>
#include <vector>

std::string get_file_contents(const std::string &filename){
    FILE *in = fopen(filename.c_str(), "rb");
    if (in)
    {
        std::string contents;
        fseek(in, 0, SEEK_END);
        contents.resize(ftell(in));
        fseek(in, 0, SEEK_SET);
        fread(contents.data(), 1, contents.size(), in);
        fclose(in);
        return contents;
    }

  return "";
}

std::string get_first_four_chars(const std::string &filename){
    FILE *in = fopen(filename.c_str(), "rb");
    if (in)
    {
        std::string contents;
        fseek(in, 0, SEEK_END);
        int fileLength = ftell(in);
        int bufSize = 4 < fileLength ? 4 : fileLength;
        contents.resize(bufSize);
        fseek(in, 0, SEEK_SET);
        fread(contents.data(), 1, contents.size(), in);
        fclose(in);
        return contents;
    }

  return "";
}

//https://stackoverflow.com/a/8518855
std::string getDirectory(const std::string &fname){
     size_t pos = fname.find_last_of('/');
     return (std::string::npos == pos) ? "" : fname.substr(0, pos);
}

bool isAbsolutePath(std::string const &path) {
    if (path.length() == 0) {
        return false;
    }

    if (path[0] == '/') {
        return true;
    } 
    
    size_t colonPos = path.find_first_of(":");
    size_t slashPos = path.find_first_of("/");

    if (colonPos != std::string::npos && slashPos == (colonPos + 1)){
        return true;
    }
    
    return false;
}

std::string getFileExtension(const std::string &path) {
    size_t pos = path.find_last_of('.');
     return (std::string::npos == pos) ? "" : path.substr(pos);
}

bool hasEnding(std::string const &fullString, const std::string &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

bool isHiddenFile(const std::string &fullString) {
    if (fullString.length() >= 0) {
        return (0 == fullString.compare(0, 2, "._"));
    } else {
        return false;
    }
}

bool isCartFile(const std::string &fullString) {
    return !isHiddenFile(fullString) &&
        (hasEnding(fullString, ".p8") || hasEnding(fullString, ".png"));
}

bool isCPostFile(const std::string &fullString) {
    return !isHiddenFile(fullString) &&
        fullString.rfind("cpost", 0) == 0;
}

