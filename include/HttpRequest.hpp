#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <map>

class HttpRequest {
private:
	std::string method;
	std::string uri;
	std::string httpVersion;
	std::map<std::string, std::string> headers;
	std::string body;
	bool complete;
	bool headersParsed;
	size_t contentLength;
	std::string rawRequest;

	void parseRequestLine(const std::string& line);
	void parseHeader(const std::string& line);

public:
	HttpRequest();
	~HttpRequest();

	bool parse(const std::string& data);
	void clear();

	const std::string& getMethod() const;
	const std::string& getUri() const;
	const std::string& getHttpVersion() const;
	const std::map<std::string, std::string>& getHeaders() const;
	const std::string& getBody() const;
	std::string getHeader(const std::string& name) const;
	bool isComplete() const;
};

#endif
