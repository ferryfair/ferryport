/* 
 * File:   mycurl.h
 * Author: newmek7
 *
 * Created on 20 March, 2013, 2:53 PM
 */

#ifndef MYCURL_H
#define	MYCURL_H

#include<string>
using std::string;

string SOAPReq(string hostname, string port, string requestPath, string SOAPAction, string content, bool ssl);

#endif	/* MYCURL_H */

