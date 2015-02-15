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

/* $Id: xsltprocessor.c,v 1.29.2.6 2005/06/14 19:40:33 rrichards Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_xsl.h"
#include "ext/libxml/php_libxml.h"

/*
* class xsl_xsltprocessor 
*
* URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#
* Since: 
*/

zend_function_entry php_xsl_xsltprocessor_class_functions[] = {
	PHP_FALIAS(importStylesheet, xsl_xsltprocessor_import_stylesheet, NULL)
	PHP_FALIAS(transformToDoc, xsl_xsltprocessor_transform_to_doc, NULL)
	PHP_FALIAS(transformToUri, xsl_xsltprocessor_transform_to_uri, NULL)
	PHP_FALIAS(transformToXml, xsl_xsltprocessor_transform_to_xml, NULL)
	PHP_FALIAS(setParameter, xsl_xsltprocessor_set_parameter, NULL)
	PHP_FALIAS(getParameter, xsl_xsltprocessor_get_parameter, NULL)
	PHP_FALIAS(removeParameter, xsl_xsltprocessor_remove_parameter, NULL)
	PHP_FALIAS(hasExsltSupport, xsl_xsltprocessor_has_exslt_support, NULL)
	PHP_FALIAS(registerPHPFunctions, xsl_xsltprocessor_register_php_functions, NULL)
	{NULL, NULL, NULL}
};

/* {{{ attribute protos, not implemented yet */
/* {{{ php_xsl_xslt_string_to_xpathexpr()
   Translates a string to a XPath Expression */
static char *php_xsl_xslt_string_to_xpathexpr(const char *str TSRMLS_DC)
{
	const xmlChar *string = (const xmlChar *)str;

	xmlChar *value;
	int str_len;
	
	str_len = xmlStrlen(string) + 3;
	
	if (xmlStrchr(string, '"')) {
		if (xmlStrchr(string, '\'')) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot create XPath expression (string contains both quote and double-quotes)");
			return NULL;
		}
		value = (xmlChar*) safe_emalloc (str_len, sizeof(xmlChar), 0);
		snprintf(value, str_len, "'%s'", string);
	} else {
		value = (xmlChar*) safe_emalloc (str_len, sizeof(xmlChar), 0);
		snprintf(value, str_len, "\"%s\"", string);
	}
	return (char *) value;
}


/* {{{ php_xsl_xslt_make_params()
   Translates a PHP array to a libxslt parameters array */
static char **php_xsl_xslt_make_params(HashTable *parht, int xpath_params TSRMLS_DC)
{
	
	int parsize;
	zval **value;
	char *xpath_expr, *string_key = NULL;
	ulong num_key;
	char **params = NULL;
	int i = 0;

	parsize = (2 * zend_hash_num_elements(parht) + 1) * sizeof(char *);
	params = (char **)emalloc(parsize);
	memset((char *)params, 0, parsize);

	for (zend_hash_internal_pointer_reset(parht);
		zend_hash_get_current_data(parht, (void **)&value) == SUCCESS;
		zend_hash_move_forward(parht)) {

		if (zend_hash_get_current_key(parht, &string_key, &num_key, 1) != HASH_KEY_IS_STRING) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid argument or parameter array");
			efree(params);
			return NULL;
		} else {
			if (Z_TYPE_PP(value) != IS_STRING) {
				SEPARATE_ZVAL(value);
				convert_to_string(*value);
			}

			if (!xpath_params) {
				xpath_expr = php_xsl_xslt_string_to_xpathexpr(Z_STRVAL_PP(value) TSRMLS_CC);
			} else {
				xpath_expr = estrndup(Z_STRVAL_PP(value), strlen(Z_STRVAL_PP(value)));
			}
			if (xpath_expr) {
				params[i++] = string_key;
				params[i++] = xpath_expr;
			}
		}
	}

	params[i++] = NULL;

	return params;
}
/* }}} */


