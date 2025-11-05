#include "Webserv.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "FileHandler.hpp"
#include "CGI_handler.hpp"
#include "UploadHandler.hpp"
#include "DeleteHandler.hpp"
#include "ConfigParser.hpp"
#include <fstream>

std::string intToString(int n)
{
    std::ostringstream oss;
    oss << n;
    return oss.str();
}

// Funzione helper: dice che tipo di fd è
enum FdType {
    FD_LISTEN,      // Socket che accetta connessioni
    FD_CLIENT,      // Socket cliente HTTP
    FD_CGI_INPUT,   // Pipe per scrivere VERSO CGI (stdin dello script)
    FD_CGI_OUTPUT   // Pipe per leggere DA CGI (stdout dello script)
};

//Updated getFdType to handle multiple listen_fds
FdType getFdType(int fd, const std::vector<int>& listen_fds, std::map<int, CgiProcess> &cgi_map)
{
    // Controlla se è uno dei listen socket
    for (size_t i = 0; i < listen_fds.size(); i++)
    {
        if (fd == listen_fds[i])
            return FD_LISTEN;
    }

    // È l'output di un CGI?
    if (cgi_map.count(fd) > 0)
        return FD_CGI_OUTPUT;

    // È l'input di un CGI?
    for (std::map<int, CgiProcess>::iterator it = cgi_map.begin(); it != cgi_map.end(); ++it)
    {
        if (it->second.pipe_in == fd)
            return FD_CGI_INPUT;
    }

    // Altrimenti è un client normale
    return FD_CLIENT;
}

//Route Matching - Trova la location per un path
LocationConfig* findLocation(ServerConfig& server, const std::string& request_path)
{
    // 1. Match esatto - priorità alta
    for (size_t i = 0; i < server.locations.size(); i++)
    {
        if (server.locations[i].path == request_path)
            return &server.locations[i];
    }

    // 2 Match prefix - per rotte come /uploads/file.txt
    for (size_t i = 0; i < server.locations.size(); i++)
    {
        std::string loc_path = server.locations[i].path;

        // Skip /
        if (loc_path == "/")
            continue;

        // Se request_path inizia con loc_path
        if (request_path.find(loc_path) == 0)
        {
            // Verifica che sia un match valido (/ o end of string)
            if (request_path.length() == loc_path.length() ||
                request_path[loc_path.length()] == '/')
            {
                return &server.locations[i];
            }
        }
    }

    // 3. Default location /
    for (size_t i = 0; i < server.locations.size(); i++)
    {
        if (server.locations[i].path == "/")
            return &server.locations[i];
    }

    // Fallback (shouldn't happen)
    return NULL;
}

