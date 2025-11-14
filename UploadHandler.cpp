/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   UploadHandler.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lottavi <lottavi@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/14 09:52:15 by lottavi           #+#    #+#             */
/*   Updated: 2025/11/14 09:52:17 by lottavi          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "UploadHandler.hpp"

static std::string intToString(int n)
{
    std::ostringstream oss;
    oss << n;
    return oss.str();
}

Response UploadHandler::handle(const HttpRequest& req, const std::string &upload_dir)
{
    Response resp(req);
    if (!ensureDirectoryExists(upload_dir))
    {
        resp.setStatus(500);
        resp.addHeader("Content-Type", "text/html");
        resp.setBody("<html><body><h1>500 Internal Server Error</h1></body></html>");
        return (resp);
    }
    std::vector<std::string> file_names = req.getFileNames();
    if (file_names.empty())
    {
        resp.setStatus(400);
        resp.addHeader("Content-Type", "text/html");
        resp.setBody("<html><body><h1>400 Bad Request</h1><p>No files uploaded</p></body></html>");
        return resp;
    }
    int success_count = 0;
    int total_count = 0;
    std::string result_html = "<html><body><h1>Upload Results</h1><ul>";
    for (size_t i = 0; i < file_names.size(); i++)
    {
        std::vector<ParsedFile> files = req.getFiles(file_names[i]);
        for (size_t j = 0; j < files.size(); j++)
        {
            total_count++;
            if (saveFile(files[j], upload_dir))
            {
                result_html += "<li> <b>" + files[j].filename + "</b> uploaded ("
                + intToString(files[j].size) + " bytes)</li>";
                success_count++;
            }
            else
                result_html += "<li><b>" + files[j].filename + "</b> failed</li>";
        }
    }
    result_html += "</ul></body></html>";
    if (success_count == (int)file_names.size())
        resp.setStatus(201);
    else if (success_count > 0)
        resp.setStatus(207);
    else
        resp.setStatus(500);
    resp.addHeader("Content-Type", "text/html");
    resp.setBody(result_html);

    return resp;
}

bool UploadHandler::ensureDirectoryExists(const std::string &dir)
{
    struct stat st;
    if (stat(dir.c_str(), &st) == 0)
        return (S_ISDIR(st.st_mode));
    return (mkdir(dir.c_str(), 0755) == 0);
}

bool UploadHandler::saveFile(const ParsedFile& file, const std::string& dir)
{

    std::string filepath = dir + "/" + file.filename;

    std::ofstream out(filepath.c_str(), std::ios::binary);
    if (!out.is_open())
        return false;
    out.write(file.data.c_str(), file.size);
    out.close();
    return !out.fail();
}