static void xsl_ext_function_php(xmlXPathParserContextPtr ctxt, int nargs, int type)
{
	xsltTransformContextPtr tctxt;
	zval **args;
	zval *retval;
	int result, i, ret;
	int error = 0;
	zend_fcall_info fci;
	zval handler;
	xmlXPathObjectPtr obj;
	char *str;
	char *callable = NULL;
	xsl_object *intern;
	
	TSRMLS_FETCH();

	if (! zend_is_executing(TSRMLS_C)) {
		xsltGenericError(xsltGenericErrorContext,
		"xsltExtFunctionTest: Function called from outside of PHP\n");
		error = 1;
	} else {
	tctxt = xsltXPathGetTransformContext(ctxt);
	if (tctxt == NULL) {
		xsltGenericError(xsltGenericErrorContext,
		"xsltExtFunctionTest: failed to get the transformation context\n");
			error = 1;
		} else {
			intern = (xsl_object *) tctxt->_private;
			if (intern == NULL) {
				xsltGenericError(xsltGenericErrorContext,
				"xsltExtFunctionTest: failed to get the internal object\n");
				error = 1;
			}
			else if (intern->registerPhpFunctions == 0) {
				xsltGenericError(xsltGenericErrorContext,
				"xsltExtFunctionTest: PHP Object did not register PHP functions\n");
				error = 1;
			}
		}
	}
	
	if (error == 1) {
		for (i = nargs - 1; i >= 0; i--) {
			obj = valuePop(ctxt);
			xmlXPathFreeObject(obj);
		}
		return;
	}

	fci.param_count = nargs - 1;
	if (fci.param_count > 0) {
		fci.params = safe_emalloc(fci.param_count, sizeof(zval**), 0);
		args = safe_emalloc(fci.param_count, sizeof(zval *), 0);
	}
	/* Reverse order to pop values off ctxt stack */
	for (i = nargs - 2; i >= 0; i--) {
		obj = valuePop(ctxt);
		MAKE_STD_ZVAL(args[i]);
		switch (obj->type) {
			case XPATH_STRING:
				ZVAL_STRING(args[i],  obj->stringval, 1);
				break;
			case XPATH_BOOLEAN:
				ZVAL_BOOL(args[i],  obj->boolval);
				break;
			case XPATH_NUMBER:
				ZVAL_DOUBLE(args[i], obj->floatval);
				break;
			case XPATH_NODESET:
				if (type == 1) {
					str = xmlXPathCastToString(obj);
					ZVAL_STRING(args[i], str, 1);
					xmlFree(str);
				} else if (type == 2) {
					int j;
					dom_object *domintern;
					array_init(args[i]);
					if (obj->nodesetval && obj->nodesetval->nodeNr > 0) {
						domintern = (dom_object *) php_dom_object_get_data((void *) obj->nodesetval->nodeTab[0]->doc);
						for (j = 0; j < obj->nodesetval->nodeNr; j++) {
							xmlNodePtr node = obj->nodesetval->nodeTab[j];
							zval *child;
							
							MAKE_STD_ZVAL(child);
							/* not sure, if we need this... it's copied from xpath.c */
							if (node->type == XML_NAMESPACE_DECL) {
								xmlNsPtr curns;
								xmlNodePtr nsparent;
								
								nsparent = node->_private;
								curns = xmlNewNs(NULL, node->name, NULL);
								if (node->children) {
									curns->prefix = xmlStrdup((char *) node->children);
								}
								if (node->children) {
									node = xmlNewDocNode(node->doc, NULL, (char *) node->children, node->name);
								} else {
									node = xmlNewDocNode(node->doc, NULL, "xmlns", node->name);
								}
								node->type = XML_NAMESPACE_DECL;
								node->parent = nsparent;
								node->ns = curns;
							}
							child = php_dom_create_object(node, &ret, NULL, child, domintern TSRMLS_CC);
							add_next_index_zval(args[i], child);
						}
					}
				}
				break;
			default:
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "php:function object type %d is not supported yet", obj->type);
			ZVAL_STRING(args[i], "", 0);
		}
		xmlXPathFreeObject(obj);
		fci.params[i] = &args[i];
	}
	
	fci.size = sizeof(fci);
	fci.function_table = EG(function_table);
	
	obj = valuePop(ctxt);
	if (obj->stringval == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Handler name must be a string");
		xmlXPathFreeObject(obj);
		if (fci.param_count > 0) {
			for (i = 0; i < nargs - 1; i++) {
				zval_ptr_dtor(&args[i]);
			}
			efree(args);
			efree(fci.params);
		}
		return; 
	}
	INIT_PZVAL(&handler);
	ZVAL_STRING(&handler, obj->stringval, 1);
	xmlXPathFreeObject(obj);
	
	fci.function_name = &handler;
	fci.symbol_table = NULL;
	fci.object_pp = NULL;
	fci.retval_ptr_ptr = &retval;
	fci.no_separation = 0;
	/*fci.function_handler_cache = &function_ptr;*/
	if (!zend_make_callable(&handler, &callable TSRMLS_CC)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to call handler %s()", callable);
		
	} else {
		result = zend_call_function(&fci, NULL TSRMLS_CC);
		if (result == FAILURE) {
			if (Z_TYPE(handler) == IS_STRING) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to call handler %s()", Z_STRVAL_P(&handler));
			}
		/* retval is == NULL, when an exception occured, don't report anything, because PHP itself will handle that */
		} else if (retval == NULL) {
		} else {
			if (retval->type == IS_OBJECT && instanceof_function( Z_OBJCE_P(retval), dom_node_class_entry TSRMLS_CC)) {
				xmlNode *nodep;
				dom_object *obj;
				if (intern->node_list == NULL) {
					ALLOC_HASHTABLE(intern->node_list);
					zend_hash_init(intern->node_list, 0, NULL, ZVAL_PTR_DTOR, 0);
				}
				zval_add_ref(&retval);
				zend_hash_next_index_insert(intern->node_list, &retval, sizeof(zval *), NULL);
				obj = (dom_object *)zend_object_store_get_object(retval TSRMLS_CC);
				nodep = dom_object_get_node(obj);
				valuePush(ctxt, xmlXPathNewNodeSet(nodep));
			} else if (retval->type == IS_BOOL) {
				valuePush(ctxt, xmlXPathNewBoolean(retval->value.lval));
			} else if (retval->type == IS_OBJECT) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "A PHP Object can not be converted to a XPath-string");
				valuePush(ctxt, xmlXPathNewString(""));
			} else {
				convert_to_string_ex(&retval);
				valuePush(ctxt, xmlXPathNewString( Z_STRVAL_P(retval)));
			}
			zval_ptr_dtor(&retval);
		}
	}
	efree(callable);
	zval_dtor(&handler);
	if (fci.param_count > 0) {
		for (i = 0; i < nargs - 1; i++) {
			zval_ptr_dtor(&args[i]);
		}
		efree(args);
		efree(fci.params);
	}
}

