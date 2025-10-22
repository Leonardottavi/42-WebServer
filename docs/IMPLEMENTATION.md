# Implementation Details

## Architecture

The WebServ project follows an event-driven architecture using `poll()` for non-blocking I/O.

### Main Components

1. **WebServer**: Main event loop manager
2. **Server**: Listening socket wrapper
3. **Client**: Client connection handler
4. **Config**: Configuration file parser
5. **HttpRequest**: HTTP request parser
6. **HttpResponse**: HTTP response builder
7. **CGI**: CGI script executor

## Event Loop

The server uses a single `poll()` call to monitor all file descriptors:

```cpp
while (running) {
    poll(&pollFds[0], pollFds.size(), timeout);

    // Process events
    for each fd with events:
        if server socket:
            accept new connection
        else if client socket:
            if POLLIN: read request
            if POLLOUT: write response
}
```

## Request Processing Flow

1. Client connects → accept() → add to poll
2. POLLIN event → read data → parse HTTP request
3. Request complete → process request → build response
4. Switch to POLLOUT → write response
5. Response complete → close connection

## Non-blocking I/O

All socket operations are non-blocking:
- `accept()` returns immediately if no connection
- `recv()` returns immediately if no data
- `send()` returns immediately if buffer full

The `poll()` call ensures we only read/write when the socket is ready.

## HTTP Request Parsing

Requests are parsed incrementally as data arrives:

1. Read request line (method, URI, version)
2. Read headers line by line
3. Determine content length from headers
4. Read body until content length reached

## HTTP Response Building

Responses are built with proper headers:

```
HTTP/1.1 200 OK\r\n
Content-Type: text/html\r\n
Content-Length: 123\r\n
\r\n
<body>
```

## CGI Execution

CGI scripts are executed using fork/exec:

1. Create pipes for stdin/stdout
2. Fork child process
3. Setup environment variables
4. Execute CGI script
5. Read output from pipe
6. Wait for child to complete

## File Upload

File uploads using multipart/form-data:

1. Parse Content-Type for boundary
2. Find filename in headers
3. Extract file data between boundaries
4. Save to upload directory

## Error Handling

- Custom error pages can be configured per server
- Default error pages are generated if none configured
- HTTP status codes are accurate per RFC

## Memory Management

- All dynamically allocated objects are properly freed
- Client objects deleted when connection closes
- No memory leaks (verified with valgrind)

## C++98 Compliance

The code strictly follows C++98 standard:
- No C++11 features (auto, nullptr, etc.)
- Uses std::string, std::vector, std::map
- Proper RAII for resource management

## Security Considerations

- Request size limits (client_max_body_size)
- Connection timeouts (60 seconds)
- Path traversal prevention
- File access validation

## Performance

- Single thread with event-driven I/O
- Can handle hundreds of concurrent connections
- Efficient with poll() syscall overhead
- No busy-waiting
