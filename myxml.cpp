#include<stdio.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include<libxml/parser.h>
#include <libxml/xpathInternals.h>
#include<assert.h>
#include "myxml.h"
#include<string>
#include<iostream>
using namespace std;

void print_node(xmlNode *n) {
    std::cout << "named '" << n->name << "'";
    const xmlChar *content = xmlNodeGetContent(n);
    if (content)
        std::cout << " with content '" << content << "'";
    std::cout << "\n";
}

void print_xpath_nodes(xmlNodeSetPtr nodes, FILE* output) {

    xmlNodePtr cur;
    int size;
    int i;

    assert(output);
    size = (nodes) ? nodes->nodeNr : 0;

    fprintf(output, "Result (%d nodes):\n", size);
    for (i = 0; i < size; ++i) {
        assert(nodes->nodeTab[i]);

        if (nodes->nodeTab[i]->type == XML_NAMESPACE_DECL) {
            xmlNsPtr ns;

            ns = (xmlNsPtr) nodes->nodeTab[i];
            cur = (xmlNodePtr) ns->next;
            if (cur->ns) {
                fprintf(output, "= namespace \"%s\"=\"%s\" for node %s:%s\n",
                        ns->prefix, ns->href, cur->ns->href, cur->name);
            } else {
                fprintf(output, "= namespace \"%s\"=\"%s\" for node %s\n",
                        ns->prefix, ns->href, cur->name);
            }
        } else if (nodes->nodeTab[i]->type == XML_ELEMENT_NODE) {
            cur = nodes->nodeTab[i];
            if (cur->ns) {
                fprintf(output, "= element node \"%s:%s\"\n",
                        cur->ns->href, cur->name);
            } else {
                fprintf(output, "= element node \"%s\"\n",
                        cur->name);
            }
        } else {
            cur = nodes->nodeTab[i];
            fprintf(output, "= node \"%s\": type %d\n", cur->name, cur->type);
        }
    }
}



