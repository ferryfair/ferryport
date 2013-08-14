
#include "myconverters.h"
#include "ClientSocket.h"
#include "SocketException.h"
#include "mycurl.h"
#include "Socket.h"
#include<string>
#include<string.h>
#include<stdio.h>
#include<sstream>
#include<iostream>
#include<stdlib.h>
using namespace std;

/*static int wait_on_socket(curl_socket_t sockfd, int for_recv, long timeout_ms) {
    struct timeval tv;
    fd_set infd, outfd, errfd;
    int res;

    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    FD_ZERO(&infd);
    FD_ZERO(&outfd);
    FD_ZERO(&errfd);

    FD_SET(sockfd, &errfd); //* always check for error 

    if (for_recv) {
        FD_SET(sockfd, &infd);
    } else {
        FD_SET(sockfd, &outfd);
    }

    //* select() returns the number of signalled sockets or -1
    res = select(sockfd + 1, &infd, &outfd, &errfd, &tv);
    return res;
}*/

string SOAPReq(string hostname, string port, string requestPath, string SOAPAction, string content, Socket::SOCKET_TYPE socketType, std::string trustedCA, std::string privatecert, std::string privatekey) {
    int cl = content.length();
    ostringstream ss;
    ss << cl;
    string ccl = ss.str();
    string req1 = "POST " + requestPath + " HTTP/1.1\r\nUser-Agent: Mozilla/4.0 (compatible; MSIE 6.0; MS Web Services Client Protocol 2.0.50727.3625)\r\nContent-Type: text/xml; charset=utf-8\r\nSOAPAction: \"" + SOAPAction + "\"\r\nHost: " + hostname + "\r\nContent-Length: " + ccl + "\r\nExpect: 100-continue\r\nConnection: Keep-Alive\r\n\r\n";
    string response;
    int p = atoi(port.c_str());

    try {
        ClientSocket client_socket(hostname, p);
        try {
            client_socket << req1;
            client_socket >> response;
        } catch (SocketException&) {
            return "CONNECTION ERROR";
        }
        if (response.find("100 Continue", 0) > 0) {
            client_socket << content;
            string resbuf;
            response = "";
            int l = 0;
            bool s = false;
            do {
                try {
                    client_socket >> resbuf;
                    response += resbuf;
                    if (!s) {
                        int si = response.find("Content-Length: ", 0) + 16;
                        l = response.find("\r\n", si) - si;
                        string len = response.substr(si, l);
                        l = atoi(len.c_str())+(int) response.find("\r\n\r\n") + 4;
                        s = true;
                    }
                } catch (SocketException&) {
                    return "CONNECTION ERROR";
                }
                l -= resbuf.length();
            } while (l > 0);
        } else {
            return "CONNECTION ERROR";
        }
    } catch (SocketException& e) {
        return "CONNECTION ERROR";
    }



    int si = response.find("Content-Length: ", 0) + 16;
    int l = response.find("\r\n", si) - si;
    string len = response.substr(si, l);
    l = atoi(len.c_str());
    si = response.find("\r\n\r\n", 0) + 4;
    string res2 = response.substr(si, l);
    return res2;
}

