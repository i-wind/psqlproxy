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

### Setup database

For proxy to work with requests in plain text `ssl` option in postgresql.conf
must be switched off

    ssl = false

Create test role and database

    $ psql -U postgres -h localhost
    postgres=# CREATE USER test with PASSWORD 'test';
    postgres=# CREATE DATABASE testdb WITH ENCODING='UTF8' OWNER=test;

### Build sqlproxy

Commands to build sqlproxy

with scons

    $ scons -Q

with cmake:

    $ mkdir .build && cd .build
    $ cmake ..
    $ make

### Example of configuration

    logger {
        filename "sqlproxy.log"
    }
    proxy {
        proxy_ip "127.0.0.1"
        proxy_port 3333
        postgresql_ip "127.0.0.1"
        postgresql_port 5432
    }

### Execute command

    $ sqlproxy --config sqlproxy.conf

### Testing

    $ pgbench -h localhost -U test -d testdb -p 3333 -c 32 -j 4 -T 360
