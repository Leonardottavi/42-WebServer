#include "ConfigParser.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cctype>

ConfigParser::ConfigParser(const std::string& file) : config_file(file) {}

ConfigParser::~ConfigParser() {}

std::string ConfigParser::trim(const std::string& str)
{
	size_t start = str.find_first_not_of(" \t\r\n");
	if (start == std::string::npos)
		return "";
	size_t end = str.find_last_not_of(" \t\r\n");
	return str.substr(start, end - start + 1);
}

std::string ConfigParser::toLower(const std::string& str)
{
	std::string result = str;
	for (size_t i = 0; i < result.length(); i++)
	{
		result[i] = std::tolower(result[i]);
	}
	return result;
}

std::vector<std::string> ConfigParser::split(const std::string& str, char delimiter)
{
	std::vector<std::string> tokens;
	std::stringstream ss(str);
	std::string token;

	while (std::getline(ss, token, delimiter))
	{
		token = trim(token);
		if (!token.empty())
			tokens.push_back(token);
	}
	return tokens;
}

bool ConfigParser::isServerBlock(const std::string& line)
{
	std::string trimmed = trim(line);
	return trimmed.find("server") == 0 && trimmed.find("{") != std::string::npos;
}

bool ConfigParser::isLocationBlock(const std::string& line)
{
	std::string trimmed = trim(line);
	return trimmed.find("location") == 0 && trimmed.find("{") != std::string::npos;
}

void ConfigParser::parseDirective(ServerConfig& server, const std::string& line)
{
	std::string trimmed = trim(line);

	if (trimmed.empty() || trimmed[0] == '#')
		return;

	// Rimuovi il ; finale se presente
	if (trimmed[trimmed.length() - 1] == ';')
		trimmed = trimmed.substr(0, trimmed.length() - 1);

	std::vector<std::string> parts = split(trimmed, ' ');

	if (parts.empty())
		return;

	std::string directive = toLower(parts[0]);

	if (directive == "listen" && parts.size() > 1)
	{
		server.listen_port = std::atoi(parts[1].c_str());
	}
	else if (directive == "server_name" && parts.size() > 1)
	{
		server.server_name = parts[1];
	}
	else if (directive == "root" && parts.size() > 1)
	{
		server.root = parts[1];
	}
	else if (directive == "index" && parts.size() > 1)
	{
		server.index = parts[1];
	}
	else if (directive == "client_max_body_size" && parts.size() > 1)
	{
		std::string size_str = parts[1];
		if (size_str[size_str.length() - 1] == 'M' || size_str[size_str.length() - 1] == 'm')
		{
			size_str = size_str.substr(0, size_str.length() - 1);
			server.client_max_body_size = std::atol(size_str.c_str()) * 1024 * 1024;
		}
		else if (size_str[size_str.length() - 1] == 'K' || size_str[size_str.length() - 1] == 'k')
		{
			size_str = size_str.substr(0, size_str.length() - 1);
			server.client_max_body_size = std::atol(size_str.c_str()) * 1024;
		}
		else
		{
			server.client_max_body_size = std::atol(size_str.c_str());
		}
	}
	else if (directive == "error_page" && parts.size() > 2)
	{
		int error_code = std::atoi(parts[1].c_str());
		std::string error_page = parts[2];
		server.error_pages[error_code] = error_page;
	}
}

void ConfigParser::parseLocationDirective(LocationConfig& location, const std::string& line)
{
	std::string trimmed = trim(line);

	if (trimmed.empty() || trimmed[0] == '#')
		return;

	// Rimuovi il ; finale se presente
	if (trimmed[trimmed.length() - 1] == ';')
		trimmed = trimmed.substr(0, trimmed.length() - 1);

	std::vector<std::string> parts = split(trimmed, ' ');

	if (parts.empty())
		return;

	std::string directive = toLower(parts[0]);

	if (directive == "allow_methods")
	{
		for (size_t i = 1; i < parts.size(); i++)
		{
			location.allowed_methods.push_back(parts[i]);
		}
	}
	else if (directive == "root" && parts.size() > 1)
	{
		location.root = parts[1];
	}
	else if (directive == "index" && parts.size() > 1)
	{
		location.index = parts[1];
	}
	else if (directive == "cgi_enabled" && parts.size() > 1)
	{
		location.cgi_enabled = (parts[1] == "on" || parts[1] == "yes" || parts[1] == "true");
	}
	else if (directive == "cgi_extension" && parts.size() > 1)
	{
		location.cgi_extension = parts[1];
	}
	else if (directive == "client_max_body_size" && parts.size() > 1)
	{
		std::string size_str = parts[1];
		if (size_str[size_str.length() - 1] == 'M' || size_str[size_str.length() - 1] == 'm')
		{
			size_str = size_str.substr(0, size_str.length() - 1);
			location.client_max_body_size = std::atol(size_str.c_str()) * 1024 * 1024;
		}
		else if (size_str[size_str.length() - 1] == 'K' || size_str[size_str.length() - 1] == 'k')
		{
			size_str = size_str.substr(0, size_str.length() - 1);
			location.client_max_body_size = std::atol(size_str.c_str()) * 1024;
		}
		else
		{
			location.client_max_body_size = std::atol(size_str.c_str());
		}
	}
	else if (directive == "autoindex" && parts.size() > 1)
	{
		location.autoindex = (parts[1] == "on" || parts[1] == "yes" || parts[1] == "true");
	}
}

