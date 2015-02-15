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

/* $Id: element.c,v 1.30.2.4 2005/05/20 15:02:48 rrichards Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#if HAVE_LIBXML && HAVE_DOM
#include "php_dom.h"


/*
* class DOMElement extends DOMNode 
*
* URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#core-ID-745549614
* Since: 
*/

zend_function_entry php_dom_element_class_functions[] = {
	PHP_FALIAS(getAttribute, dom_element_get_attribute, NULL)
	PHP_FALIAS(setAttribute, dom_element_set_attribute, NULL)
	PHP_FALIAS(removeAttribute, dom_element_remove_attribute, NULL)
	PHP_FALIAS(getAttributeNode, dom_element_get_attribute_node, NULL)
	PHP_FALIAS(setAttributeNode, dom_element_set_attribute_node, NULL)
	PHP_FALIAS(removeAttributeNode, dom_element_remove_attribute_node, NULL)
	PHP_FALIAS(getElementsByTagName, dom_element_get_elements_by_tag_name, NULL)
	PHP_FALIAS(getAttributeNS, dom_element_get_attribute_ns, NULL)
	PHP_FALIAS(setAttributeNS, dom_element_set_attribute_ns, NULL)
	PHP_FALIAS(removeAttributeNS, dom_element_remove_attribute_ns, NULL)
	PHP_FALIAS(getAttributeNodeNS, dom_element_get_attribute_node_ns, NULL)
	PHP_FALIAS(setAttributeNodeNS, dom_element_set_attribute_node_ns, NULL)
	PHP_FALIAS(getElementsByTagNameNS, dom_element_get_elements_by_tag_name_ns, NULL)
	PHP_FALIAS(hasAttribute, dom_element_has_attribute, NULL)
	PHP_FALIAS(hasAttributeNS, dom_element_has_attribute_ns, NULL)
	PHP_FALIAS(setIdAttribute, dom_element_set_id_attribute, NULL)
	PHP_FALIAS(setIdAttributeNS, dom_element_set_id_attribute_ns, NULL)
	PHP_FALIAS(setIdAttributeNode, dom_element_set_id_attribute_node, NULL)
	PHP_ME(domelement, __construct, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

/* {{{ proto void DOMElement::__construct(string name, [string value], [string uri]); */
PHP_METHOD(domelement, __construct)
{

	zval *id;
	xmlNodePtr nodep = NULL, oldnode = NULL;
	dom_object *intern;
	char *name, *value = NULL, *uri = NULL;
	char *localname = NULL, *prefix = NULL;
	int errorcode = 0, uri_len = 0;
	int name_len, value_len = 0, name_valid;
	xmlNsPtr nsptr = NULL;

	php_set_error_handling(EH_THROW, dom_domexception_class_entry TSRMLS_CC);
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os|s!s", &id, dom_element_class_entry, &name, &name_len, &value, &value_len, &uri, &uri_len) == FAILURE) {
		php_std_error_handling();
		return;
	}

	php_std_error_handling();
	name_valid = xmlValidateName((xmlChar *) name, 0);
	if (name_valid != 0) {
		php_dom_throw_error(INVALID_CHARACTER_ERR, 1 TSRMLS_CC);
		RETURN_FALSE;
	}

	/* Namespace logic is seperate and only when uri passed in to insure no BC breakage */
	if (uri_len > 0) {
		errorcode = dom_check_qname(name, &localname, &prefix, uri_len, name_len);
		if (errorcode == 0) {
			nodep = xmlNewNode (NULL, localname);
			if (nodep != NULL && uri != NULL) {
				nsptr = dom_get_ns(nodep, uri, &errorcode, prefix);
				xmlSetNs(nodep, nsptr);
			}
		}
		xmlFree(localname);
		if (prefix != NULL) {
			xmlFree(prefix);
		}
		if (errorcode != 0) {
			if (nodep != NULL) {
				xmlFree(nodep);
			}
			php_dom_throw_error(errorcode, 1 TSRMLS_CC);
			RETURN_FALSE;
		}
	} else {
	    /* If you don't pass a namespace uri, then you can't set a prefix */
	    localname = xmlSplitQName2(name, (xmlChar **) &prefix);
	    if (prefix != NULL) {
			xmlFree(localname);
			xmlFree(prefix);
	        php_dom_throw_error(NAMESPACE_ERR, 1 TSRMLS_CC);
	        RETURN_FALSE;
	    }
		nodep = xmlNewNode(NULL, (xmlChar *) name);
	}

	if (!nodep) {
		php_dom_throw_error(INVALID_STATE_ERR, 1 TSRMLS_CC);
		RETURN_FALSE;
	}

	if (value_len > 0) {
		xmlNodeSetContentLen(nodep, value, value_len);
	}

	intern = (dom_object *)zend_object_store_get_object(id TSRMLS_CC);
	if (intern != NULL) {
		oldnode = (xmlNodePtr)intern->ptr;
		if (oldnode != NULL) {
			php_libxml_node_free_resource(oldnode  TSRMLS_CC);
		}
		php_libxml_increment_node_ptr((php_libxml_node_object *)intern, nodep, (void *)intern TSRMLS_CC);
	}
}
/* }}} end DOMElement::__construct */

/* {{{ tagName	string	
readonly=yes 
URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#core-ID-104682815
Since: 
*/
int dom_element_tag_name_read(dom_object *obj, zval **retval TSRMLS_DC)
{
	xmlNodePtr nodep;
	xmlNsPtr ns;
	xmlChar *qname;

	nodep = dom_object_get_node(obj);

	if (nodep == NULL) {
		php_dom_throw_error(INVALID_STATE_ERR, 0 TSRMLS_CC);
		return FAILURE;
	}

	ALLOC_ZVAL(*retval);
	ns = nodep->ns;
	if (ns != NULL && ns->prefix) {
		qname = xmlStrdup(ns->prefix);
		qname = xmlStrcat(qname, ":");
		qname = xmlStrcat(qname, nodep->name);
		ZVAL_STRING(*retval, qname, 1);
		xmlFree(qname);
	} else {
		ZVAL_STRING(*retval, (char *) nodep->name, 1);
	}

	return SUCCESS;
}

/* }}} */



/* {{{ schemaTypeInfo	typeinfo	
readonly=yes 
URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#Element-schemaTypeInfo
Since: DOM Level 3
*/
int dom_element_schema_type_info_read(dom_object *obj, zval **retval TSRMLS_DC)
{
	ALLOC_ZVAL(*retval);
	ZVAL_NULL(*retval);
	return SUCCESS;
}

/* }}} */



/* {{{ proto string dom_element_get_attribute(string name);
URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#core-ID-666EE0F9
Since:
*/
PHP_FUNCTION(dom_element_get_attribute)
{
	zval *id;
	xmlNode *nodep;
	char *name, *value;
	dom_object *intern;
	int name_len;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os", &id, dom_element_class_entry, &name, &name_len) == FAILURE) {
		return;
	}

	DOM_GET_OBJ(nodep, id, xmlNodePtr, intern);

	value = xmlGetProp(nodep, name);
	if (value == NULL) {
		RETURN_EMPTY_STRING();
	} else {
		RETVAL_STRING(value, 1);
		xmlFree(value);
	}
}
/* }}} end dom_element_get_attribute */


