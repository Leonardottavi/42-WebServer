#include "DeleteHandler.hpp"
#include <unistd.h>
#include <sys/stat.h>

Response DeleteHandler::handle(const HttpRequest &req, const std::string &root_dir)
{
    Response resp(req);
    std::string filepath = root_dir + req.getPath();
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
    return resp;
}

bool DeleteHandler::deleteFile(const std::string& filepath)
{
    return (unlink(filepath.c_str()) == 0);
    //unlink () est un appel syst de unix pour delet e un file avec valeur de succes retourner si echec ou non 
}