string HTTPReq(string hostname, string requestPath, string port, string content, Socket::SOCKET_TYPE socketType, std::string trustedCA, std::string privatecert, std::string privatekey) {
    int cl = content.length();
    ostringstream ss;
    ss << cl;
    string ccl = ss.str();
    string req1 = "POST " + requestPath + " HTTP/1.1\r\nUser-Agent: Mozilla/4.0 (compatible; MSIE 6.0; MS Web Services Client Protocol 2.0.50727.3625)\r\nContent-Type: text/xml; charset=utf-8\r\nHost: " + hostname + "\r\nContent-Length: " + ccl + "\r\nConnection: Keep-Alive\r\n\r\n";
    string response;
    int p = atoi(port.c_str());

    try {
        ClientSocket client_socket(hostname, p, socketType);
        try {
            client_socket << req1;
        } catch (SocketException&) {
            return "CONNECTION ERROR";
        }
        string resbuf;
        response = "";
        int l = 0;
        bool s = false;
        do {
            try {
                client_socket >> resbuf;
                response += resbuf;
                if (!s) {
                    int si = response.find("Content-Length: ", 0) + 16;
                    l = response.find("\r\n", si) - si;
                    string len = response.substr(si, l);
                    l = atoi(len.c_str())+(int) response.find("\r\n\r\n") + 4;
                    s = true;
                }
            } catch (SocketException&) {
                return "CONNECTION ERROR";
            }
            l -= resbuf.length();
        } while (l > 0);
    } catch (SocketException& e) {
        return "CONNECTION ERROR";
    }
    int si = response.find("Content-Length: ", 0) + 16;
    int l = response.find("\r\n", si) - si;
    string len = response.substr(si, l);
    l = atoi(len.c_str());
    si = response.find("\r\n\r\n", 0) + 4;
    string res2 = response.substr(si, l);
    return res2;
}

/*string httpReq(string hostname, string port, string method, string requestPath, string contentType, string content, bool ssl) {
    CURL *curl;
    CURLcode res;
    int cl = content.length();
    ostringstream ss;
    ss << cl;
    string ccl = ss.str();
    string req1 = method + " " + requestPath + " HTTP/1.0\r\nHost: " + hostname + "\r\nContent-Type:" + contentType + "\r\n" + ((cl > 0) ? "Content-Length:" + ccl + "\r\n" : "") + "\r\n" + content;
    char* request = (char*) req1.c_str();
    string response;
    curl_socket_t sockfd; //* socket 
    long sockextr;
    size_t iolen;
    curl_off_t nread;

    curl = curl_easy_init();
    if (curl) {
        string url = "http://" + hostname + ":" + port;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        //* Do not do the transfer - only connect to host 
        curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 1L);
        res = curl_easy_perform(curl);

        if (CURLE_OK != res) {
            //printf("Error: %s\n", strerror(res));
            return "";
        }

        //* Extract the socket from the curl handle - we'll need it for waiting.
         //* Note that this API takes a pointer to a 'long' while we use
         // curl_socket_t for sockets otherwise.
	 
        res = curl_easy_getinfo(curl, CURLINFO_LASTSOCKET, &sockextr);

        if (CURLE_OK != res) {
            fprintf(stderr, "Error: %s\n", curl_easy_strerror(res));
            return "";
        }

        sockfd = sockextr;

        //* wait for the socket to become ready for sending
        if (!wait_on_socket(sockfd, 0, 60000L)) {
            //printf("Error: timeout.\n");
            return NULL;
        }

        //puts("Sending request.");
        //* Send the request. Real applications should check the iolen
         //* to see if all the request has been sent 
        res = curl_easy_send(curl, request, strlen(request), &iolen);

        if (CURLE_OK != res) {
            //printf("Error: %s\n", curl_easy_strerror(res));
            return NULL;
        }
        //puts("Reading response.");

        //* read the response 
        for (;;) {
            string buf2;
            char buf[2048];

            wait_on_socket(sockfd, 1, 60000L);
            res = curl_easy_recv(curl, buf, 2048, &iolen);

            if (CURLE_OK != res) {
                break;
            }
            nread = (curl_off_t) iolen;

            //printf("Received %" CURL_FORMAT_CURL_OFF_T " bytes.\n", nread);
            buf2 = string(buf);
            response += buf2;
        }

        //* always cleanup 
        curl_easy_cleanup(curl);
    }
    int si = response.find("Content-Length: ", 0) + 16;
    int l = response.find("\r\n", si) - si;
    string len = response.substr(si, l);
    l = atoi(len.c_str());
    si = response.find("\r\n\r\n", 0) + 4;
    string res2 = response.substr(si, l);
    return res2;
}*/