/* {{{ proto void dom_element_set_attribute(string name, string value);
URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#core-ID-F68F082
Since: 
*/
PHP_FUNCTION(dom_element_set_attribute)
{
	zval *id, *rv = NULL;
	xmlNode *nodep;
	xmlAttr *attr;
	int ret, name_len, value_len;
	dom_object *intern;
	char *name, *value;


	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss", &id, dom_element_class_entry, &name, &name_len, &value, &value_len) == FAILURE) {
		return;
	}

	if (name_len == 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Attribute Name is required");
		RETURN_FALSE;
	}

	DOM_GET_OBJ(nodep, id, xmlNodePtr, intern);

	if (dom_node_is_read_only(nodep) == SUCCESS) {
		php_dom_throw_error(NO_MODIFICATION_ALLOWED_ERR, dom_get_strict_error(intern->document) TSRMLS_CC);
		RETURN_FALSE;
	}

	attr = xmlHasProp(nodep,name);
	if (attr != NULL && attr->type != XML_ATTRIBUTE_DECL) {
		node_list_unlink(attr->children TSRMLS_CC);
	}
	attr = xmlSetProp(nodep, name, value);
	if (!attr) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "No such attribute '%s'", name);
		RETURN_FALSE;
	}

	DOM_RET_OBJ(rv, (xmlNodePtr) attr, &ret, intern);

}
/* }}} end dom_element_set_attribute */