// Load error page from file or return default
std::string getErrorPageContent(ServerConfig& server, int error_code)
{
    // Check if there's a custom error page configured
    if (server.error_pages.find(error_code) != server.error_pages.end())
    {
        std::string error_file = server.error_pages[error_code];
        std::string full_path = server.root + "/" + error_file;

        std::ifstream file(full_path.c_str());
        if (file.is_open())
        {
            std::string content((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
            file.close();
            return content;
        }
    }

    // Fallback to default error page
    std::string error_msg;
    switch (error_code)
    {
        case 404:
            error_msg = "Not Found";
            break;
        case 405:
            error_msg = "Method Not Allowed";
            break;
        case 413:
            error_msg = "Payload Too Large";
            break;
        case 500:
            error_msg = "Internal Server Error";
            break;
        case 400:
            error_msg = "Bad Request";
            break;
        default:
            error_msg = "Error";
            break;
    }

    return std::string("<html><body><h1>") + intToString(error_code) +
           std::string(" ") + error_msg + std::string("</h1></body></html>");
}

int main(int argc, char **argv)
{
    std::string config_file = "webserv.conf";

    if (argc > 1)
        config_file = argv[1];

    std::cout << "Initializing Webserv..." << std::endl;
    std::cout << "Loading configuration from: " << config_file << std::endl;

    // Parse configuration file
    ConfigParser parser(config_file);
    std::vector<ServerConfig> servers = parser.parse();

    if (servers.empty())
    {
        std::cerr << "Error: No server configuration found" << std::endl;
        return 1;
    }

    parser.displayServers();

    // Setup multiple ports instead of single server
    std::vector<int> listen_fds;
    std::map<int, size_t> fd_to_server_index;

    for (size_t s = 0; s < servers.size(); s++)
    {
        ServerConfig& srv = servers[s];

        int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_fd < 0)
        {
            std::cerr << "socket error for server " << s << std::endl;
            return 1;
        }

        int option = 1;
        if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0)
        {
            std::cerr << "setsockopt error for server " << s << std::endl;
            return 1;
        }

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(srv.listen_port);

        if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            std::cerr << "bind error for port " << srv.listen_port << std::endl;
            return 1;
        }

        if (listen(listen_fd, SOMAXCONN) < 0)
        {
            std::cerr << "listen error";
            return 1;
        }

        int flags = fcntl(listen_fd, F_GETFL, 0);
        fcntl(listen_fd, F_SETFL, flags | O_NONBLOCK);

        listen_fds.push_back(listen_fd);
        fd_to_server_index[listen_fd] = s;

        std::cout << "Listening on port " << srv.listen_port << " (server " << s << ")" << std::endl;
    }

    std::cout << "Webserv is running with " << servers.size() << " server(s)" << std::endl;

    // ========== STRUTTURE DATI ==========
    std::vector<struct pollfd> fds;
    std::map<int, std::string> send_buffers;    // fd → dati da inviare
    std::map<int, std::string> recv_buffers;    // fd → dati ricevuti

    // Mappa CGI: pipe_output → struttura CGI
    std::map<int, CgiProcess> cgi_map;
    // Mappa: pipe_output → fd_client (per sapere a chi inviare la risposta)
    std::map<int, int> cgi_client_map;
    // Mappa client_fd -> server_index (per sapere quale server usa ogni client)
    std::map<int, size_t> client_to_server;

    // Aggiungi tutti i listen_fds a poll
    for (size_t i = 0; i < listen_fds.size(); i++)
    {
        struct pollfd listen_poll = {listen_fds[i], POLLIN, 0};
        fds.push_back(listen_poll);
    }

    // ========== LOOP PRINCIPALE ==========
    while (1)
    {
        int ready = poll(fds.data(), fds.size(), -1);
        if (ready < 0)
        {
            std::cerr << "poll error" << std::endl;
            break;
        }

        // Itera su tutti i fd monitorati
        for (size_t i = 0; i < fds.size(); i++)
        {
            int fd = fds[i].fd;
            int revents = fds[i].revents;

            // ==========================================
            // GESTIONE ERRORI
            // ==========================================
            if (revents & (POLLERR | POLLHUP | POLLNVAL))
            {
                std::cerr << "Error on fd " << fd << ", closing..." << std::endl;

                FdType type = getFdType(fd, listen_fds, cgi_map);

                if (type == FD_CGI_OUTPUT)
                {
                    // Se è output CGI, chiudi anche input
                    CgiProcess &cgi = cgi_map[fd];
                    if (!cgi.stdin_closed)
                        close(cgi.pipe_in);
                    cgi_map.erase(fd);
                    cgi_client_map.erase(fd);
                }
                else if (type == FD_CGI_INPUT)
                {
                    // Se è input CGI, trova e chiudi anche output
                    for (std::map<int, CgiProcess>::iterator it = cgi_map.begin(); it != cgi_map.end(); ++it)
                    {
                        if (it->second.pipe_in == fd)
                        {
                            close(it->second.pipe_out);
                            int pipe_out = it->first;
                            cgi_map.erase(it);
                            cgi_client_map.erase(pipe_out);
                            break;
                        }
                    }
                }

                close(fd);
                send_buffers.erase(fd);
                recv_buffers.erase(fd);
                fds.erase(fds.begin() + i);
                i--; // Importante: compensa l'erase
                continue;
            }

            // ==========================================
            // CASO 1: NUOVO CLIENT (POLLIN su listen_fd)
            // ==========================================
            bool is_listen_socket = false;
            size_t current_server_index = 0;
            for (size_t j = 0; j < listen_fds.size(); j++)
            {
                if (fd == listen_fds[j])
                {
                    is_listen_socket = true;
                    current_server_index = fd_to_server_index[fd];
                    break;
                }
            }

            if (is_listen_socket && (revents & POLLIN))
            {
                int client_fd = accept(fd, NULL, NULL);
                if (client_fd < 0)
                {
                    std::cerr << "accept error" << std::endl;
                    continue;
                }

                // Rendi il client non-bloccante
                fcntl(client_fd, F_SETFL, O_NONBLOCK);

                // Aggiungi a poll per monitorare lettura
                struct pollfd client_poll = {client_fd, POLLIN, 0};
                fds.push_back(client_poll);

                // FIX #3: Track which server this client is connected to
                client_to_server[client_fd] = current_server_index;

                std::cout << "New client connected: fd=" << client_fd << " (server " << current_server_index << ")" << std::endl;
                continue;
            }

            // ==========================================
            // CASO 2: LETTURA DA CGI OUTPUT (POLLIN)
            // ==========================================
            if ((revents & POLLIN) && cgi_map.count(fd) > 0)
            {
                CgiProcess &cgi = cgi_map[fd];

                // Leggi dati dallo stdout del CGI
                char buf[1024];
                ssize_t n = read(fd, buf, sizeof(buf));

                if (n > 0)
                {
                    cgi.read_buffer.append(buf, n);
                    std::cout << "Read " << n << " bytes from CGI" << std::endl;
                }
                else if (n == 0)
                {
                    std::cout << "CGI closed output pipe" << std::endl;
                }

                // Trasferisci output al buffer del client
                int client_fd = cgi_client_map[fd];
                send_buffers[client_fd] += cgi.read_buffer;
                cgi.read_buffer.clear();

                // Abilita scrittura verso client
                for (size_t j = 0; j < fds.size(); j++)
                {
                    if (fds[j].fd == client_fd)
                    {
                        fds[j].events |= POLLOUT;
                        break;
                    }
                }

                // Controlla se il processo CGI è terminato
                int status;
                pid_t result = waitpid(cgi.pid, &status, WNOHANG);
                if (result > 0)
                {
                    std::cout << "CGI process finished" << std::endl;

                    // Chiudi pipe
                    close(fd);
                    if (!cgi.stdin_closed)
                        close(cgi.pipe_in);

                    // Rimuovi da mappe
                    cgi_map.erase(fd);
                    cgi_client_map.erase(fd);

                    // Rimuovi da fds
                    fds.erase(fds.begin() + i);
                    i--;
                }

                continue;
            }

            // ==========================================
            // CASO 3: SCRITTURA VERSO CGI INPUT (POLLOUT)
            // ==========================================
            if (revents & POLLOUT)
            {
                FdType type = getFdType(fd, listen_fds, cgi_map);

                if (type == FD_CGI_INPUT)
                {
                    // Trova il CgiProcess associato
                    CgiProcess *cgi_ptr = NULL;
                    for (std::map<int, CgiProcess>::iterator it = cgi_map.begin(); it != cgi_map.end(); ++it)
                    {
                        if (it->second.pipe_in == fd)
                        {
                            cgi_ptr = &(it->second);
                            break;
                        }
                    }

                    if (cgi_ptr && !cgi_ptr->stdin_closed && !cgi_ptr->write_buffer.empty())
                    {
                        // Scrivi dati verso stdin del CGI
                        ssize_t written = write(fd, cgi_ptr->write_buffer.c_str(), cgi_ptr->write_buffer.size());

                        if (written > 0)
                        {
                            std::cout << "Wrote " << written << " bytes to CGI" << std::endl;
                            cgi_ptr->write_buffer.erase(0, written);
                        }

                        // Se ho finito di scrivere, chiudi stdin
                        if (cgi_ptr->write_buffer.empty())
                        {
                            close(fd);
                            cgi_ptr->stdin_closed = true;
                            std::cout << "CGI stdin closed" << std::endl;

                            // Rimuovi POLLOUT da questo fd
                            fds[i].events &= ~POLLOUT;
                        }
                    }

                    continue; // Vai al prossimo fd
                }
            }

            // ==========================================
            // CASO 4: LETTURA DA CLIENT (POLLIN)
            // ==========================================
            if (revents & POLLIN)
            {
                FdType type = getFdType(fd, listen_fds, cgi_map);

                if (type == FD_CLIENT)
                {
                    char buf[1024];
                    int bytes_received = recv(fd, buf, sizeof(buf) - 1, 0);

                    if (bytes_received <= 0)
                    {
                        // Client disconnesso
                        std::cout << "Client fd=" << fd << " disconnected" << std::endl;
                        close(fd);
                        send_buffers.erase(fd);
                        recv_buffers.erase(fd);
                        fds.erase(fds.begin() + i);
                        i--;
                        continue;
                    }

                    buf[bytes_received] = '\0';
                    recv_buffers[fd] += std::string(buf, bytes_received);

                    // Controlla se la richiesta è completa
                    size_t header_end = recv_buffers[fd].find("\r\n\r\n");
                    if (header_end == std::string::npos)
                        continue; // Header incompleto, aspetta altri dati

                    // Parsing Content-Length e Transfer-Encoding
                    std::string headers_part = recv_buffers[fd].substr(0, header_end + 4);
                    size_t content_length = 0;
                    bool has_content_length = false;
                    bool is_chunked = false;

                    size_t cl_pos = headers_part.find("Content-Length:");
                    if (cl_pos != std::string::npos)
                    {
                        cl_pos += 15;
                        while (cl_pos < headers_part.size() && (headers_part[cl_pos] == ' ' || headers_part[cl_pos] == '\t'))
                            cl_pos++;
                        size_t end_pos = headers_part.find("\r\n", cl_pos);
                        if (end_pos != std::string::npos)
                        {
                            std::string value = headers_part.substr(cl_pos, end_pos - cl_pos);
                            content_length = std::strtoul(value.c_str(), NULL, 10);
                            has_content_length = true;
                        }
                    }

                    size_t te_pos = headers_part.find("Transfer-Encoding:");
                    if (te_pos != std::string::npos)
                    {
                        size_t line_end = headers_part.find("\r\n", te_pos);
                        std::string value = headers_part.substr(te_pos, line_end - te_pos);
                        if (value.find("chunked") != std::string::npos)
                            is_chunked = true;
                    }

                    // Verifica se richiesta completa
                    bool request_complete = false;
                    if (is_chunked)
                    {
                        if (recv_buffers[fd].find("0\r\n\r\n", header_end + 4) != std::string::npos)
                            request_complete = true;
                    }
                    else if (has_content_length)
                    {
                        if (recv_buffers[fd].size() >= header_end + 4 + content_length)
                            request_complete = true;
                    }
                    else
                    {
                        request_complete = true;
                    }

                    if (!request_complete)
                        continue;

                    // Estrai richiesta completa
                    std::string full_request;
                    if (is_chunked)
                    {
                        size_t end = recv_buffers[fd].find("0\r\n\r\n", header_end + 4);
                        full_request = recv_buffers[fd].substr(0, end + 5);
                        recv_buffers[fd].erase(0, end + 5);
                    }
                    else if (has_content_length)
                    {
                        size_t total = header_end + 4 + content_length;
                        full_request = recv_buffers[fd].substr(0, total);
                        recv_buffers[fd].erase(0, total);
                    }
                    else
                    {
                        full_request = recv_buffers[fd].substr(0, header_end + 4);
                        recv_buffers[fd].erase(0, header_end + 4);
                    }

                    // PARSING RICHIESTA HTTP
                    HttpRequest request;
                    request.parse(full_request);
                    Response response;

                    //Get the correct server config for this client
                    size_t server_idx = 0;
                    if (client_to_server.count(fd) > 0)
                        server_idx = client_to_server[fd];
                    ServerConfig& server_config = servers[server_idx];

                    if (!request.isValid())
                    {
                        response.setStatus(request.getErrorCode());
                        response.addHeader("Content-Type", "text/html");
                        std::string error_body = getErrorPageContent(server_config, request.getErrorCode());
                        response.setBody(error_body);
                        send_buffers[fd] = response.generate();
                        fds[i].events = POLLOUT;
                        continue;
                    }

                    std::cout << "Request: " << request.getMethod() << " " << request.getPath() << std::endl;

                    // Trova la location per validare i metodi permessi
                    LocationConfig* loc = findLocation(server_config, request.getPath());

                    // Valida se il metodo è permesso per questa location
                    if (loc != NULL)
                    {
                        bool method_allowed = false;
                        for (size_t m = 0; m < loc->allowed_methods.size(); m++)
                        {
                            if (loc->allowed_methods[m] == request.getMethod())
                            {
                                method_allowed = true;
                                break;
                            }
                        }

                        if (!method_allowed)
                        {
                            std::cout << "Method " << request.getMethod() << " not allowed for " << request.getPath() << std::endl;
                            response.setStatus(405);
                            response.addHeader("Content-Type", "text/html");
                            response.addHeader("Allow", "GET, POST"); // TODO: lista dinamica
                            std::string error_body = getErrorPageContent(server_config, 405);
                            response.setBody(error_body);
                            send_buffers[fd] = response.generate();
                            fds[i].events = POLLOUT;
                            continue;
                        }
                    }

                    // Determina se è CGI
                    std::string path = request.getPath();
                    bool is_cgi = (path.find("/cgi-bin/") != std::string::npos ||
                                   path.find(".py") != std::string::npos ||
                                   path.find(".php") != std::string::npos ||
                                   path.find(".cgi") != std::string::npos);

                    if (is_cgi)
                    {
                        // GESTIONE CGI
                        try
                        {
                            CgiProcess cgi = CGIHandler::spawnCgi(request, path, "./www");

                            // Registra nelle mappe
                            cgi_map[cgi.pipe_out] = cgi;
                            cgi_client_map[cgi.pipe_out] = fd;

                            // Aggiungi pipe_out per leggere output
                            struct pollfd cgi_out = {cgi.pipe_out, POLLIN, 0};
                            fds.push_back(cgi_out);

                            // Se c'è body da inviare, aggiungi pipe_in per scrittura
                            if (!cgi.write_buffer.empty())
                            {
                                struct pollfd cgi_in = {cgi.pipe_in, POLLOUT, 0};
                                fds.push_back(cgi_in);
                            }
                            else
                            {
                                // Nessun body, chiudi subito stdin
                                close(cgi.pipe_in);
                                cgi_map[cgi.pipe_out].stdin_closed = true;
                            }

                            std::cout << "CGI spawned: pipe_in=" << cgi.pipe_in
                                    << ", pipe_out=" << cgi.pipe_out << std::endl;
                        }
                        catch (const std::exception &e)
                        {
                            response.setStatus(500);
                            response.addHeader("Content-Type", "text/html");
                            std::string error_body = getErrorPageContent(server_config, 500);
                            response.setBody(error_body);
                            send_buffers[fd] = response.generate();
                            fds[i].events = POLLOUT;
                        }
                    }
                    else if (request.getMethod() == "GET")
                    {
                        //Usa findLocation per determinare root
                        LocationConfig* loc = findLocation(server_config, request.getPath());
                        std::string root = (loc != NULL) ? loc->root : server_config.root;

                        std::cout << "DEBUG GET: loc=" << (loc ? loc->path : "NULL") << ", root=" << root << std::endl;

                        // Rimuovi il prefisso della location dal path se necessario
                        std::string file_path = request.getPath();
                        if (loc != NULL && loc->path != "/" && file_path.find(loc->path) == 0)
                        {
                            // Rimuovi il prefisso della location
                            file_path = file_path.substr(loc->path.length());
                            if (file_path.empty() || file_path[0] != '/')
                                file_path = "/" + file_path;
                            std::cout << "Location match: " << loc->path << ", adjusted path: " << file_path << ", root: " << root << std::endl;
                        }

                        // FILE STATICO
                        response = FileHandler::handleWithPath(request, root, file_path);

                        // Se è un errore, usa custom error page
                        if (response.getStatus() >= 400)
                        {
                            std::string error_body = getErrorPageContent(server_config, response.getStatus());
                            response.setBody(error_body);
                        }

                        send_buffers[fd] = response.generate();
                        fds[i].events = POLLOUT;
                    }
                    else if (request.getMethod() == "POST")
                    {
                        //Usa findLocation per determinare root
                        LocationConfig* loc = findLocation(server_config, request.getPath());
                        std::string root = (loc != NULL) ? loc->root : "./uploads";

                        response = UploadHandler::handle(request, root);
                        send_buffers[fd] = response.generate();
                        fds[i].events = POLLOUT;
                    }
                    else if (request.getMethod() == "DELETE")
                    {
                        // Usa findLocation per determinare root
                        LocationConfig* loc = findLocation(server_config, request.getPath());
                        std::string root = (loc != NULL) ? loc->root : "./uploads";

                        response = DeleteHandler::handle(request, root);
                        send_buffers[fd] = response.generate();
                        fds[i].events = POLLOUT;
                    }
                    else
                    {
                        response.setStatus(405);
                        response.addHeader("Content-Type", "text/html");
                        response.setBody("<html><body><h1>405 Method Not Allowed</h1></body></html>");
                        send_buffers[fd] = response.generate();
                        fds[i].events = POLLOUT;
                    }

                    continue;
                }
            }

            // ==========================================
            // CASO 5: SCRITTURA VERSO CLIENT (POLLOUT)
            // ==========================================
            if (revents & POLLOUT)
            {
                FdType type = getFdType(fd, listen_fds, cgi_map);

                if (type == FD_CLIENT)
                {
                    if (send_buffers.find(fd) != send_buffers.end())
                    {
                        std::string &buf = send_buffers[fd];
                        int bytes_sent = send(fd, buf.c_str(), buf.size(), 0);

                        if (bytes_sent < 0)
                        {
                            // In modalità non-blocking, -1 è normale (riprova più tardi)
                            // Non serve controllare errno
                            continue;
                        }

                        if (bytes_sent == 0)
                        {
                            // Socket chiuso
                            std::cerr << "Client fd=" << fd << " closed connection" << std::endl;
                            close(fd);
                            send_buffers.erase(fd);
                            recv_buffers.erase(fd);
                            fds.erase(fds.begin() + i);
                            i--;
                            continue;
                        }

                        if (bytes_sent < (int)buf.size())
                        {
                            // Invio parziale
                            buf.erase(0, bytes_sent);
                        }
                        else
                        {
                            // Tutto inviato
                            buf.clear();
                            fds[i].events &= ~POLLOUT; // Disabilita POLLOUT
                        }
                    }

                    continue;
                }
            }
        } // Fine loop su fds

    } // Fine while(1)

    // Close all listening sockets
    for (size_t i = 0; i < listen_fds.size(); i++)
    {
        close(listen_fds[i]);
    }

    return 0;
}
