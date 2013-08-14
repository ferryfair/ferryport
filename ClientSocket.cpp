// Implementation of the ClientSocket class

#include "ClientSocket.h"
#include "SocketException.h"
#include "debug.h"

ClientSocket::ClientSocket(std::string host, int port, Socket::SOCKET_TYPE socketType, std::string trustedCA, std::string privatecert, std::string privatekey) {
    soc = socketType == Socket::DEFAULT ? new Socket() : new Socket(socketType, trustedCA, privatecert, privatekey);
    if (!soc->create()) {
        throw SocketException("Could not create client socket.");
    }

    if (!soc->connect(host, port)) {
        throw SocketException("Could not bind to port.");
    }

}

const ClientSocket& ClientSocket::operator <<(const std::string& s) const {
    if (!soc->send(s)) {
        throw SocketException("Could not write to socket.");
    }

    return *this;

}

const ClientSocket& ClientSocket::operator >>(std::string& s) const {
    if (!soc->recv(s)) {
        throw SocketException("Could not read from socket.");
    }

    return *this;
}

