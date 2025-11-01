#ifndef DELETEHANDLER_HPP
#define DELETEHANDLER_HPP

#include "Request.hpp"
#include "Response.hpp"
#include <string>

class DeleteHandler
{
public:
    static Response handle(const HttpRequest& req, const std::string& root_dir);
private:
    static bool deleteFile(const std::string& filepath);
};

#endif