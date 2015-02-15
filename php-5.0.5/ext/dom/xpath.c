/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2004 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_0.txt.                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Christian Stocker <chregu@php.net>                          |
   |          Rob Richards <rrichards@php.net>                            |
   +----------------------------------------------------------------------+
*/

/* $Id: xpath.c,v 1.22.2.1 2004/11/18 19:55:00 rrichards Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#if HAVE_LIBXML && HAVE_DOM
#include "php_dom.h"


/*
* class DOMXPath 
*/

#if defined(LIBXML_XPATH_ENABLED)

zend_function_entry php_dom_xpath_class_functions[] = {
	PHP_ME(domxpath, __construct, NULL, ZEND_ACC_PUBLIC)
	PHP_FALIAS(registerNamespace, dom_xpath_register_ns, NULL)
	PHP_FALIAS(query, dom_xpath_query, NULL)
	{NULL, NULL, NULL}
};

/* {{{ proto void DOMXPath::__construct(DOMDocument doc); */
PHP_METHOD(domxpath, __construct)
{
	zval *id, *doc;
	xmlDocPtr docp = NULL;
	dom_object *docobj, *intern;
	xmlXPathContextPtr ctx, oldctx;

	php_set_error_handling(EH_THROW, dom_domexception_class_entry TSRMLS_CC);
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "OO", &id, dom_xpath_class_entry, &doc, dom_document_class_entry) == FAILURE) {
		php_std_error_handling();
		return;
	}

	php_std_error_handling();
	DOM_GET_OBJ(docp, doc, xmlDocPtr, docobj);

	ctx = xmlXPathNewContext(docp);
	if (ctx == NULL) {
		php_dom_throw_error(INVALID_STATE_ERR, 1 TSRMLS_CC);
		RETURN_FALSE;
	}

	intern = (dom_object *)zend_object_store_get_object(id TSRMLS_CC);
	if (intern != NULL) {
		oldctx = (xmlXPathContextPtr)intern->ptr;
		if (oldctx != NULL) {
			php_libxml_decrement_doc_ref((php_libxml_node_object *)intern TSRMLS_CC);
			xmlXPathFreeContext(oldctx);
		}
		intern->ptr = ctx;
		intern->document = docobj->document;
		php_libxml_increment_doc_ref((php_libxml_node_object *)intern, docp TSRMLS_CC);
	}
}
/* }}} end DOMXPath::__construct */

/* {{{ document DOMDocument*/
int dom_xpath_document_read(dom_object *obj, zval **retval TSRMLS_DC)
{
	xmlDoc *docp = NULL;
	xmlXPathContextPtr ctx;
	int ret;

	ctx = (xmlXPathContextPtr) obj->ptr;

	if (ctx) {
		docp = (xmlDocPtr) ctx->doc;
	}

	ALLOC_ZVAL(*retval);
	if (NULL == (*retval = php_dom_create_object((xmlNodePtr) docp, &ret, NULL, *retval, obj TSRMLS_CC))) {
		php_error(E_WARNING, "Cannot create required DOM object");
		return FAILURE;
	}
	return SUCCESS;
}

/* {{{ proto boolean dom_xpath_register_ns(string prefix, string uri); */
PHP_FUNCTION(dom_xpath_register_ns)
{
	zval *id;
	xmlXPathContextPtr ctxp;
	int prefix_len, ns_uri_len;
	dom_object *intern;
	unsigned char *prefix, *ns_uri;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss", &id, dom_xpath_class_entry, &prefix, &prefix_len, &ns_uri, &ns_uri_len) == FAILURE) {
		return;
	}

	intern = (dom_object *)zend_object_store_get_object(id TSRMLS_CC);

	ctxp = (xmlXPathContextPtr) intern->ptr;
	if (ctxp == NULL) {
		php_error(E_WARNING, "Invalid XPath Context");
		RETURN_FALSE;
	}

	if (xmlXPathRegisterNs(ctxp, prefix, ns_uri) != 0) {
		RETURN_FALSE
	}
	RETURN_TRUE;
}

