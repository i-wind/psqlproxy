## SQLProxy server

SQLProxy server is designed to proxy tcp-requests for PostgreSQL database
server.

PostgreSQL uses a message-based protocol for communication between frontends
and backends (clients and servers). The protocol is supported over TCP/IP and
also over Unix-domain sockets. Port number 5432 has been registered with IANA
as the customary TCP port number for servers supporting this protocol, but in
practice any non-privileged port number can be used.

### Protocol version 3.0 documentation:

* [PostgreSQL Frontend/Backend Protocol](https://www.postgresql.org/docs/9.4/static/protocol.html)