/* {{{ proto void dom_element_remove_attribute(string name);
URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#core-ID-6D6AC0F9
Since:
*/
PHP_FUNCTION(dom_element_remove_attribute)
{
	zval *id;
	xmlNode *nodep;
	xmlAttr *attrp;
	dom_object *intern;
	int name_len;
	char *name;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os", &id, dom_element_class_entry, &name, &name_len) == FAILURE) {
		return;
	}

	DOM_GET_OBJ(nodep, id, xmlNodePtr, intern);

	if (dom_node_is_read_only(nodep) == SUCCESS) {
		php_dom_throw_error(NO_MODIFICATION_ALLOWED_ERR, dom_get_strict_error(intern->document) TSRMLS_CC);
		RETURN_FALSE;
	}

	attrp = xmlHasProp(nodep,name);
	if (attrp == NULL) {
		RETURN_FALSE;
	}

	if (attrp->type != XML_ATTRIBUTE_DECL) {
		if (php_dom_object_get_data((xmlNodePtr) attrp) == NULL) {
			node_list_unlink(attrp->children TSRMLS_CC);
			xmlUnlinkNode((xmlNodePtr) attrp);
			xmlFreeProp(attrp);
		} else {
			xmlUnlinkNode((xmlNodePtr) attrp);
		}
	}

	RETURN_TRUE;
}
/* }}} end dom_element_remove_attribute */


/* {{{ proto DOMAttr dom_element_get_attribute_node(string name);
URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#core-ID-217A91B8
Since: 
*/
PHP_FUNCTION(dom_element_get_attribute_node)
{
	zval *id, *rv = NULL;
	xmlNode *nodep;
	xmlAttr  *attrp;
	int name_len, ret;
	dom_object *intern;
	char *name;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os", &id, dom_element_class_entry, &name, &name_len) == FAILURE) {
		return;
	}

	DOM_GET_OBJ(nodep, id, xmlNodePtr, intern);

	attrp = xmlHasProp(nodep,name);
	if (attrp == NULL) {
		RETURN_FALSE;
	}

	DOM_RET_OBJ(rv, (xmlNodePtr) attrp, &ret, intern);
}
/* }}} end dom_element_get_attribute_node */


/* {{{ proto DOMAttr dom_element_set_attribute_node(DOMAttr newAttr);
URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#core-ID-887236154
Since: 
*/
PHP_FUNCTION(dom_element_set_attribute_node)
{
	zval *id, *node, *rv = NULL;
	xmlNode *nodep;
	xmlAttr *attrp, *existattrp = NULL;
	dom_object *intern, *attrobj, *oldobj;
	int ret;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "OO", &id, dom_element_class_entry, &node, dom_attr_class_entry) == FAILURE) {
		return;
	}

	DOM_GET_OBJ(nodep, id, xmlNodePtr, intern);

	if (dom_node_is_read_only(nodep) == SUCCESS) {
		php_dom_throw_error(NO_MODIFICATION_ALLOWED_ERR, dom_get_strict_error(intern->document) TSRMLS_CC);
		RETURN_FALSE;
	}

	DOM_GET_OBJ(attrp, node, xmlAttrPtr, attrobj);

	if (attrp->type != XML_ATTRIBUTE_NODE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Attribute node is required");
		RETURN_FALSE;
	}

	existattrp = xmlHasProp(nodep, attrp->name);
	if (existattrp != NULL && existattrp->type != XML_ATTRIBUTE_DECL) {
		if ((oldobj = php_dom_object_get_data((xmlNodePtr) existattrp)) != NULL && 
			((php_libxml_node_ptr *)oldobj->ptr)->node == (xmlNodePtr) attrp)
		{
			RETURN_NULL();
		}
		xmlUnlinkNode((xmlNodePtr) existattrp);
	}

	if (attrp->doc == NULL && nodep->doc != NULL) {
		attrobj->document = intern->document;
		php_libxml_increment_doc_ref((php_libxml_node_object *)attrobj, NULL TSRMLS_CC);
	}

	xmlAddChild(nodep, (xmlNodePtr) attrp);

	/* Returns old property if removed otherwise NULL */
	if (existattrp != NULL) {
		DOM_RET_OBJ(rv, (xmlNodePtr) existattrp, &ret, intern);
	} else {
		RETVAL_NULL();
	}

}
/* }}} end dom_element_set_attribute_node */


