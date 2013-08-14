// Definition of the ClientSocket class

#ifndef ClientSocket_class
#define ClientSocket_class

#include "Socket.h"

class ClientSocket : private Socket {
public:

    ClientSocket(std::string host, int port, Socket::SOCKET_TYPE socketType = Socket::DEFAULT, std::string trustedCA = "", std::string privatecert = "", std::string privatekey = "");

    virtual ~ClientSocket() {
        delete soc;
    };

    const ClientSocket& operator <<(const std::string&) const;
    const ClientSocket& operator >>(std::string&) const;

private:
    Socket* soc;
};


#endif