void ConfigParser::parseLocationBlock(ServerConfig& server, const std::string& content, size_t& pos)
{
	LocationConfig location;
	location.cgi_enabled = false;
	location.autoindex = false;

	// Eredita proprietà da server
	location.root = server.root;
	location.index = server.index;
	location.client_max_body_size = server.client_max_body_size;

	// Trova il path della location (es: "location /cgi-bin {")
	size_t location_start = content.find("location", pos);
	if (location_start == std::string::npos)
		return;

	size_t path_start = content.find_first_not_of(" \t", location_start + 8);
	size_t path_end = content.find_first_of(" \t{", path_start);

	if (path_start != std::string::npos && path_end != std::string::npos)
	{
		location.path = trim(content.substr(path_start, path_end - path_start));
	}

	// Trova l'inizio del blocco
	size_t block_start = content.find('{', pos);
	if (block_start == std::string::npos)
		return;

	// Trova la fine del blocco
	size_t block_end = content.find('}', block_start);
	if (block_end == std::string::npos)
		return;

	std::string block_content = content.substr(block_start + 1, block_end - block_start - 1);

	// Parsa le direttive dentro il blocco location
	std::stringstream ss(block_content);
	std::string line;

	while (std::getline(ss, line))
	{
		parseLocationDirective(location, line);
	}

	// Se non ci sono metodi specificati, usa tutti
	if (location.allowed_methods.empty())
	{
		location.allowed_methods.push_back("GET");
		location.allowed_methods.push_back("POST");
		location.allowed_methods.push_back("DELETE");
	}

	server.locations.push_back(location);
	pos = block_end + 1;
}

void ConfigParser::parseServerBlock(const std::string& content, size_t& pos)
{
	ServerConfig server;

	// Trova l'inizio del blocco server
	size_t block_start = content.find('{', pos);
	if (block_start == std::string::npos)
		return;

	// Trova la fine del blocco
	size_t block_end = content.find('}', block_start);
	if (block_end == std::string::npos)
		return;

	std::string block_content = content.substr(block_start + 1, block_end - block_start - 1);

	// Parsa le direttive principali e i location block
	std::stringstream ss(block_content);
	std::string line;

	while (std::getline(ss, line))
	{
		if (isLocationBlock(line))
		{
			// Parsa il location block
			size_t location_search_start = content.find(line, pos);
			if (location_search_start != std::string::npos)
			{
				parseLocationBlock(server, content, location_search_start);
			}
		}
		else
		{
			parseDirective(server, line);
		}
	}

	// Se non ci sono location blocks, crea uno di default per /
	if (server.locations.empty())
	{
		LocationConfig default_location;
		default_location.path = "/";
		default_location.root = server.root;
		default_location.index = server.index;
		default_location.allowed_methods.push_back("GET");
		default_location.allowed_methods.push_back("POST");
		default_location.allowed_methods.push_back("DELETE");
		default_location.cgi_enabled = false;
		default_location.autoindex = false;
		default_location.client_max_body_size = server.client_max_body_size;
		server.locations.push_back(default_location);
	}

	servers.push_back(server);
	pos = block_end + 1;
}