void xsl_ext_function_string_php(xmlXPathParserContextPtr ctxt, int nargs)
{
	xsl_ext_function_php(ctxt, nargs, 1);
}

void xsl_ext_function_object_php(xmlXPathParserContextPtr ctxt, int nargs)
{
	xsl_ext_function_php(ctxt, nargs, 2);
}


/* {{{ proto void xsl_xsltprocessor_import_stylesheet(domdocument doc);
URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#
Since: 
*/
PHP_FUNCTION(xsl_xsltprocessor_import_stylesheet)
{
	zval *id, *docp = NULL;
	xmlDoc *doc = NULL, *newdoc = NULL;
	xsltStylesheetPtr sheetp, oldsheetp;
	xsl_object *intern;
	int prevSubstValue, prevExtDtdValue, clone_docu = 0;
	xmlNode *nodep = NULL;
	zend_object_handlers *std_hnd;
	zval *cloneDocu, *member;
	
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oo", &id, xsl_xsltprocessor_class_entry, &docp) == FAILURE) {
		RETURN_FALSE;
	}

	nodep = php_libxml_import_node(docp TSRMLS_CC);
	
	if (nodep) {
		doc = nodep->doc;
	}
	if (doc == NULL) {
		php_error(E_WARNING, "Invalid Document");
		RETURN_NULL();
	}

	/* libxslt uses _private, so we must copy the imported 
	stylesheet document otherwise the node proxies will be a mess */
	newdoc = xmlCopyDoc(doc, 1);
	xmlNodeSetBase((xmlNodePtr) newdoc, (xmlChar *)doc->URL);
	prevSubstValue = xmlSubstituteEntitiesDefault(1);
	prevExtDtdValue = xmlLoadExtDtdDefaultValue;
	xmlLoadExtDtdDefaultValue = XML_DETECT_IDS | XML_COMPLETE_ATTRS;

	sheetp = xsltParseStylesheetDoc(newdoc);
	xmlSubstituteEntitiesDefault(prevSubstValue);
	xmlLoadExtDtdDefaultValue = prevExtDtdValue;

	if (!sheetp) {
		xmlFreeDoc(newdoc);
		RETURN_FALSE;
	}

	intern = (xsl_object *)zend_object_store_get_object(id TSRMLS_CC); 

	std_hnd = zend_get_std_object_handlers();
	MAKE_STD_ZVAL(member);
	ZVAL_STRING(member, "cloneDocument", 0);
	cloneDocu = std_hnd->read_property(id, member, BP_VAR_IS TSRMLS_CC);
	if (Z_TYPE_P(cloneDocu) != IS_NULL) {
		convert_to_long(cloneDocu);
		clone_docu = Z_LVAL_P(cloneDocu);
	}
	efree(member);
	if (clone_docu == 0) {
		/* check if the stylesheet is using xsl:key, if yes, we have to clone the document _always_ before a transformation */
		nodep = xmlDocGetRootElement(sheetp->doc)->children;
		while (nodep) {
			if (nodep->type == XML_ELEMENT_NODE && xmlStrEqual(nodep->name, "key") && xmlStrEqual(nodep->ns->href, XSLT_NAMESPACE)) {
				intern->hasKeys = 1;
				break;
			}
			nodep = nodep->next;
		}
	} else {
		intern->hasKeys = clone_docu;
	}
	
	if ((oldsheetp = (xsltStylesheetPtr)intern->ptr)) { 
		/* free wrapper */
		if (((xsltStylesheetPtr) intern->ptr)->_private != NULL) {
			((xsltStylesheetPtr) intern->ptr)->_private = NULL;   
		}
		xsltFreeStylesheet((xsltStylesheetPtr) intern->ptr);
		intern->ptr = NULL;
	}

	php_xsl_set_object(id, sheetp TSRMLS_CC);
}
/* }}} end xsl_xsltprocessor_import_stylesheet */


