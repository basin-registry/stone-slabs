# HTTP Server

LLVM/native builds only. Provides HTTP server functionality for Stone applications.

```stone
import server
```

`server.serve(port)` -- Start listening, returns server handle

`server.await_request(s)` -- Block until next request, returns request record

`server.send_response(req, {status, body})` -- Send response to a request

`server.stop_server(s)` -- Stop the server

```stone
import server

s = server.serve(3000)
print("Listening on port 3000")

req = server.await_request(s)
server.send_response(req, {status = 200, body = "Hello!"})
```

## API Reference

| Function | Description |
|----------|-------------|
| `serve(port)` | Start HTTP server. Returns handle |
| `await_request(handle)` | Block until next request. Returns { method, path, body, ... } |
| `send_response(req, config)` | Send response. Config: { status, body, content_type? } |
| `stop_server(handle)` | Stop the server |
