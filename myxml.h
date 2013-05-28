/* 
 * File:   myxml.h
 * Author: newmek7
 *
 * Created on 19 March, 2013, 10:52 AM
 */

#ifndef MYXML_H
#define	MYXML_H

#ifdef	__cplusplus
extern "C" {
#endif
#include <libxml/xpath.h>
#include <libxml/parser.h>

    void print_node(xmlNode *n);
    void print_xpath_nodes(xmlNodeSetPtr nodes, FILE* output);

#ifdef	__cplusplus
}
#endif

#endif	/* MYXML_H */

