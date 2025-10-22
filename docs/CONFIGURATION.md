# Configuration Guide

## Configuration File Structure

The configuration file uses a simple block-based syntax similar to NGINX.

### Server Block

```
server {
    # Server directives
    location {
        # Location directives
    }
}
```

## Server Directives

### listen
Sets the port for the server to listen on.
```
listen 8080;
```

### host
Sets the host address to bind to.
```
host 0.0.0.0;
```

### server_name
Sets the server name(s).
```
server_name localhost example.com;
```

### error_page
Defines custom error pages for specific HTTP status codes.
```
error_page 404 /errors/404.html;
error_page 500 /errors/500.html;
```

### client_max_body_size
Sets the maximum allowed size of the client request body (in bytes).
```
client_max_body_size 10485760;  # 10MB
```

## Location Directives

### root
Sets the root directory for requests.
```
root www;
```

### index
Defines the default file to serve.
```
index index.html;
```

### allow_methods
Specifies which HTTP methods are allowed.
```
allow_methods GET POST DELETE;
```

### autoindex
Enables or disables directory listing.
```
autoindex on;
```

### upload_path
Sets the directory where uploaded files will be saved.
```
upload_path www/uploads;
```

### cgi_extension
Specifies the file extension for CGI scripts.
```
cgi_extension .py;
```

### cgi_path
Sets the path to the CGI interpreter.
```
cgi_path /usr/bin/python3;
```

### return
Creates a redirect.
```
return http://example.com;
```

## Example Configuration

```
server {
    listen 8080;
    host 0.0.0.0;
    server_name localhost;

    error_page 404 www/errors/404.html;
    error_page 500 www/errors/500.html;

    client_max_body_size 10485760;

    # Static files
    location / {
        root www;
        index index.html;
        allow_methods GET POST DELETE;
        autoindex off;
    }

    # File uploads
    location /uploads {
        root www;
        allow_methods GET POST DELETE;
        autoindex on;
        upload_path www/uploads;
    }

    # CGI scripts
    location /cgi-bin {
        root www;
        allow_methods GET POST;
        cgi_extension .py;
        cgi_path /usr/bin/python3;
    }

    # Redirect
    location /redirect {
        return http://www.google.com;
    }
}

# Multiple servers
server {
    listen 8081;
    host 0.0.0.0;
    server_name alternative;

    location / {
        root www2;
        index index.html;
        allow_methods GET;
    }
}
```

## Notes

- Comments start with `#`
- Each directive must end with `;`
- Location paths are matched using longest prefix matching
- If no location matches, the request will return 404
- Multiple servers can listen on different ports