static void dom_xpath_iter(zval *baseobj, dom_object *intern)
{
	dom_nnodemap_object *mapptr;

	mapptr = (dom_nnodemap_object *)intern->ptr;
	mapptr->baseobjptr = baseobj;
	mapptr->nodetype = DOM_NODESET;

}

/* {{{ proto DOMNodeList dom_xpath_query(string expr [,DOMNode context]); */
PHP_FUNCTION(dom_xpath_query)
{
	zval *id, *retval, *context = NULL;
	xmlXPathContextPtr ctxp;
	xmlNodePtr nodep = NULL;
	xmlXPathObjectPtr xpathobjp;
	int expr_len, ret, nsnbr = 0;
	dom_object *intern, *nodeobj;
	char *expr;
	xmlDoc *docp = NULL;
	xmlNsPtr *ns;


	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os|O", &id, dom_xpath_class_entry, &expr, &expr_len, &context, dom_node_class_entry) == FAILURE) {
		return;
	}

	intern = (dom_object *)zend_object_store_get_object(id TSRMLS_CC);

	ctxp = (xmlXPathContextPtr) intern->ptr;
	if (ctxp == NULL) {
		php_error(E_WARNING, "Invalid XPath Context");
		RETURN_FALSE;
	}

	docp = (xmlDocPtr) ctxp->doc;
	if (docp == NULL) {
		php_error(E_WARNING, "Invalid XPath Document Pointer");
		RETURN_FALSE;
	}

	if (context != NULL) {
		DOM_GET_OBJ(nodep, context, xmlNodePtr, nodeobj);
	}

	if (!nodep) {
		nodep = xmlDocGetRootElement(docp);
	}

	if (nodep && docp != nodep->doc) {
		php_error(E_WARNING, "Node From Wrong Document");
		RETURN_FALSE;
	}

	ctxp->node = nodep;

	/* Register namespaces in the node */
	ns = xmlGetNsList(docp, nodep);

    if (ns != NULL) {
        while (ns[nsnbr] != NULL)
	    nsnbr++;
    }


    ctxp->namespaces = ns;
    ctxp->nsNr = nsnbr;

	xpathobjp = xmlXPathEvalExpression(expr, ctxp);
	ctxp->node = NULL;

	if (ns != NULL) {
		xmlFree(ns);
		ctxp->namespaces = NULL;
		ctxp->nsNr = 0;
	}

	if (! xpathobjp) {
		RETURN_FALSE;
	}


	MAKE_STD_ZVAL(retval);
	array_init(retval);

	if (xpathobjp->type ==  XPATH_NODESET) {
		int i;
		xmlNodeSetPtr nodesetp;

		if (NULL != (nodesetp = xpathobjp->nodesetval)) {

			for (i = 0; i < nodesetp->nodeNr; i++) {
				xmlNodePtr node = nodesetp->nodeTab[i];
				zval *child;

				MAKE_STD_ZVAL(child);
				
				if (node->type == XML_NAMESPACE_DECL) {
					xmlNsPtr curns;
					xmlNodePtr nsparent;

					nsparent = node->_private;
					curns = xmlNewNs(NULL, node->name, NULL);
					if (node->children) {
						curns->prefix = xmlStrdup((char *) node->children);
					}
					if (node->children) {
						node = xmlNewDocNode(docp, NULL, (char *) node->children, node->name);
					} else {
						node = xmlNewDocNode(docp, NULL, "xmlns", node->name);
					}
					node->type = XML_NAMESPACE_DECL;
					node->parent = nsparent;
					node->ns = curns;
				}
				child = php_dom_create_object(node, &ret, NULL, child, intern TSRMLS_CC);
				add_next_index_zval(retval, child);
			}
		}
	}

	php_dom_create_interator(return_value, DOM_NODELIST TSRMLS_CC);
	intern = (dom_object *)zend_objects_get_address(return_value TSRMLS_CC);
	dom_xpath_iter(retval, intern);

	xmlXPathFreeObject(xpathobjp);
}
/* }}} end dom_xpath_query */

#endif /* LIBXML_XPATH_ENABLED */

/* }}} */
#endif
