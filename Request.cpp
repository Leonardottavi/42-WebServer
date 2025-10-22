#include "Request.hpp"

Request::Request(){}

void Request::parse(std::string request)
{
    //on converti en stream la str pour eviter direct les 
    //espace entre les infos
    std::istringstream stream(request);
    std::string line;

    //on va lire la premier ligne
    std::getline(stream, line);
    //std::cout << "DDEBUG line='" << line << "'" << std::endl;

    if (!line.empty() && line[line.size() - 1] == '\r')
    {
        //on enleve le \r qui termine une ligne de request
        line.erase(line.size() - 1);
    }
    
    std::istringstream line_stream(line);
    //creation de lobjet linestream qui lit line 
    line_stream >> method >> uri >> version;
    validateMethod();
    validateVersion();
    validateUri();
    // >> saute automatiquement les espaces , mais stock ce que tu trouve avant dant >> method
    // puis dans >> uri puis dans version

    while (std::getline(stream, line))
    {
        //pour les headers maintenant
        //supprimer les \r dabbord si ligne pas empty
        if (line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        if (line.empty())
            break;
        size_t sep_pos = line.find(':');
        if (sep_pos == std::string::npos)
            continue;
        //npos c une valeur retourner pour dire non found
        //car find retourn tj une valeur
        std::string key = line.substr(0, sep_pos);
        std::string value = line.substr(sep_pos + 1);
        size_t start = value.find_first_not_of(" \t\r\n");
        //trouve la pos du premier char qui n'est pas espace ou tab
        // pour ne pas remplie les value dans headers avec espace / tab
        if (start != std::string::npos)
            value = value.substr(start);
        else
            value = "";
        size_t end = value.find_last_not_of(" \t\r\n");
        if (end != std::string::npos)
            value = value.substr(0, end + 1);
        else
            value = "";
        key = toLower(key);
        headers[key] = value;
    }
    validateHost();
    std::string body_contain;
    char c;
    while (stream.get(c))
        body_contain += c;
    body = body_contain;
    validateContentLength();
}

void Request::validateContentLength()
{
    std::map<std::string, std::string>::iterator it = headers.find("content-length");
    if (it != headers.end())
    {
        has_content_length = true;
        std::istringstream input_strring_stream(it->second);
        //devrait sappeler iss ( convention)
        // pour convertir content length en valeur int

        
        //stock la valeur trouver it (length) converti en stream dans input_strring_stream 
        //
        if (!(input_strring_stream >> content_length))
        {
            std::cerr << "Error: Invalid Content-Length format" << std::endl;
            throw std::runtime_error("400 Bad Request: Invalid Content-Length");
        }
        if (content_length > MAX_BODY_SIZE)
        {
            std::cerr   << "Error: Content-Length too large (" << content_length
                        << " > " << MAX_BODY_SIZE << ")" << std::endl;
            throw std::runtime_error("413 Payload Too Large");
        }
        if (body.length() != content_length)
        {
            std::cerr << "Error: Content-Length mismatch!" << std::endl;
            std::cerr << "  Declared in header: " << content_length << std::endl;
            std::cerr << "  Actual body length: " << body.length() << std::endl;
            throw std::runtime_error("400 Bad Request: Content-Length mismatch");
        }
        std::cout << "Content-Length validated: " << content_length << " bytes" << std::endl;
    }
    else
    {
        has_content_length = false;
        content_length = 0;
        if (body.length() > MAX_BODY_SIZE)
        {
            std::cerr   << "Error: Body too large without Content-Length(" << body.length()
                        << " > " << MAX_BODY_SIZE << ")" << std::endl;
            throw std::runtime_error("413 Payload Too Large");
        }
        if ((method == "POST" || method == "PUT") && !body.empty())
        {
            std::cerr << "Warming: POST/PUT request without Content-Length" << std::endl;
            // throw une erreur ???
        }
    }
}

std::string Request::getMethod()const
{
    return method;
}

std::string Request::getUri()const
{
    return uri;
}

std::string Request::getVersion()const
{
    return version;
}

std::string Request::getBody()const
{
    return body;
}

std::string Request::getHeader(std::string param)const
{
    std::string key_lower = toLower(param);
    std::map<std::string, std::string>::const_iterator it = headers.find(key_lower);
    if (it != headers.end())
        return it->second;
    return "";
}

void Request::displayHeaders()const
{
    std::map<std::string, std::string>::const_iterator it;
    for (it = headers.begin(); it != headers.end(); ++it)
    {
        std::cout << it->first << ": " << it->second << std::endl;
    }
}

size_t Request::getContentLength() const
{
    return content_length;
}

bool Request::hasContentLength()const
{
   return has_content_length; 
}

std::string Request::toLower(const std::string& str)const
{
    std::string cont_len_lower = str;
    for (size_t i = 0; i < str.size(); i++)
    {
        if (cont_len_lower[i] >= 'A' && cont_len_lower[i] <= 'Z')
            cont_len_lower[i] = cont_len_lower[i] + 32;
    }
    return cont_len_lower;
}

void Request::validateMethod()
{
    if (method.empty())
    {
        std::cerr << "Error: Method is empty" << std::endl;
        throw std::runtime_error("400 Bad Request: No method specified");
    }
    if (!isMethodValid(method))
    {
        std::cerr << "Error: Invalid HTTP method: " << method << std::endl; 
        throw std::runtime_error("405 Method Not Allowed");
    }
    std::cout << "Method validated: " << method << std::endl;
}

bool Request::isMethodValid(const std::string& method)const
{
    if (method == "GET" || method == "POST" || method == "DELETE")
        return true;
    //je dois gerer plus ???
    return false;
}

void Request::validateVersion()
{
    if (version.empty())
    {
        std::cerr << "Error: HTTP version is empty" << std::endl;
        throw std::runtime_error("400 Bad Request: No HTTP version specified");
    }
    if (version != "HTTP/1.0" && version != "HTTP/1.1")
    {
        std::cerr << "Error: Unsupported HTTP Version: " << version << std::endl;
        throw std::runtime_error("505 HTTP Version Not Supported");
    }
    std::cout << "Version validated" << std::endl;
}

void Request::validateUri()
{
    const size_t max_uri_len = 8192;
    if (uri.empty())
    {
        std::cerr << "Error: URI is empty" << std::endl;
        throw std::runtime_error("400 Bad Request: No URI specified");
    }
    if (uri.size() > max_uri_len)
    {
        std::cerr   << "Error: URI too long (" << uri.length() 
                    << " > " << max_uri_len <<std::endl;
        throw std::runtime_error("414 URI Too long");
    }
    if (uri[0] != '/')
        {
            std::cerr << "Error: URI must start with '/': " << uri << std::endl;
            throw std::runtime_error("400 Bad Request: Invalid URI format");
        }
    std::cout << "URI validated: " << uri << std::endl;

}

void Request::validateHost()
{
    if (version == "HTTP/1.1")
    {
        std::map<std::string, std::string>::iterator it = headers.find("host");
        if (it == headers.end())
        //si host nexist pas
        {
            std::cerr << "Error: Host header missing (required for HTTP/1.1)" << std::endl;
            throw std::runtime_error("400 Bad Request: Host header required");
        }
        if (it->second.empty())
        {
            std::cerr << "Error: Host header is empty" << std::endl;
            throw std::runtime_error("400 Bad Request: Host header cannot be empty");
        }
        std::cout << "Host validated: " << it->second << std::endl;
    }
    else
    {
        std::cout << "Host validation skipped(HTTP/1.0)" << std::endl;
    }
}