static xmlDocPtr php_xsl_apply_stylesheet(xsl_object *intern, xsltStylesheetPtr style, xmlDocPtr doc TSRMLS_DC)
{
	xmlDocPtr newdocp;
	xsltTransformContextPtr ctxt;
	char **params = NULL;
	int clone;

	if (style == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "No stylesheet associated to this object");
		return NULL;
	}
	if (intern->parameter) {
		params = php_xsl_xslt_make_params(intern->parameter, 0 TSRMLS_CC);
	}

	if (intern->hasKeys == 1) {
		doc = xmlCopyDoc(doc, 1);
	}

	ctxt = xsltNewTransformContext(style, doc);
	ctxt->_private = (void *) intern;
	
	newdocp = xsltApplyStylesheetUser(style, doc, (const char**) params, NULL, NULL, ctxt);

	xsltFreeTransformContext(ctxt);

	if (intern->node_list != NULL) {
		zend_hash_destroy(intern->node_list);
		FREE_HASHTABLE(intern->node_list);	
		intern->node_list = NULL;
	}

	if (intern->hasKeys == 1) {
		xmlFreeDoc(doc);
	}

	if (params) {
		clone = 0;
		while(params[clone]) {
			efree(params[clone++]);
		}
		efree(params);
	}

	return newdocp;

}