/* {{{ proto DOMAttr dom_element_remove_attribute_node(DOMAttr oldAttr);
URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#core-ID-D589198
Since: 
*/
PHP_FUNCTION(dom_element_remove_attribute_node)
{
	zval *id, *node, *rv = NULL;
	xmlNode *nodep;
	xmlAttr *attrp;
	dom_object *intern, *attrobj;
	int ret;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "OO", &id, dom_element_class_entry, &node, dom_attr_class_entry) == FAILURE) {
		return;
	}

	DOM_GET_OBJ(nodep, id, xmlNodePtr, intern);

	if (dom_node_is_read_only(nodep) == SUCCESS) {
		php_dom_throw_error(NO_MODIFICATION_ALLOWED_ERR, dom_get_strict_error(intern->document) TSRMLS_CC);
		RETURN_FALSE;
	}

	DOM_GET_OBJ(attrp, node, xmlAttrPtr, attrobj);

	if (attrp->type != XML_ATTRIBUTE_NODE || attrp->parent != nodep) {
		php_dom_throw_error(NOT_FOUND_ERR, dom_get_strict_error(intern->document) TSRMLS_CC);
		RETURN_FALSE;
	}

	xmlUnlinkNode((xmlNodePtr) attrp);

	DOM_RET_OBJ(rv, (xmlNodePtr) attrp, &ret, intern);

}
/* }}} end dom_element_remove_attribute_node */


/* {{{ proto DOMNodeList dom_element_get_elements_by_tag_name(string name);
URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#core-ID-1938918D
Since: 
*/
PHP_FUNCTION(dom_element_get_elements_by_tag_name)
{
	zval *id;
	xmlNodePtr elemp;
	int name_len;
	dom_object *intern, *namednode;
	char *name;
	xmlChar *local;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os", &id, dom_element_class_entry, &name, &name_len) == FAILURE) {
		return;
	}

	DOM_GET_OBJ(elemp, id, xmlNodePtr, intern);

	php_dom_create_interator(return_value, DOM_NODELIST TSRMLS_CC);
	namednode = (dom_object *)zend_objects_get_address(return_value TSRMLS_CC);
	local = xmlCharStrndup(name, name_len);
	dom_namednode_iter(intern, 0, namednode, NULL, local, NULL TSRMLS_CC);
}
/* }}} end dom_element_get_elements_by_tag_name */


/* {{{ proto string dom_element_get_attribute_ns(string namespaceURI, string localName);
URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#core-ID-ElGetAttrNS
Since: DOM Level 2
*/
PHP_FUNCTION(dom_element_get_attribute_ns)
{
	zval *id;
	xmlNodePtr elemp;
	xmlNsPtr nsptr;
	dom_object *intern;
	int uri_len = 0, name_len = 0;
	char *uri, *name, *strattr;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os!s", &id, dom_element_class_entry, &uri, &uri_len, &name, &name_len) == FAILURE) {
		return;
	}

	DOM_GET_OBJ(elemp, id, xmlNodePtr, intern);

	strattr = xmlGetNsProp(elemp, name, uri);

	if (strattr != NULL) {
		RETVAL_STRING(strattr, 1);
		xmlFree(strattr);
	} else {
		if (xmlStrEqual(uri, DOM_XMLNS_NAMESPACE)) {
			nsptr = dom_get_nsdecl(elemp, name);
			if (nsptr != NULL) {
				RETVAL_STRING((char *) nsptr->href, 1);
			} else {
				RETVAL_EMPTY_STRING();
			}
		} else {
			RETVAL_EMPTY_STRING();
		}
	}

}
/* }}} end dom_element_get_attribute_ns */


