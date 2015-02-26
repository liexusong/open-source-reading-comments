 /*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2001 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Sam Ruby (rubys@us.ibm.com)                                  |
   +----------------------------------------------------------------------+
*/

/* $Id: java.c,v 1.42 2001/02/26 06:07:00 andi Exp $ */

/*
 * This module implements Zend OO syntax overloading support for Java
 * components using JNI and reflection.
 */


#include "php.h"
#include "zend_compile.h"
#include "php_ini.h"
#include "php_globals.h"

#ifdef PHP_WIN32
  #include "win32/winutil.h"
  #define DL_ERROR php_win_err()
#else
  #define DL_ERROR dlerror()
#endif

#include <jni.h>

#include <stdio.h>

#define IS_EXCEPTION 86

/***************************************************************************/

#ifndef KAFFE
#ifndef JNI_11
#ifndef JNI_12

#ifdef JNI_VERSION_1_2
#define JNI_12
#else
#define JNI_11
#endif

#endif
#endif
#endif

#ifdef PHP_WIN32
#ifdef JNI_12
#define JAVALIB "jvm.dll"
#else
#define JAVALIB "javai.dll"
#endif
#else
#endif

/***************************************************************************/

static int le_jobject = 0;

static char *classpath = 0;
static char *libpath   = 0;
static char *javahome  = 0;
static char *javalib   = 0;

static int iniUpdated  = 0;
static void *dl_handle = 0;

typedef struct {
  JavaVM *jvm;
  JNIEnv *jenv;
  jobject php_reflect;
  jclass  reflect_class;
} php_java_globals;

#ifdef ZTS
#define JG(v) (java_globals->v)
#define JG_FETCH() php_java_globals *java_globals = ts_resource(java_globals_id)
#define JG_D       php_java_globals *java_globals
#define JG_DC      , JG_D
#define JG_C       java_globals
#define JG_CC      , JG_C
int java_globals_id;
#else
#define JG(v) (java_globals.v)
#define JG_FETCH()
#define JG_D
#define JG_DC
#define JG_C
#define JG_CC
php_java_globals java_globals;
#endif

static zend_class_entry java_class_entry;

static PHP_INI_MH(OnIniUpdate) {
  if (new_value) *(char**)mh_arg1 = new_value;
  iniUpdated=1;
  return SUCCESS;
}

PHP_INI_BEGIN()
  PHP_INI_ENTRY1("java.class.path",
    NULL, PHP_INI_ALL, OnIniUpdate, &classpath)
#ifndef JNI_11
  PHP_INI_ENTRY1("java.home",
    NULL, PHP_INI_ALL, OnIniUpdate, &javahome)
  PHP_INI_ENTRY1("java.library.path",
    NULL, PHP_INI_ALL, OnIniUpdate, &libpath)
#endif
#ifdef JAVALIB
  PHP_INI_ENTRY1("java.library",
    JAVALIB, PHP_INI_ALL, OnIniUpdate, &javalib)
#else
  PHP_INI_ENTRY1("java.library",
    NULL, PHP_INI_ALL, OnIniUpdate, &javalib)
#endif
PHP_INI_END()

/***************************************************************************/

/*
 * Destroy a Java Virtual Machine.
 */
void jvm_destroy() {
  JG_FETCH();

  if (JG(php_reflect)) (*JG(jenv))->DeleteGlobalRef(JG(jenv), JG(php_reflect));
  if (JG(jvm)) {
    (*JG(jvm))->DetachCurrentThread(JG(jvm));
    (*JG(jvm))->DestroyJavaVM(JG(jvm));
    JG(jvm) = 0;
  }
  if (dl_handle) DL_UNLOAD(dl_handle);
  JG(php_reflect) = 0;
  JG(jenv) = 0;
}

/*
 * Create a Java Virtual Machine.
 *  - class.path, home, and library.path are read out of the INI file
 *  - appropriate (pre 1.1, JDK 1.1, and JDK 1.2) initialization is performed
 *  - net.php.reflect class file is located
 */

#ifdef JNI_12
static void addJVMOption(JavaVMInitArgs *vm_args, char *name, char *value) {
  char *option = (char*) malloc(strlen(name) + strlen(value) + 1);
  strcpy(option, name);
  strcat(option, value);
  vm_args->options[vm_args->nOptions++].optionString = option;
}
#endif