std::vector<ServerConfig> ConfigParser::parse()
{
	std::ifstream file(config_file.c_str());
	if (!file.is_open())
	{
		std::cerr << "Error: Cannot open config file: " << config_file << std::endl;
		std::cerr << "Using default configuration..." << std::endl;

		// Configurazone di default
		ServerConfig default_server;
		LocationConfig default_location;
		default_location.path = "/";
		default_location.root = "./www";
		default_location.index = "index.html";
		default_location.allowed_methods.push_back("GET");
		default_location.allowed_methods.push_back("POST");
		default_location.allowed_methods.push_back("DELETE");
		default_location.cgi_enabled = false;
		default_location.autoindex = false;
		default_location.client_max_body_size = default_server.client_max_body_size;

		default_server.locations.push_back(default_location);
		servers.push_back(default_server);

		return servers;
	}

	std::string content((std::istreambuf_iterator<char>(file)),
					   std::istreambuf_iterator<char>());
	file.close();

	// Rimuovi commenti
	std::string cleaned_content;
	for (size_t i = 0; i < content.length(); i++)
	{
		if (content[i] == '#')
		{
			// Salta fino a fine linea
			while (i < content.length() && content[i] != '\n')
				i++;
		}
		cleaned_content += content[i];
	}

	// Parsa i server block
	size_t pos = 0;
	while (pos < cleaned_content.length())
	{
		size_t server_pos = cleaned_content.find("server", pos);
		if (server_pos == std::string::npos)
			break;

		// Verifica che sia veramente un server block
		if (cleaned_content.find('{', server_pos) != std::string::npos)
		{
			parseServerBlock(cleaned_content, server_pos);
		}

		pos = server_pos + 1;
	}

	// Se non trovi nessun server block, usa configurazione di default
	if (servers.empty())
	{
		std::cerr << "Warning: No server blocks found in config file" << std::endl;
		std::cerr << "Using default configuration..." << std::endl;

		ServerConfig default_server;
		LocationConfig default_location;
		default_location.path = "/";
		default_location.root = "./www";
		default_location.index = "index.html";
		default_location.allowed_methods.push_back("GET");
		default_location.allowed_methods.push_back("POST");
		default_location.allowed_methods.push_back("DELETE");
		default_location.cgi_enabled = false;
		default_location.autoindex = false;
		default_location.client_max_body_size = default_server.client_max_body_size;

		default_server.locations.push_back(default_location);
		servers.push_back(default_server);
	}

	// Validate configuration
	for (size_t i = 0; i < servers.size(); i++)
	{
		ServerConfig& srv = servers[i];

		// Validate port
		if (srv.listen_port <= 0 || srv.listen_port > 65535)
		{
			std::cerr << "Warning: Invalid port " << srv.listen_port
					  << " in server " << (i + 1) << ", using 8080" << std::endl;
			srv.listen_port = 8080;
		}

		// Validate body size
		if (srv.client_max_body_size == 0)
		{
			srv.client_max_body_size = 1048576; // 1MB
		}

		// Ensure there's at least a default location
		if (srv.locations.empty())
		{
			LocationConfig default_location;
			default_location.path = "/";
			default_location.root = srv.root;
			default_location.index = srv.index;
			default_location.allowed_methods.push_back("GET");
			default_location.allowed_methods.push_back("POST");
			default_location.allowed_methods.push_back("DELETE");
			default_location.cgi_enabled = false;
			default_location.autoindex = false;
			default_location.client_max_body_size = srv.client_max_body_size;

			srv.locations.push_back(default_location);
		}
	}

	return servers;
}

void ConfigParser::displayServers() const
{
	for (size_t i = 0; i < servers.size(); i++)
	{
		std::cout << "\n=== Server " << (i + 1) << " ===" << std::endl;
		std::cout << "  Listen Port: " << servers[i].listen_port << std::endl;
		std::cout << "  Server Name: " << servers[i].server_name << std::endl;
		std::cout << "  Root: " << servers[i].root << std::endl;
		std::cout << "  Max Body Size: " << servers[i].client_max_body_size << " bytes" << std::endl;
		std::cout << "  Index: " << servers[i].index << std::endl;

		std::cout << "  Error Pages:" << std::endl;
		for (std::map<int, std::string>::const_iterator it = servers[i].error_pages.begin();
			 it != servers[i].error_pages.end(); ++it)
		{
			std::cout << "    " << it->first << " -> " << it->second << std::endl;
		}

		std::cout << "  Locations:" << std::endl;
		for (size_t j = 0; j < servers[i].locations.size(); j++)
		{
			std::cout << "    Path: " << servers[i].locations[j].path << std::endl;
			std::cout << "      Root: " << servers[i].locations[j].root << std::endl;
			std::cout << "      Index: " << servers[i].locations[j].index << std::endl;
			std::cout << "      CGI Enabled: " << (servers[i].locations[j].cgi_enabled ? "yes" : "no") << std::endl;
			std::cout << "      Autoindex: " << (servers[i].locations[j].autoindex ? "yes" : "no") << std::endl;
			std::cout << "      Max Body Size: " << servers[i].locations[j].client_max_body_size << " bytes" << std::endl;
			std::cout << "      Allowed Methods: ";
			for (size_t k = 0; k < servers[i].locations[j].allowed_methods.size(); k++)
			{
				std::cout << servers[i].locations[j].allowed_methods[k];
				if (k < servers[i].locations[j].allowed_methods.size() - 1)
					std::cout << ", ";
			}
			std::cout << std::endl;
		}
	}
}