/* {{{ proto void dom_element_set_attribute_ns(string namespaceURI, string qualifiedName, string value);
URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#core-ID-ElSetAttrNS
Since: DOM Level 2
*/
PHP_FUNCTION(dom_element_set_attribute_ns)
{
	zval *id;
	xmlNodePtr elemp, nodep = NULL;
	xmlNsPtr nsptr;
	xmlAttr *attr;
	int uri_len = 0, name_len = 0, value_len = 0;
	char *uri, *name, *value;
	char *localname = NULL, *prefix = NULL;
	dom_object *intern;
	int errorcode = 0, stricterror, is_xmlns = 0;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os!ss", &id, dom_element_class_entry, &uri, &uri_len, &name, &name_len, &value, &value_len) == FAILURE) {
		return;
	}

	if (name_len == 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Attribute Name is required");
		RETURN_FALSE;
	}

	DOM_GET_OBJ(elemp, id, xmlNodePtr, intern);

	stricterror = dom_get_strict_error(intern->document);

	if (dom_node_is_read_only(elemp) == SUCCESS) {
		php_dom_throw_error(NO_MODIFICATION_ALLOWED_ERR, stricterror TSRMLS_CC);
		RETURN_NULL();
	}

	errorcode = dom_check_qname(name, &localname, &prefix, uri_len, name_len);

	if (errorcode == 0) {
		if (uri_len > 0) {
			nodep = (xmlNodePtr) xmlHasNsProp(elemp, localname, uri);
			if (nodep != NULL && nodep->type != XML_ATTRIBUTE_DECL) {
				node_list_unlink(nodep->children TSRMLS_CC);
			}

			if (xmlStrEqual(prefix,"xmlns") && xmlStrEqual(uri, DOM_XMLNS_NAMESPACE)) {
				is_xmlns = 1;
				nsptr = dom_get_nsdecl(elemp, localname);
			} else {
				nsptr = xmlSearchNsByHref(elemp->doc, elemp, uri);
				while (nsptr && nsptr->prefix == NULL) {
					nsptr = nsptr->next;
				}
			}

			if (nsptr == NULL) {
				if (prefix == NULL) {
					errorcode = NAMESPACE_ERR;
				} else {
					if (is_xmlns == 1) {
						xmlNewNs(elemp, value, localname);
					} else {
						nsptr = dom_get_ns(elemp, uri, &errorcode, prefix);
					}
				}
			} else {
				if (is_xmlns == 1) {
					if (nsptr->href) {
						xmlFree((xmlChar *) nsptr->href);
					}
					nsptr->href = xmlStrdup(value);
				}
			}

			if (errorcode == 0 && is_xmlns == 0) {
				attr = xmlSetNsProp(elemp, nsptr, localname, value);
			}
		} else {
			attr = xmlHasProp(elemp, localname);
			if (attr != NULL && attr->type != XML_ATTRIBUTE_DECL) {
				node_list_unlink(attr->children TSRMLS_CC);
			}
			attr = xmlSetProp(elemp, localname, value);
		}
	}

	xmlFree(localname);
	if (prefix != NULL) {
		xmlFree(prefix);
	}

	if (errorcode != 0) {
		php_dom_throw_error(errorcode, stricterror TSRMLS_CC);
	}

	RETURN_NULL();
}
/* }}} end dom_element_set_attribute_ns */


/* {{{ proto void dom_element_remove_attribute_ns(string namespaceURI, string localName);
URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#core-ID-ElRemAtNS
Since: DOM Level 2
*/
PHP_FUNCTION(dom_element_remove_attribute_ns)
{
	zval *id;
	xmlNode *nodep;
	xmlAttr *attrp;
	xmlNsPtr nsptr;
	dom_object *intern;
	int name_len, uri_len;
	char *name, *uri;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os!s", &id, dom_element_class_entry, &uri, &uri_len, &name, &name_len) == FAILURE) {
		return;
	}

	DOM_GET_OBJ(nodep, id, xmlNodePtr, intern);

	if (dom_node_is_read_only(nodep) == SUCCESS) {
		php_dom_throw_error(NO_MODIFICATION_ALLOWED_ERR, dom_get_strict_error(intern->document) TSRMLS_CC);
		RETURN_NULL();
	}

	attrp = xmlHasNsProp(nodep, name, uri);

	nsptr = dom_get_nsdecl(nodep, name);
	if (nsptr != NULL) {
		if (xmlStrEqual(uri, nsptr->href)) {
			if (nsptr->href != NULL) {
				xmlFree((char *) nsptr->href);
				nsptr->href = NULL;
			}
			if (nsptr->prefix != NULL) {
				xmlFree((char *) nsptr->prefix);
				nsptr->prefix = NULL;
			}
		} else {
			RETURN_NULL();
		}
	}

	if (attrp && attrp->type != XML_ATTRIBUTE_DECL) {
		if (php_dom_object_get_data((xmlNodePtr) attrp) == NULL) {
			node_list_unlink(attrp->children TSRMLS_CC);
			xmlUnlinkNode((xmlNodePtr) attrp);
			xmlFreeProp(attrp);
		} else {
			xmlUnlinkNode((xmlNodePtr) attrp);
		}
	}

	RETURN_NULL();
}
/* }}} end dom_element_remove_attribute_ns */


