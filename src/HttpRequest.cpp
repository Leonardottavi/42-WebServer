#include "HttpRequest.hpp"
#include "Utils.hpp"
#include <sstream>

HttpRequest::HttpRequest() : complete(false), headersParsed(false), contentLength(0) {}

HttpRequest::~HttpRequest() {}

void HttpRequest::parseRequestLine(const std::string& line) {
	std::istringstream iss(line);
	iss >> method >> uri >> httpVersion;
}

void HttpRequest::parseHeader(const std::string& line) {
	size_t pos = line.find(':');
	if (pos != std::string::npos) {
		std::string key = Utils::trim(line.substr(0, pos));
		std::string value = Utils::trim(line.substr(pos + 1));
		headers[key] = value;
	}
}

bool HttpRequest::parse(const std::string& data) {
	rawRequest += data;

	if (!headersParsed) {
		size_t headerEnd = rawRequest.find("\r\n\r\n");
		if (headerEnd == std::string::npos)
			return false;

		std::string headerSection = rawRequest.substr(0, headerEnd);
		std::istringstream stream(headerSection);
		std::string line;

		if (std::getline(stream, line)) {
			if (!line.empty() && line[line.length() - 1] == '\r')
				line = line.substr(0, line.length() - 1);
			parseRequestLine(line);
		}

		while (std::getline(stream, line)) {
			if (!line.empty() && line[line.length() - 1] == '\r')
				line = line.substr(0, line.length() - 1);
			if (!line.empty())
				parseHeader(line);
		}

		headersParsed = true;

		std::string contentLenStr = getHeader("Content-Length");
		if (!contentLenStr.empty())
			contentLength = Utils::stringToSize(contentLenStr);

		body = rawRequest.substr(headerEnd + 4);
	} else {
		body = rawRequest.substr(rawRequest.find("\r\n\r\n") + 4);
	}

	if (method == "GET" || method == "DELETE") {
		complete = true;
		return true;
	}

	if (body.length() >= contentLength) {
		complete = true;
		body = body.substr(0, contentLength);
		return true;
	}

	return false;
}

void HttpRequest::clear() {
	method.clear();
	uri.clear();
	httpVersion.clear();
	headers.clear();
	body.clear();
	rawRequest.clear();
	complete = false;
	headersParsed = false;
	contentLength = 0;
}

const std::string& HttpRequest::getMethod() const {
	return method;
}

const std::string& HttpRequest::getUri() const {
	return uri;
}

const std::string& HttpRequest::getHttpVersion() const {
	return httpVersion;
}

const std::map<std::string, std::string>& HttpRequest::getHeaders() const {
	return headers;
}

const std::string& HttpRequest::getBody() const {
	return body;
}

std::string HttpRequest::getHeader(const std::string& name) const {
	std::map<std::string, std::string>::const_iterator it = headers.find(name);
	if (it != headers.end())
		return it->second;
	return "";
}

bool HttpRequest::isComplete() const {
	return complete;
}
