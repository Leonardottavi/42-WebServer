#ifndef FILEHANDLER_HPP
#define FILEHANDLER_HPP

#include "Request.hpp"
#include "Response.hpp"
#include <string>
#include <vector>

class FileHandler
{
    public:
        static Response handle(const HttpRequest& res, const std::string& root_dir);
        static Response handleWithPath(const HttpRequest& res, const std::string& root_dir, const std::string& file_path);
        static Response listDirectoryFiles(const HttpRequest& res, const std::string& root_dir, const std::string& dir_path);
    private:
        static std::string getContentType(const std::string &path);
        static std::string readFile(const std::string &path);
        static bool fileExists(const std::string &path);
        static bool canRead(const std::string& path);
        static std::vector<std::string> getFilesInDirectory(const std::string& dir_path);
};

#endif
