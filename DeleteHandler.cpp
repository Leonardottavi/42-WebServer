#include "DeleteHandler.hpp"
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>

Response DeleteHandler::handle(const HttpRequest &req, const std::string &root_dir)
{
    Response resp(req);

    // Extract relative path from request path
    std::string request_path = req.getPath();
    std::string relative_path = request_path;

    std::cerr << "DELETE DEBUG: request_path=" << request_path << ", root_dir=" << root_dir << std::endl;

    // If root_dir contains "/files", remove "/files" prefix from request_path
    if (request_path.find("/files") == 0)
        relative_path = request_path.substr(6); // Remove "/files"
    else if (request_path.find("/uploads") == 0)
        relative_path = request_path.substr(8); // Remove "/uploads"
    else if (request_path.find("/upload") == 0)
        relative_path = request_path.substr(7); // Remove "/upload"

    std::string filepath = root_dir + relative_path;
    std::cerr << "DELETE DEBUG: relative_path=" << relative_path << ", final filepath=" << filepath << std::endl;
    if (!req.fileExists(filepath))
    {
        resp.setStatus(404);
        resp.addHeader("Content-Type", "text/html");
        resp.setBody("<html><body><h1>404 Not Found</h1><p>File does not exist</p></body></html>");
        return resp;
    }
    if (!req.isRegularFile(filepath))
    {
        resp.setStatus(403);
        resp.addHeader("Content-Type", "text/html");
        resp.setBody("<html><body><h1>403 Forbidden</h1><p>Cannot delete directories</p></body></html>");
        return resp;
    }
    if (!req.canWrite(filepath))
    {
        resp.setStatus(403);
        resp.addHeader("Content-Type", "text/html");
        resp.setBody("<html><body><h1>403 Forbidden</h1><p>No permission to delete this file</p></body></html>");
        return resp;
    }
    if (deleteFile(filepath))
    {
        resp.setStatus(204);
        resp.addHeader("Content-Type", "text/html");
        resp.setBody("<html><body><h1>✅ File Deleted</h1><p>File deleted successfully: " + req.getPath() + "</p></body></html>");
        std::cout << "File deleted successfully: " << filepath << std::endl;
    }
    else
    {
        resp.setStatus(500);
        resp.addHeader("Content-Type", "text/html");
        resp.setBody("<html><body><h1>500 Internal Server Error</h1><p>Failed to delete file: " + req.getPath() + "</p></body></html>");
        std::cerr << "Failed to delete file: " << filepath << std::endl;
    }
    return resp;
}

bool DeleteHandler::deleteFile(const std::string& filepath)
{
    return (unlink(filepath.c_str()) == 0);
    //unlink () est un appel syst de unix pour delet e un file avec valeur de succes retourner si echec ou non
}