/* {{{ proto domdocument xsl_xsltprocessor_transform_to_doc(domnode doc);
URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#
Since: 
*/
PHP_FUNCTION(xsl_xsltprocessor_transform_to_doc)
{
	zval *id, *rv = NULL, *docp = NULL;
	xmlDoc *doc = NULL;
	xmlNodePtr node = NULL;
	xmlDoc *newdocp;
	xsltStylesheetPtr sheetp;
	int ret;
	xsl_object *intern;
	
	id = getThis();
	intern = (xsl_object *)zend_object_store_get_object(id TSRMLS_CC);
	sheetp = (xsltStylesheetPtr) intern->ptr;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "o", &docp) == FAILURE) {
		RETURN_FALSE;
	}

	node = php_libxml_import_node(docp TSRMLS_CC);
	
	if (node) {
		doc = node->doc;
	}
	if (doc == NULL) {
		php_error(E_WARNING, "Invalid Document");
		RETURN_NULL();
	}

	newdocp = php_xsl_apply_stylesheet(intern, sheetp, doc TSRMLS_CC);

	if (newdocp) {
		DOM_RET_OBJ(rv, (xmlNodePtr) newdocp, &ret, NULL);
	} else {
		RETURN_FALSE;
	}
	
}
/* }}} end xsl_xsltprocessor_transform_to_doc */


/* {{{ proto int xsl_xsltprocessor_transform_to_uri(domdocument doc, string uri);
*/
PHP_FUNCTION(xsl_xsltprocessor_transform_to_uri)
{
	zval *id, *docp = NULL;
	xmlDoc *doc = NULL;
	xmlDoc *newdocp;
	xmlNodePtr node = NULL;
	xsltStylesheetPtr sheetp;
	int ret, uri_len;
	char *uri;
	xsl_object *intern;
	
	id = getThis();
	intern = (xsl_object *)zend_object_store_get_object(id TSRMLS_CC);
	sheetp = (xsltStylesheetPtr) intern->ptr;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "os", &docp, &uri, &uri_len) == FAILURE) {
		RETURN_FALSE;
	}

	node = php_libxml_import_node(docp TSRMLS_CC);
	
	if (node) {
		doc = node->doc;
	}
	if (doc == NULL) {
		php_error(E_WARNING, "Invalid Document");
		RETURN_NULL();
	}

	newdocp = php_xsl_apply_stylesheet(intern, sheetp, doc TSRMLS_CC);

	ret = -1;
	if (newdocp) {
		ret = xsltSaveResultToFilename(uri, newdocp, sheetp, 0);
		xmlFreeDoc(newdocp);
	}

	RETVAL_LONG(ret);
}
/* }}} end xsl_xsltprocessor_transform_to_uri */


/* {{{ proto string xsl_xsltprocessor_transform_to_xml(domdocument doc);
*/
PHP_FUNCTION(xsl_xsltprocessor_transform_to_xml)
{
	zval *id, *docp = NULL;
	xmlDoc *doc = NULL;
	xmlDoc *newdocp;
	xmlNodePtr node = NULL;
	xsltStylesheetPtr sheetp;
	int ret;
	xmlChar *doc_txt_ptr;
	int doc_txt_len;
	xsl_object *intern;
	
	id = getThis();
	intern = (xsl_object *)zend_object_store_get_object(id TSRMLS_CC);
	sheetp = (xsltStylesheetPtr) intern->ptr;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "o", &docp) == FAILURE) {
		RETURN_FALSE;
	}

	node = php_libxml_import_node(docp TSRMLS_CC);
	
	if (node) {
		doc = node->doc;
	}
	if (doc == NULL) {
		php_error(E_WARNING, "Invalid Document");
		RETURN_NULL();
	}

	newdocp = php_xsl_apply_stylesheet(intern, sheetp, doc TSRMLS_CC);

	ret = -1;
	if (newdocp) {
		ret = xsltSaveResultToString(&doc_txt_ptr, &doc_txt_len, newdocp, sheetp);
		if (doc_txt_ptr) {
			RETVAL_STRINGL(doc_txt_ptr, doc_txt_len, 1);
			xmlFree(doc_txt_ptr);
		}
		xmlFreeDoc(newdocp);
	}

	if (ret < 0) {
		RETURN_FALSE;
	}
}
/* }}} end xsl_xsltprocessor_transform_to_xml */