/* {{{ proto DOMAttr dom_element_get_attribute_node_ns(string namespaceURI, string localName);
URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#core-ID-ElGetAtNodeNS
Since: DOM Level 2
*/
PHP_FUNCTION(dom_element_get_attribute_node_ns)
{
	zval *id, *rv = NULL;
	xmlNodePtr elemp;
	xmlAttrPtr attrp;
	dom_object *intern;
	int uri_len, name_len, ret;
	char *uri, *name;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss", &id, dom_element_class_entry, &uri, &uri_len, &name, &name_len) == FAILURE) {
		return;
	}

	DOM_GET_OBJ(elemp, id, xmlNodePtr, intern);

	attrp = xmlHasNsProp(elemp, name, uri);

	if (attrp == NULL) {
		RETURN_NULL();
	}

	DOM_RET_OBJ(rv, (xmlNodePtr) attrp, &ret, intern);

}
/* }}} end dom_element_get_attribute_node_ns */


/* {{{ proto DOMAttr dom_element_set_attribute_node_ns(DOMAttr newAttr);
URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#core-ID-ElSetAtNodeNS
Since: DOM Level 2
*/
PHP_FUNCTION(dom_element_set_attribute_node_ns)
{
	zval *id, *node, *rv = NULL;
	xmlNode *nodep;
	xmlNs *nsp;
	xmlAttr *attrp, *existattrp = NULL;
	dom_object *intern, *attrobj, *oldobj;
	int ret;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "OO", &id, dom_element_class_entry, &node, dom_attr_class_entry) == FAILURE) {
		return;
	}

	DOM_GET_OBJ(nodep, id, xmlNodePtr, intern);

	if (dom_node_is_read_only(nodep) == SUCCESS) {
		php_dom_throw_error(NO_MODIFICATION_ALLOWED_ERR, dom_get_strict_error(intern->document) TSRMLS_CC);
		RETURN_FALSE;
	}

	DOM_GET_OBJ(attrp, node, xmlAttrPtr, attrobj);

	if (attrp->type != XML_ATTRIBUTE_NODE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Attribute node is required");
		RETURN_FALSE;
	}

    nsp = attrp->ns;
    if (nsp != NULL) {
        existattrp = xmlHasNsProp(nodep, nsp->href, attrp->name);
    } else {
        existattrp = xmlHasProp(nodep, attrp->name);
    }

	if (existattrp != NULL && existattrp->type != XML_ATTRIBUTE_DECL) {
		if ((oldobj = php_dom_object_get_data((xmlNodePtr) existattrp)) != NULL && 
			((php_libxml_node_ptr *)oldobj->ptr)->node == (xmlNodePtr) attrp)
		{
			RETURN_NULL();
		}
		xmlUnlinkNode((xmlNodePtr) existattrp);
	}

	if (attrp->doc == NULL && nodep->doc != NULL) {
		attrobj->document = intern->document;
		php_libxml_increment_doc_ref((php_libxml_node_object *)attrobj, NULL TSRMLS_CC);
	}

	xmlAddChild(nodep, (xmlNodePtr) attrp);

	/* Returns old property if removed otherwise NULL */
	if (existattrp != NULL) {
		DOM_RET_OBJ(rv, (xmlNodePtr) existattrp, &ret, intern);
	} else {
		RETVAL_NULL();
	}

}
/* }}} end dom_element_set_attribute_node_ns */



