// Definition of the Socket class

#ifndef Socket_class
#define Socket_class

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>
#include <openssl/ossl_typ.h>


const int MAXHOSTNAME = 200;
const int MAXCONNECTIONS = 5;
const int MAXRECV = 500;

class Socket {
public:

    enum SOCKET_TYPE {
        DEFAULT, TLS1_1
    };

    bool is_valid() const {
        return m_sock != -1;
    }

    Socket();
    Socket(SOCKET_TYPE socketType, std::string trustedCA, std::string privatecert, std::string privatekey);
    virtual ~Socket();
    bool create();
    bool bind(const int port);
    bool listen() const;
    bool accept(Socket&) const;
    bool connect(const std::string host, const int port);
    bool send(const std::string) const;
    int recv(std::string&) const;
    void set_non_blocking(const bool);

private:
    int m_sock;
    sockaddr_in m_addr;
    SSL_CTX *sslctx;
    SSL *cSSL;
    std::string trustedCA;
    std::string privatecert;
    std::string privatekey;
    SOCKET_TYPE socketType;
};

#endif