/* {{{ proto xsl_ xsl_xsltprocessor_set_parameter(string namespace, string name, string value);
*/
PHP_FUNCTION(xsl_xsltprocessor_set_parameter)
{
 
	zval *id;
	int name_len = 0, namespace_len = 0, value_len = 0;
	char *name, *namespace, *value;
	xsl_object *intern;
	zval *new_string;

	DOM_GET_THIS(id);
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sss", &namespace, &namespace_len, &name, &name_len, &value, &value_len) == FAILURE) {
		RETURN_FALSE;
	}
	intern = (xsl_object *)zend_object_store_get_object(id TSRMLS_CC);
   

	MAKE_STD_ZVAL(new_string);
	ZVAL_STRING(new_string, value, 1);
	zend_hash_update(intern->parameter, name, name_len + 1, &new_string, sizeof(zval*), NULL);
}
/* }}} end xsl_xsltprocessor_set_parameter */

/* {{{ proto string xsl_xsltprocessor_get_parameter(string namespace, string name);
*/
PHP_FUNCTION(xsl_xsltprocessor_get_parameter)
{
	zval *id;
	int name_len = 0, namespace_len = 0;
	char *name, *namespace;
	zval **value;
	xsl_object *intern;

	DOM_GET_THIS(id);
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &namespace, &namespace_len, &name, &name_len) == FAILURE) {
		RETURN_FALSE;
	}
	intern = (xsl_object *)zend_object_store_get_object(id TSRMLS_CC);
	if ( zend_hash_find(intern->parameter, name, name_len + 1,  (void**) &value) == SUCCESS) {
		convert_to_string_ex(value);
		RETVAL_STRING(Z_STRVAL_PP(value),1);
	} else {
		RETURN_FALSE;
	}
}
/* }}} end xsl_xsltprocessor_get_parameter */

/* {{{ proto bool xsl_xsltprocessor_remove_parameter(string namespace, string name);
*/
PHP_FUNCTION(xsl_xsltprocessor_remove_parameter)
{
	zval *id;
	int name_len = 0, namespace_len = 0;
	char *name, *namespace;
	xsl_object *intern;

	DOM_GET_THIS(id);
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &namespace, &namespace_len, &name, &name_len) == FAILURE) {
		RETURN_FALSE;
	}
	intern = (xsl_object *)zend_object_store_get_object(id TSRMLS_CC);
	if ( zend_hash_del(intern->parameter, name, name_len + 1) == SUCCESS) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} end xsl_xsltprocessor_remove_parameter */

/* {{{ proto void xsl_xsltprocessor_register_php_functions();
*/
PHP_FUNCTION(xsl_xsltprocessor_register_php_functions)
{
	zval *id;
	xsl_object *intern;

	DOM_GET_THIS(id);
	
	
	intern = (xsl_object *)zend_object_store_get_object(id TSRMLS_CC);
	intern->registerPhpFunctions = 1;
	
}
/* }}} end xsl_xsltprocessor_register_php_functions(); */

/* {{{ proto bool xsl_xsltprocessor_has_exslt_support();
*/
PHP_FUNCTION(xsl_xsltprocessor_has_exslt_support)
{
#if HAVE_XSL_EXSLT
	RETURN_TRUE;
#else
	RETURN_FALSE;
#endif
}
/* }}} end xsl_xsltprocessor_has_exslt_support(); */