static int jvm_create() {
  int rc;
  jobject local_php_reflect;
  jthrowable error;

  jint (JNICALL *JNI_CreateVM)(const void*,const void*,void*);
#ifndef JNI_12
  jint (JNICALL *JNI_DefaultArgs)(void*);
#endif

#ifdef JNI_11
  JDK1_1InitArgs vm_args;
#else
  JavaVMInitArgs vm_args;
#ifdef JNI_12
  JavaVMOption options[3];
#endif
#endif

  JG_FETCH();

  iniUpdated=0;

  if (javalib) {
    dl_handle = DL_LOAD(javalib);

    if (!dl_handle) {
      php_error(E_ERROR, "Unable to load Java Library %s, error: %s", 
        javalib, DL_ERROR);
      return -1;
    }
  }

#ifndef JAVALIB
  if (!dl_handle)
    JNI_CreateVM = &JNI_CreateJavaVM;
  else
#endif

  JNI_CreateVM = (jint (JNICALL *)(const void*,const void*,void*))
    DL_FETCH_SYMBOL(dl_handle, "JNI_CreateJavaVM");

  if (!JNI_CreateVM) {
    php_error(E_ERROR, "Unable to locate CreateJavaVM function");
    return -1;
  }

#ifdef JNI_12

  vm_args.version = JNI_VERSION_1_2;
  vm_args.ignoreUnrecognized = JNI_FALSE;
  vm_args.options = options;
  vm_args.nOptions = 0;

  if (classpath) addJVMOption(&vm_args, "-Djava.class.path=",   classpath);
  if (javahome)  addJVMOption(&vm_args, "-Djava.home=",         javahome);
  if (libpath)   addJVMOption(&vm_args, "-Djava.library.path=", libpath);

#else

#ifndef JAVALIB
  if (!dl_handle)
    JNI_DefaultArgs = &JNI_GetDefaultJavaVMInitArgs;
  else
#endif

  JNI_DefaultArgs = (jint (JNICALL *)(void*))
    DL_FETCH_SYMBOL(dl_handle, "JNI_GetDefaultJavaVMInitArgs");

  if (!JNI_DefaultArgs) {
    php_error(E_ERROR, "Unable to locate GetDefaultJavaVMInitArgs function");
    return -1;
  }

  vm_args.version=0x00010001;
  (*JNI_DefaultArgs)(&vm_args);

  if (!classpath) classpath = "";
  vm_args.classpath = classpath;
#ifdef KAFFE
  vm_args.classhome = javahome;
  vm_args.libraryhome = libpath;
#endif

#endif

  rc = (*JNI_CreateVM)(&JG(jvm), &JG(jenv), &vm_args);

  if (rc) {
    php_error(E_ERROR, "Unable to create Java Virtual Machine");
    return rc;
  }

  JG(reflect_class) = (*JG(jenv))->FindClass(JG(jenv), "net/php/reflect");
  error = (*JG(jenv))->ExceptionOccurred(JG(jenv));
  if (error) {
    jclass errClass;
    jmethodID toString;
    jobject errString;
    const char *errAsUTF;
    jboolean isCopy;
    JNIEnv *jenv = JG(jenv);
    (*jenv)->ExceptionClear(jenv);
    errClass = (*jenv)->GetObjectClass(jenv, error);
    toString = (*jenv)->GetMethodID(jenv, errClass, "toString",
      "()Ljava/lang/String;");
    errString = (*jenv)->CallObjectMethod(jenv, error, toString);
    errAsUTF = (*jenv)->GetStringUTFChars(jenv, errString, &isCopy);
    php_error(E_ERROR, "%s", errAsUTF);
    if (isCopy) (*jenv)->ReleaseStringUTFChars(jenv, error, errAsUTF);
    jvm_destroy();
    return -1;
  }

  local_php_reflect = (*JG(jenv))->AllocObject(JG(jenv), JG(reflect_class));
  JG(php_reflect) = (*JG(jenv))->NewGlobalRef(JG(jenv), local_php_reflect);
  return rc;
}

/***************************************************************************/

