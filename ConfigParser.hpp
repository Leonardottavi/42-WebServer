#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <string>
#include <vector>
#include <map>
#include <iostream>

struct LocationConfig
{
	std::string path;
	std::vector<std::string> allowed_methods;
	std::string root;
	std::string index;
	bool cgi_enabled;
	std::string cgi_extension;
};

struct ServerConfig
{
	int listen_port;
	std::string server_name;
	std::string root;
	std::string index;
	size_t client_max_body_size;
	std::vector<LocationConfig> locations;
	std::map<int, std::string> error_pages;
	bool cgi_enabled;

	ServerConfig() : listen_port(8080),
					 root("./www"),
					 index("index.html"),
					 client_max_body_size(10485760),
					 cgi_enabled(false) {}
};

class ConfigParser
{
private:
	std::string config_file;
	std::vector<ServerConfig> servers;

	// Helper methods
	std::string trim(const std::string& str);
	std::string toLower(const std::string& str);
	std::vector<std::string> split(const std::string& str, char delimiter);
	bool isServerBlock(const std::string& line);
	bool isLocationBlock(const std::string& line);

	// Parsing methods
	void parseServerBlock(const std::string& content, size_t& pos);
	void parseLocationBlock(ServerConfig& server, const std::string& content, size_t& pos);
	void parseDirective(ServerConfig& server, const std::string& line);
	void parseLocationDirective(LocationConfig& location, const std::string& line);

public:
	ConfigParser(const std::string& file);
	~ConfigParser();

	std::vector<ServerConfig> parse();
	void displayServers() const;
};

#endif