/* {{{ proto DOMNodeList dom_element_get_elements_by_tag_name_ns(string namespaceURI, string localName);
URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#core-ID-A6C90942
Since: DOM Level 2
*/
PHP_FUNCTION(dom_element_get_elements_by_tag_name_ns)
{
	zval *id;
	xmlNodePtr elemp;
	int uri_len, name_len;
	dom_object *intern, *namednode;
	char *uri, *name;
	xmlChar *local, *nsuri;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss", &id, dom_element_class_entry, &uri, &uri_len, &name, &name_len) == FAILURE) {
		return;
	}

	DOM_GET_OBJ(elemp, id, xmlNodePtr, intern);

	php_dom_create_interator(return_value, DOM_NODELIST TSRMLS_CC);
	namednode = (dom_object *)zend_objects_get_address(return_value TSRMLS_CC);
	local = xmlCharStrndup(name, name_len);
	nsuri = xmlCharStrndup(uri, uri_len);
	dom_namednode_iter(intern, 0, namednode, NULL, local, nsuri TSRMLS_CC);

}
/* }}} end dom_element_get_elements_by_tag_name_ns */


/* {{{ proto boolean dom_element_has_attribute(string name);
URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#core-ID-ElHasAttr
Since: DOM Level 2
*/
PHP_FUNCTION(dom_element_has_attribute)
{
	zval *id;
	xmlNode *nodep;
	dom_object *intern;
	char *name, *value;
	int name_len;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os", &id, dom_element_class_entry, &name, &name_len) == FAILURE) {
		return;
	}

	DOM_GET_OBJ(nodep, id, xmlNodePtr, intern);

	value = xmlGetProp(nodep, name);
	if (value == NULL) {
		RETURN_FALSE;
	} else {
		xmlFree(value);
		RETURN_TRUE;
	}
}
/* }}} end dom_element_has_attribute */


/* {{{ proto boolean dom_element_has_attribute_ns(string namespaceURI, string localName);
URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#core-ID-ElHasAttrNS
Since: DOM Level 2
*/
PHP_FUNCTION(dom_element_has_attribute_ns)
{
	zval *id;
	xmlNodePtr elemp;
	xmlNs *nsp;
	dom_object *intern;
	int uri_len, name_len;
	char *uri, *name, *value;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os!s", &id, dom_element_class_entry, &uri, &uri_len, &name, &name_len) == FAILURE) {
		return;
	}

	DOM_GET_OBJ(elemp, id, xmlNodePtr, intern);

	value = xmlGetNsProp(elemp, name, uri);

	if (value != NULL) {
		xmlFree(value);
		RETURN_TRUE;
	} else {
		if (xmlStrEqual(uri, DOM_XMLNS_NAMESPACE)) {
			nsp = dom_get_nsdecl(elemp, name);
			if (nsp != NULL) {
				RETURN_TRUE;
			}
		}
	}

	RETURN_FALSE;
}
/* }}} end dom_element_has_attribute_ns */


/* {{{ proto void dom_element_set_id_attribute(string name, boolean isId);
URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#core-ID-ElSetIdAttr
Since: DOM Level 3
*/
PHP_FUNCTION(dom_element_set_id_attribute)
{
 DOM_NOT_IMPLEMENTED();
}
/* }}} end dom_element_set_id_attribute */


/* {{{ proto void dom_element_set_id_attribute_ns(string namespaceURI, string localName, boolean isId);
URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#core-ID-ElSetIdAttrNS
Since: DOM Level 3
*/
PHP_FUNCTION(dom_element_set_id_attribute_ns)
{
 DOM_NOT_IMPLEMENTED();
}
/* }}} end dom_element_set_id_attribute_ns */


/* {{{ proto void dom_element_set_id_attribute_node(attr idAttr, boolean isId);
URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#core-ID-ElSetIdAttrNode
Since: DOM Level 3
*/
PHP_FUNCTION(dom_element_set_id_attribute_node)
{
 DOM_NOT_IMPLEMENTED();
}
/* }}} end dom_element_set_id_attribute_node */

#endif
