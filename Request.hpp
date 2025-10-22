#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <cstdlib>

class Request
{
    private:
        std::string method;
        std::string uri;
        std::string version;
        std::map<std::string, std::string> headers;
        std::string body;
        size_t content_length;
        bool has_content_length;
        static const size_t MAX_BODY_SIZE = 10485760;
        //10 mb
    public:
        Request();
        void parse(std::string request);
        std::string getMethod()const;
        std::string getUri()const;
        std::string getVersion()const;
        std::string getBody()const;
        void displayHeaders()const;
        std::string getHeader(std::string param)const;
        size_t getContentLength() const;
        bool hasContentLength()const;
        void validateMethod();
        bool isMethodValid(const std::string& method)const;
    private:
        void validateContentLength();
        std::string toLower(const std::string& str)const;
        void validateVersion();
        void validateUri();
        void validateHost();
};

#endif