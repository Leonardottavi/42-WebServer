/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FileHandler.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lottavi <lottavi@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/14 10:09:51 by lottavi           #+#    #+#             */
/*   Updated: 2025/11/14 10:09:52 by lottavi          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "FileHandler.hpp"
#include "ConfigParser.hpp"  // Per accedere a ServerConfig
#include <dirent.h>
#include <sstream>

 Response FileHandler::handle(const HttpRequest& res, const std::string& root_dir)
{
    return handleWithPath(res, root_dir, res.getPath());
}

 Response FileHandler::handleWithPath(const HttpRequest& res, const std::string& root_dir, const std::string& file_path)
{
    Response resp(res);
    std::string path = root_dir + file_path; // construction du path complet

    if (path[path.length() - 1] == '/')
        path += "index.html";// ajout de index html apres / si trouver
    if (!fileExists(path))
    {
        resp.setStatus(404);
        resp.addHeader("Content-Type", "text/html");
        resp.setBody("<html><body><h1>404 Not Found</h1></body></html>");
        return resp;
    }
    if (!canRead(path))
    {
        resp.setStatus(403);
        resp.addHeader("Content-Type", "text/html");
        resp.setBody("<html><body><h1>Forbidden</h1></body></html>");
        return resp;
    }
    std::string content = readFile(path);
    //lire file

    //reponse positive
    resp.setStatus(200);
    resp.addHeader("Content-Type", getContentType(path));
    resp.setBody(content);
    return resp;
}


 std::string FileHandler::getContentType(const std::string &path)
{
    if (path.find(".html") != std::string::npos)
        return "text/html";
    if (path.find(".css") != std::string::npos)
        return ("text/css");
    if (path.find(".js") != std::string::npos)
        return ("application/javascript");
    if (path.find(".png") != std::string::npos)
        return ("image/png");
    if (path.find(".jpg") != std::string::npos)
        return ("image/jpeg");
    if (path.find(".gif") != std::string::npos)
        return ("image/gif");
    return "application/octet-stream";
}

 std::string FileHandler::readFile(const std::string &path)
{
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open())
        return "";
    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

 bool FileHandler::fileExists(const std::string &path)
{
    return (access(path.c_str(), F_OK) == 0);
}

 bool FileHandler::canRead(const std::string& path)
{
    return (access(path.c_str(), R_OK) == 0);
}

std::vector<std::string> FileHandler::getFilesInDirectory(const std::string& dir_path)
{
    std::vector<std::string> files;
    DIR* dir = opendir(dir_path.c_str());

    if (!dir)
        return files;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL)
    {
        std::string filename = entry->d_name;

        // Skip . and ..
        if (filename == "." || filename == "..")
            continue;

        std::string fullPath = dir_path + "/" + filename;

        // Only add files, not directories
        if (entry->d_type == DT_REG && canRead(fullPath))
            files.push_back(filename);
    }

    closedir(dir);
    return files;
}

Response FileHandler::listDirectoryFiles(const HttpRequest& res, const std::string& root_dir, const std::string& dir_path)
{
    Response resp(res);
    std::string fullPath = root_dir + dir_path;

    // Normalizza il percorso (remove trailing slashes per dirent)
    if (fullPath.length() > 1 && fullPath[fullPath.length() - 1] == '/')
        fullPath = fullPath.substr(0, fullPath.length() - 1);

    if (!fileExists(fullPath))
    {
        resp.setStatus(404);
        resp.addHeader("Content-Type", "application/json");
        resp.setBody("{\"error\": \"Directory not found\"}");
        return resp;
    }

    if (!canRead(fullPath))
    {
        resp.setStatus(403);
        resp.addHeader("Content-Type", "application/json");
        resp.setBody("{\"error\": \"Access denied\"}");
        return resp;
    }

    std::vector<std::string> files = getFilesInDirectory(fullPath);

    // Build JSON response
    std::stringstream json;
    json << "{\"files\": [";

    for (size_t i = 0; i < files.size(); ++i)
    {
        json << "\"" << files[i] << "\"";
        if (i < files.size() - 1)
            json << ", ";
    }

    json << "]}";

    resp.setStatus(200);
    resp.addHeader("Content-Type", "application/json");
    resp.setBody(json.str());
    return resp;
}