static jobject _java_makeObject(pval* arg JG_DC) {
  JNIEnv *jenv = JG(jenv);
  jobject result;
  pval **handle;
  int type;
  jmethodID makeArg;
  jclass hashClass;

  switch (arg->type) {
    case IS_STRING:
      result=(*jenv)->NewByteArray(jenv,arg->value.str.len);
      (*jenv)->SetByteArrayRegion(jenv,(jbyteArray)result,0,
        arg->value.str.len, arg->value.str.val);
      break;

    case IS_OBJECT:
      zend_hash_index_find(arg->value.obj.properties, 0, (void*)&handle);
      result = zend_list_find((*handle)->value.lval, &type);
      break;

    case IS_BOOL:
      makeArg = (*jenv)->GetMethodID(jenv, JG(reflect_class), "MakeArg",
        "(Z)Ljava/lang/Object;");
      result = (*jenv)->CallObjectMethod(jenv, JG(php_reflect), makeArg,
        (jboolean)(arg->value.lval));
      break;

    case IS_LONG:
      makeArg = (*jenv)->GetMethodID(jenv, JG(reflect_class), "MakeArg",
        "(J)Ljava/lang/Object;");
      result = (*jenv)->CallObjectMethod(jenv, JG(php_reflect), makeArg,
        (jlong)(arg->value.lval));
      break;

    case IS_DOUBLE:
      makeArg = (*jenv)->GetMethodID(jenv, JG(reflect_class), "MakeArg",
        "(D)Ljava/lang/Object;");
      result = (*jenv)->CallObjectMethod(jenv, JG(php_reflect), makeArg,
        (jdouble)(arg->value.dval));
      break;

    case IS_ARRAY:
      {
      jobject jkey, jval;
      zval **value;
      zval key;
      char *string_key;
      ulong num_key;
      jobject jold;
      jmethodID put, init;

      hashClass = (*jenv)->FindClass(jenv, "java/util/Hashtable");
      init = (*jenv)->GetMethodID(jenv, hashClass, "<init>", "()V");
      result = (*jenv)->NewObject(jenv, hashClass, init);

      put = (*jenv)->GetMethodID(jenv, hashClass, "put",
        "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

      /* Iterate through hash */
      zend_hash_internal_pointer_reset(arg->value.ht);
      while(zend_hash_get_current_data(arg->value.ht, (void**)&value) == SUCCESS) {
        jval = _java_makeObject(*value JG_CC);

        switch (zend_hash_get_current_key(arg->value.ht, &string_key, &num_key, 0)) {
          case HASH_KEY_IS_STRING:
            key.type = IS_STRING;
            key.value.str.val = string_key;
            key.value.str.len = strlen(string_key);
            jkey = _java_makeObject(&key JG_CC);
            break;
          case HASH_KEY_IS_LONG:
            key.type = IS_LONG;
            key.value.lval = num_key;
            jkey = _java_makeObject(&key JG_CC);
            break;
          default: /* HASH_KEY_NON_EXISTANT */
            jkey = 0;
        }
        jold = (*jenv)->CallObjectMethod(jenv, result, put, jkey, jval);
        if ((*value)->type != IS_OBJECT) (*jenv)->DeleteLocalRef(jenv, jval);
        zend_hash_move_forward(arg->value.ht);
      }

      break;
      }

    default:
      result=0;
  }

  return result;
}

/***************************************************************************/

static jobjectArray _java_makeArray(int argc, pval** argv JG_DC) {
  JNIEnv *jenv = JG(jenv);

  jclass objectClass = (*jenv)->FindClass(jenv, "java/lang/Object");
  jobjectArray result = (*jenv)->NewObjectArray(jenv, argc, objectClass, 0);
  jobject arg;
  int i;

  for (i=0; i<argc; i++) {
    arg = _java_makeObject(argv[i] JG_CC);
    (*jenv)->SetObjectArrayElement(jenv, result, i, arg);
    if (argv[i]->type != IS_OBJECT) (*jenv)->DeleteLocalRef(jenv, arg);
  }
  return result;
}

static int checkError(pval *value) {
  if (value->type == IS_EXCEPTION) {
    php_error(E_WARNING, "%s", value->value.str.val);
    efree(value->value.str.val);
    var_reset(value);
    return 1;
  };
  return 0;
}


/***************************************************************************/

/*
 * Invoke a method on an object.  If method name is "java", create a new
 * object instead.
 */
void java_call_function_handler
  (INTERNAL_FUNCTION_PARAMETERS, zend_property_reference *property_reference)
{
  JNIEnv *jenv;

  pval *object = property_reference->object;
  zend_overloaded_element *function_name = (zend_overloaded_element *)
    property_reference->elements_list->tail->data;

  int arg_count = ZEND_NUM_ARGS();
  jlong result = 0;
  pval **arguments = (pval **) emalloc(sizeof(pval *)*arg_count);

  JG_FETCH();

  getParametersArray(ht, arg_count, arguments);

  if (iniUpdated && JG(jenv)) jvm_destroy();
  if (!JG(jenv)) jvm_create();
  if (!JG(jenv)) return;
  jenv = JG(jenv);

  if (!strcmp("java",function_name->element.value.str.val)) {

    /* construct a Java object:
       First argument is the class name.  Any additional arguments will
       be treated as constructor parameters. */

    jmethodID co = (*jenv)->GetMethodID(jenv, JG(reflect_class), "CreateObject",
      "(Ljava/lang/String;[Ljava/lang/Object;J)V");
    jstring className;
    result = (jlong)(long)object;

    if (ZEND_NUM_ARGS() < 1) {
      php_error(E_ERROR, "Missing classname in new Java() call");
      return;
    }

    className=(*jenv)->NewStringUTF(jenv, arguments[0]->value.str.val);
    (*jenv)->CallVoidMethod(jenv, JG(php_reflect), co,
      className, _java_makeArray(arg_count-1, arguments+1 JG_CC), result);

    (*jenv)->DeleteLocalRef(jenv, className);

  } else {

    pval **handle;
    int type;
    jobject obj;
    jstring method;

    /* invoke a method on the given object */

    jmethodID invoke = (*jenv)->GetMethodID(jenv, JG(reflect_class), "Invoke",
      "(Ljava/lang/Object;Ljava/lang/String;[Ljava/lang/Object;J)V");
    zend_hash_index_find(object->value.obj.properties, 0, (void**) &handle);
    obj = zend_list_find((*handle)->value.lval, &type);
    method = (*jenv)->NewStringUTF(jenv, function_name->element.value.str.val);
    result = (jlong)(long)return_value;

    (*jenv)->CallVoidMethod(jenv, JG(php_reflect), invoke,
      obj, method, _java_makeArray(arg_count, arguments JG_CC), result);

    (*jenv)->DeleteLocalRef(jenv, method);

  }

  efree(arguments);
  pval_destructor(&function_name->element);

  checkError((pval*)(long)result);
}

/***************************************************************************/

PHP_FUNCTION(java_last_exception_get)
{
  jlong result = 0;
  jmethodID lastEx;

  JG_FETCH();

  if (ZEND_NUM_ARGS()!=0) WRONG_PARAM_COUNT;

  result = (jlong)(long)return_value;
  
  lastEx = (*JG(jenv))->GetMethodID(JG(jenv), JG(reflect_class), 
          "lastException", "(J)V");

  (*JG(jenv))->CallVoidMethod(JG(jenv), JG(php_reflect), lastEx, result);
}

/***************************************************************************/

PHP_FUNCTION(java_last_exception_clear)
{
  jlong result = 0;
  jmethodID clearEx;

  JG_FETCH();

  if (ZEND_NUM_ARGS()!=0) WRONG_PARAM_COUNT;

  result = (jlong)(long)return_value;
  
  clearEx = (*JG(jenv))->GetMethodID(JG(jenv), JG(reflect_class), 
          "clearException", "()V");

  (*JG(jenv))->CallVoidMethod(JG(jenv), JG(php_reflect), clearEx);
}

/***************************************************************************/

static pval _java_getset_property
  (zend_property_reference *property_reference, jobjectArray value)
{
  pval presult;
  jlong result = 0;
  pval **pobject;
  jobject obj;
  int type;

  /* get the property name */
  zend_llist_element *element = property_reference->elements_list->head;
  zend_overloaded_element *property=(zend_overloaded_element *)element->data;
  jstring propName;

  JNIEnv *jenv;
  JG_FETCH(); 
  jenv = JG(jenv);

  propName = (*jenv)->NewStringUTF(jenv, property->element.value.str.val);

  /* get the object */
  zend_hash_index_find(property_reference->object->value.obj.properties,
    0, (void **) &pobject);
  obj = zend_list_find((*pobject)->value.lval,&type);
  result = (jlong)(long) &presult;
  var_uninit(&presult);

  if (!obj || (type!=le_jobject)) {
    php_error(E_ERROR,
      "Attempt to access a Java property on a non-Java object");
  } else {
    /* invoke the method */
    jmethodID gsp = (*jenv)->GetMethodID(jenv, JG(reflect_class), "GetSetProp",
      "(Ljava/lang/Object;Ljava/lang/String;[Ljava/lang/Object;J)V");
    (*jenv)->CallVoidMethod
       (jenv, JG(php_reflect), gsp, obj, propName, value, result);
  }

  (*jenv)->DeleteLocalRef(jenv, propName);
  pval_destructor(&property->element);
  return presult;
}

pval java_get_property_handler
  (zend_property_reference *property_reference)
{
  pval presult = _java_getset_property(property_reference, 0);
  checkError(&presult);
  return presult;
}


int java_set_property_handler
  (zend_property_reference *property_reference, pval *value)
{
  pval presult;
  JG_FETCH();
  presult = _java_getset_property
    (property_reference, _java_makeArray(1, &value JG_CC));
  return checkError(&presult) ? FAILURE : SUCCESS;
}

/***************************************************************************/

static void _php_java_destructor(zend_rsrc_list_entry *rsrc)
{
	void *jobject = (void *)rsrc->ptr;
  JG_FETCH();
  if (JG(jenv)) (*JG(jenv))->DeleteGlobalRef(JG(jenv), jobject);
}

#ifdef ZTS
static void alloc_java_globals_ctor(php_java_globals *java_globals) {
  memset(java_globals, 0, sizeof(php_java_globals));
}
#endif

PHP_MINIT_FUNCTION(java) {
  INIT_OVERLOADED_CLASS_ENTRY(java_class_entry, "java", NULL,
    java_call_function_handler,
    java_get_property_handler,
    java_set_property_handler);

  zend_register_internal_class(&java_class_entry);

  le_jobject = zend_register_list_destructors_ex(_php_java_destructor, NULL, "java", module_number);

  REGISTER_INI_ENTRIES();

  if (!classpath) classpath = getenv("CLASSPATH");

  if (!libpath) {
    PLS_FETCH();
    libpath=PG(extension_dir);
  }

#ifdef ZTS
  java_globals_id = ts_allocate_id(sizeof(php_java_globals), 
    (ts_allocate_ctor)alloc_java_globals_ctor, NULL);
#endif

  return SUCCESS;
}


PHP_MSHUTDOWN_FUNCTION(java) {
  JG_FETCH();
  UNREGISTER_INI_ENTRIES();
  if (JG(jvm)) jvm_destroy();
  return SUCCESS;
}

function_entry java_functions[] = {
  PHP_FE(java_last_exception_get, NULL)
  PHP_FE(java_last_exception_clear, NULL)
  {NULL, NULL, NULL}
};


static PHP_MINFO_FUNCTION(java) {
  DISPLAY_INI_ENTRIES();
}

zend_module_entry java_module_entry = {
  "java",
  java_functions,
  PHP_MINIT(java),
  PHP_MSHUTDOWN(java),
  NULL,
  NULL,
  PHP_MINFO(java),
  STANDARD_MODULE_PROPERTIES
};

ZEND_GET_MODULE(java)

/***************************************************************************/

JNIEXPORT void JNICALL Java_net_php_reflect_setResultFromString
  (JNIEnv *jenv, jclass self, jlong result, jbyteArray jvalue)
{
  jboolean isCopy;
  jbyte *value = (*jenv)->GetByteArrayElements(jenv, jvalue, &isCopy);
  pval *presult = (pval*)(long)result;
  presult->type=IS_STRING;
  presult->value.str.len=(*jenv)->GetArrayLength(jenv, jvalue);
  presult->value.str.val=emalloc(presult->value.str.len+1);
  memcpy(presult->value.str.val, value, presult->value.str.len);
  presult->value.str.val[presult->value.str.len]=0;
  if (isCopy) (*jenv)->ReleaseByteArrayElements(jenv, jvalue, value, 0);
}

JNIEXPORT void JNICALL Java_net_php_reflect_setResultFromLong
  (JNIEnv *jenv, jclass self, jlong result, jlong value)
{
  pval *presult = (pval*)(long)result;
  presult->type=IS_LONG;
  presult->value.lval=(long)value;
}

JNIEXPORT void JNICALL Java_net_php_reflect_setResultFromDouble
  (JNIEnv *jenv, jclass self, jlong result, jdouble value)
{
  pval *presult = (pval*)(long)result;
  presult->type=IS_DOUBLE;
  presult->value.dval=value;
}

JNIEXPORT void JNICALL Java_net_php_reflect_setResultFromBoolean
  (JNIEnv *jenv, jclass self, jlong result, jboolean value)
{
  pval *presult = (pval*)(long)result;
  presult->type=IS_BOOL;
  presult->value.lval=value;
}

JNIEXPORT void JNICALL Java_net_php_reflect_setResultFromObject
  (JNIEnv *jenv, jclass self, jlong result, jobject value)
{
  /* wrapper the java object in a pval object */
  pval *presult = (pval*)(long)result;
  pval *handle;

  if (presult->type != IS_OBJECT) {
    presult->type=IS_OBJECT;
    presult->value.obj.ce=&java_class_entry;
    presult->value.obj.properties = (HashTable *) emalloc(sizeof(HashTable));
    presult->is_ref=1;
    presult->refcount=1;
    zend_hash_init(presult->value.obj.properties, 0, NULL, ZVAL_PTR_DTOR, 0);
  };

  ALLOC_ZVAL(handle);
  handle->type = IS_LONG;
  handle->value.lval =
    zend_list_insert((*jenv)->NewGlobalRef(jenv,value), le_jobject);
  pval_copy_constructor(handle);
  INIT_PZVAL(handle);
  zend_hash_index_update(presult->value.obj.properties, 0,
    &handle, sizeof(pval *), NULL);
}

JNIEXPORT void JNICALL Java_net_php_reflect_setResultFromArray
  (JNIEnv *jenv, jclass self, jlong result)
{
  array_init( (pval*)(long)result );
}

JNIEXPORT jlong JNICALL Java_net_php_reflect_nextElement
  (JNIEnv *jenv, jclass self, jlong array)
{
  pval *result;
  pval *handle = (pval*)(long)array;
  ALLOC_ZVAL(result);
  zend_hash_next_index_insert(handle->value.ht, &result, sizeof(zval *), NULL);
  return (jlong)(long)result;
}

JNIEXPORT jlong JNICALL Java_net_php_reflect_hashIndexUpdate
  (JNIEnv *jenv, jclass self, jlong array, jlong key)
{
  pval *result;
  pval *handle = (pval*)(long)array;
  ALLOC_ZVAL(result);
  zend_hash_index_update(handle->value.ht, (unsigned long)key, 
    &result, sizeof(zval *), NULL);
  return (jlong)(long)result;
}

JNIEXPORT jlong JNICALL Java_net_php_reflect_hashUpdate
  (JNIEnv *jenv, jclass self, jlong array, jbyteArray key)
{
  pval *result;
  pval pkey;
  pval *handle = (pval*)(long)array;
  ALLOC_ZVAL(result);
  Java_net_php_reflect_setResultFromString(jenv, self, (jlong)(long)&pkey, key);
  zend_hash_update(handle->value.ht, pkey.value.str.val, pkey.value.str.len+1,
    &result, sizeof(zval *), NULL);
  return (jlong)(long)result;
}

JNIEXPORT void JNICALL Java_net_php_reflect_setException
  (JNIEnv *jenv, jclass self, jlong result, jbyteArray value)
{
  pval *presult = (pval*)(long)result;
  Java_net_php_reflect_setResultFromString(jenv, self, result, value);
  presult->type=IS_EXCEPTION;
}

JNIEXPORT void JNICALL Java_net_php_reflect_setEnv
  (JNIEnv *newJenv, jclass self)
{
  jobject local_php_reflect;
  JG_FETCH();

  iniUpdated=0;
  JG(jenv)=newJenv;

  if (!self) self = (*JG(jenv))->FindClass(JG(jenv), "net/php/reflect");
  JG(reflect_class) = self;

  if (JG(php_reflect)) (*JG(jenv))->DeleteGlobalRef(JG(jenv), JG(php_reflect));
  local_php_reflect = (*JG(jenv))->AllocObject(JG(jenv), JG(reflect_class));
  JG(php_reflect) = (*JG(jenv))->NewGlobalRef(JG(jenv), local_php_reflect);
}
