/*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2001 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_01.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: dave@opaque.net                                              |
   +----------------------------------------------------------------------+
*/

#include <stdio.h>
#include <math.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"

#if HAVE_MING
#include "ext/standard/info.h"
#include "ext/standard/file.h"
#include "php_ming.h"

static zend_function_entry ming_functions[] = {
  PHP_FALIAS(ming_setcubicthreshold,  ming_setCubicThreshold,  NULL)
  PHP_FALIAS(ming_setscale,           ming_setScale,           NULL)
  PHP_FALIAS(swfbutton_keypress,      swfbutton_keypress,      NULL)
  { NULL, NULL, NULL }
};

static inline SWFMovie getMovie(zval *id);
static inline SWFFill getFill(zval *id);
static inline SWFGradient getGradient(zval *id);
static inline SWFBitmap getBitmap(zval *id);
static inline SWFShape getShape(zval *id);
static inline SWFFont getFont(zval *id);
static inline SWFText getText(zval *id);
static inline SWFTextField getTextField(zval *id);
static inline SWFDisplayItem getDisplayItem(zval *id);
static inline SWFButton getButton(zval *id);
static inline SWFAction getAction(zval *id);
static inline SWFMorph getMorph(zval *id);
static inline SWFMovieClip getSprite(zval *id);

PHP_FUNCTION(ming_setCubicThreshold)
{
  zval **num;

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &num) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_long_ex(num);

  Ming_setCubicThreshold(Z_LVAL_PP(num));
}

PHP_FUNCTION(ming_setScale)
{
  zval **num;

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &num) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(num);

  Ming_setScale(Z_DVAL_PP(num));
}

static int le_swfmoviep;
static int le_swfshapep;
static int le_swffillp;
static int le_swfgradientp;
static int le_swfbitmapp;
static int le_swffontp;
static int le_swftextp;
static int le_swftextfieldp;
static int le_swfdisplayitemp;
static int le_swfbuttonp;
static int le_swfactionp;
static int le_swfmorphp;
static int le_swfspritep;

static int le_fopen;

zend_class_entry movie_class_entry;
zend_class_entry shape_class_entry;
zend_class_entry fill_class_entry;
zend_class_entry gradient_class_entry;
zend_class_entry bitmap_class_entry;
zend_class_entry font_class_entry;
zend_class_entry text_class_entry;
zend_class_entry textfield_class_entry;
zend_class_entry displayitem_class_entry;
zend_class_entry button_class_entry;
zend_class_entry action_class_entry;
zend_class_entry morph_class_entry;
zend_class_entry sprite_class_entry;

/* {{{ internal function SWFgetProperty
 */

static void *SWFgetProperty(zval *id, char *name, int namelen, int proptype)
{
  zval **tmp;
  int id_to_find;
  void *property;
  int type;

  if(id)
  {
    if(zend_hash_find(id->value.obj.properties, name, namelen+1, (void **)&tmp) == FAILURE)
    {
      php_error(E_WARNING, "unable to find property %s", name);
      return NULL;
    }
    id_to_find = (*tmp)->value.lval;
  }
  else
    return NULL;

  property = zend_list_find(id_to_find, &type);

  if (!property || type != proptype)
  {
    php_error(E_WARNING, "unable to find identifier (%d)", id_to_find);
    return NULL;
  }

  return property;
}

/* }}} */
/* {{{ SWFCharacter - not a real class */

/* {{{ internal function SWFCharacter getCharacter(zval *id)
   Returns the SWFCharacter contained in zval *id */

SWFCharacter getCharacter(zval *id)
{
  if(id->value.obj.ce == &shape_class_entry)
    return (SWFCharacter)getShape(id);
  else if(id->value.obj.ce == &font_class_entry)
    return (SWFCharacter)getFont(id);
  else if(id->value.obj.ce == &text_class_entry)
    return (SWFCharacter)getText(id);
  else if(id->value.obj.ce == &textfield_class_entry)
    return (SWFCharacter)getTextField(id);
  else if(id->value.obj.ce == &button_class_entry)
    return (SWFCharacter)getButton(id);
  else if(id->value.obj.ce == &morph_class_entry)
    return (SWFCharacter)getMorph(id);
  else if(id->value.obj.ce == &sprite_class_entry)
    return (SWFCharacter)getSprite(id);
  else
    php_error(E_ERROR, "called object is not an SWFCharacter");
}

/* }}} */

/* }}} */

/* {{{ SWFAction */

static zend_function_entry swfaction_functions[] = {
  PHP_FALIAS(swfaction,              swfaction_init,         NULL)
  { NULL, NULL, NULL }
};

/* {{{ proto object swfaction_init(string)
   returns a new SWFAction object, compiling the given script */

PHP_FUNCTION(swfaction_init)
{
  SWFAction action;
  zval **script;
  int ret;

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &script) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_string_ex(script);

  /* XXX - need to deal with compiler errors */
  action = compileSWFActionCode(Z_STRVAL_PP(script));

  if(!action)
    php_error(E_ERROR, "Couldn't compile code.  And I'm not smart enough to tell you why, sorry.");

  ret = zend_list_insert(action, le_swfactionp);

  object_init_ex(getThis(), &action_class_entry);
  add_property_resource(getThis(), "action", ret);
  zend_list_addref(ret);
}
/* no destructor for SWFAction, it's not a character */

/* }}} */
/* {{{ internal function getAction
   Returns the SWFAction object contained in zval *id */

static inline SWFAction getAction(zval *id)
{
  void *action = SWFgetProperty(id, "action", 6, le_swfactionp);

  if(!action)
    php_error(E_ERROR, "called object is not an SWFAction!");

  return (SWFAction)action;
}

/* }}} */

/* }}} */
/* {{{ SWFBitmap */

static zend_function_entry swfbitmap_functions[] = {
  PHP_FALIAS(swfbitmap,           swfbitmap_init,                NULL)
  PHP_FALIAS(getwidth,            swfbitmap_getWidth,            NULL)
  PHP_FALIAS(getheight,           swfbitmap_getHeight,           NULL)
  { NULL, NULL, NULL }
};

/* {{{ proto class swfbitmap_init(file, [maskfile])
   Returns a new SWFBitmap object from jpg (with optional mask) or dbl file */

PHP_FUNCTION(swfbitmap_init)
{
  zval **file, **mask;
  char *filename, *maskname = NULL;
  SWFBitmap bitmap;
  int ret, l;

  if(ZEND_NUM_ARGS() == 1)
  {
    if(zend_get_parameters_ex(1, &file) == FAILURE)
      WRONG_PARAM_COUNT;
  }
  else if(ZEND_NUM_ARGS() == 2)
  {
    if(zend_get_parameters_ex(2, &file, &mask) == FAILURE)
      WRONG_PARAM_COUNT;

    maskname = Z_STRVAL_PP(mask);
  }
  else
    WRONG_PARAM_COUNT;

  filename = Z_STRVAL_PP(file);
  l = Z_STRLEN_PP(file);

  if(strncasecmp(filename+l-4, ".jpg", 4) == 0 ||
     strncasecmp(filename+l-5, ".jpeg", 5) == 0)
  {
    if(maskname != NULL)
    {
      FILE *jpeg, *mask;
      if((jpeg = VCWD_FOPEN(filename, "rb")) == NULL)
	 php_error(E_ERROR, "Couldn't find file %s", filename);

      if((mask = VCWD_FOPEN(maskname, "rb")) == NULL)
	php_error(E_ERROR, "Couldn't find file %s", maskname);

      bitmap = newSWFJpegWithAlpha(jpeg, mask);

      ZEND_REGISTER_RESOURCE(NULL, jpeg, le_fopen);
      ZEND_REGISTER_RESOURCE(NULL, mask, le_fopen);
    }
    else
    {
      FILE *jpeg;

      if((jpeg = VCWD_FOPEN(filename, "rb")) == NULL)
	 php_error(E_ERROR, "Couldn't find file %s", filename);

      bitmap = newSWFJpegBitmap(jpeg);

      ZEND_REGISTER_RESOURCE(NULL, jpeg, le_fopen);
    }
  }
  else if(strncasecmp(filename+l-4, ".dbl", 4) == 0)
  {
    FILE *dbl;

    if((dbl = VCWD_FOPEN(filename, "rb")) == NULL)
      php_error(E_ERROR, "Couldn't find file %s", filename);

    bitmap = newSWFDBLBitmap(dbl);

    ZEND_REGISTER_RESOURCE(NULL, dbl, le_fopen);
  }
  else
    php_error(E_ERROR, "Sorry, can't tell what type of file %s is", filename);

  ret = zend_list_insert(bitmap, le_swfbitmapp);
  object_init_ex(getThis(), &bitmap_class_entry);
  add_property_resource(getThis(), "bitmap", ret);
  zend_list_addref(ret);
}
static void destroy_SWFBitmap_resource(zend_rsrc_list_entry *resource)
{
  destroySWFBitmap((SWFBitmap)resource->ptr);
}

/* }}} */
/* {{{ internal function getBitmap
   Returns the SWFBitmap object contained in zval *id */

static inline SWFBitmap getBitmap(zval *id)
{
  void *bitmap = SWFgetProperty(id, "bitmap", 6, le_swfbitmapp);

  if(!bitmap)
    php_error(E_ERROR, "called object is not an SWFBitmap!");

  return (SWFBitmap)bitmap;
}

/* }}} */
/* {{{ proto void swfbitmap_getWidth()
   Returns the width of this bitmap */

PHP_FUNCTION(swfbitmap_getWidth)
{
  if(ZEND_NUM_ARGS() != 0)
    WRONG_PARAM_COUNT;

  RETURN_DOUBLE(SWFBitmap_getWidth(getBitmap(getThis())));
}

/* }}} */
/* {{{ proto void swfbitmap_getHeight()
   Returns the height of this bitmap */

PHP_FUNCTION(swfbitmap_getHeight)
{
  if(ZEND_NUM_ARGS() != 0)
    WRONG_PARAM_COUNT;

  RETURN_DOUBLE(SWFBitmap_getHeight(getBitmap(getThis())));
}

/* }}} */

/* }}} */
/* {{{ SWFButton */

static zend_function_entry swfbutton_functions[] = {
  PHP_FALIAS(swfbutton,              swfbutton_init,            NULL)
  PHP_FALIAS(sethit,                 swfbutton_setHit,          NULL)
  PHP_FALIAS(setover,                swfbutton_setOver,         NULL)
  PHP_FALIAS(setup,                  swfbutton_setUp,           NULL)
  PHP_FALIAS(setdown,                swfbutton_setDown,         NULL)
  PHP_FALIAS(setaction,              swfbutton_setAction,       NULL)
  PHP_FALIAS(addshape,               swfbutton_addShape,        NULL)
  PHP_FALIAS(addaction,              swfbutton_addAction,       NULL)
  { NULL, NULL, NULL }
};

/* {{{ proto object swfbutton_init(void)
   returns a new SWFButton object */

PHP_FUNCTION(swfbutton_init)
{
  SWFButton button = newSWFButton();
  int ret = zend_list_insert(button, le_swfbuttonp);

  object_init_ex(getThis(), &button_class_entry);
  add_property_resource(getThis(), "button", ret);
  zend_list_addref(ret);
}
static void destroy_SWFButton_resource(zend_rsrc_list_entry *resource)
{
  destroySWFButton((SWFButton)resource->ptr);
}

/* }}} */
/* {{{ internal function getButton
   Returns the SWFButton object contained in zval *id */

static inline SWFButton getButton(zval *id)
{
  void *button = SWFgetProperty(id, "button", 6, le_swfbuttonp);

  if(!button)
    php_error(E_ERROR, "called object is not an SWFButton!");

  return (SWFButton)button;
}

/* }}} */
/* {{{ proto void swfbutton_setHit(SWFCharacter)
   sets the character for this button's hit test state */

PHP_FUNCTION(swfbutton_setHit)
{
  zval **zchar;
  SWFButton button = getButton(getThis());
  SWFCharacter character;

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &zchar) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_object_ex(zchar);
  character = getCharacter(*zchar);

  SWFButton_addShape(button, character, SWFBUTTONRECORD_HITSTATE);
}

/* }}} */
/* {{{ proto void swfbutton_setOver(SWFCharacter)
   sets the character for this button's over state */

PHP_FUNCTION(swfbutton_setOver)
{
  zval **zchar;
  SWFButton button = getButton(getThis());
  SWFCharacter character;

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &zchar) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_object_ex(zchar);
  character = getCharacter(*zchar);

  SWFButton_addShape(button, character, SWFBUTTONRECORD_OVERSTATE);
}

/* }}} */
/* {{{ proto void swfbutton_setUp(SWFCharacter)
   sets the character for this button's up state */

PHP_FUNCTION(swfbutton_setUp)
{
  zval **zchar;
  SWFButton button = getButton(getThis());
  SWFCharacter character;

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &zchar) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_object_ex(zchar);
  character = getCharacter(*zchar);

  SWFButton_addShape(button, character, SWFBUTTONRECORD_UPSTATE);
}

/* }}} */
/* {{{ proto void swfbutton_setDown(SWFCharacter)
   sets the character for this button's down state */

PHP_FUNCTION(swfbutton_setDown)
{
  zval **zchar;
  SWFButton button = getButton(getThis());
  SWFCharacter character;

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &zchar) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_object_ex(zchar);
  character = getCharacter(*zchar);

  SWFButton_addShape(button, character, SWFBUTTONRECORD_DOWNSTATE);
}

/* }}} */
/* {{{ proto void swfbutton_addShape(SWFCharacter character, int flags)
   sets the character to display for the condition described in flags */

PHP_FUNCTION(swfbutton_addShape)
{
  zval **zchar, **flags;
  SWFButton button = getButton(getThis());
  SWFCharacter character;

  if(ZEND_NUM_ARGS() != 2 ||
     zend_get_parameters_ex(2, &zchar, &flags) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_object_ex(zchar);
  character = getCharacter(*zchar);

  convert_to_long_ex(flags);

  SWFButton_addShape(button, character, Z_LVAL_PP(flags));
}

/* }}} */
/* {{{ proto void swfbutton_setAction(SWFAction)
   sets the action to perform when button is pressed */

PHP_FUNCTION(swfbutton_setAction)
{
  zval **zaction;
  SWFButton button = getButton(getThis());
  SWFAction action;

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &zaction) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_object_ex(zaction);
  action = getAction(*zaction);

  SWFButton_addAction(button, action, SWFBUTTON_OVERDOWNTOOVERUP);
}

/* }}} */
/* {{{ proto void swfbutton_addAction(SWFAction action, int flags)
   sets the action to perform when conditions described in flags is met */

PHP_FUNCTION(swfbutton_addAction)
{
  zval **zaction, **flags;
  SWFButton button = getButton(getThis());
  SWFAction action;

  if(ZEND_NUM_ARGS() != 2 ||
     zend_get_parameters_ex(2, &zaction, &flags) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_object_ex(zaction);
  action = getAction(*zaction);

  convert_to_long_ex(flags);

  SWFButton_addAction(button, action, Z_LVAL_PP(flags));
}

/* }}} */
/* {{{ proto int SWFBUTTON_KEYPRESS(char)
   returns the action flag for keyPress(char) */

PHP_FUNCTION(swfbutton_keypress)
{
  zval **key;
  char c;

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &key) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_string_ex(key);

  if(Z_STRLEN_PP(key) > 1)
    php_error(E_ERROR, "SWFBUTTON_KEYPRESS expects only one character!");

  c = Z_STRVAL_PP(key)[0];

  RETURN_LONG((c&0x7f)<<9);
}

/* }}} */
/* }}} */
/* {{{ SWFDisplayitem */

static zend_function_entry swfdisplayitem_functions[] = {
  PHP_FALIAS(moveto,       swfdisplayitem_moveTo,      NULL)
  PHP_FALIAS(move,         swfdisplayitem_move,        NULL)
  PHP_FALIAS(scaleto,      swfdisplayitem_scaleTo,     NULL)
  PHP_FALIAS(scale,        swfdisplayitem_scale,       NULL)
  PHP_FALIAS(rotateto,     swfdisplayitem_rotateTo,    NULL)
  PHP_FALIAS(rotate,       swfdisplayitem_rotate,      NULL)
  PHP_FALIAS(skewxto,      swfdisplayitem_skewXTo,     NULL)
  PHP_FALIAS(skewx,        swfdisplayitem_skewX,       NULL)
  PHP_FALIAS(skewyto,      swfdisplayitem_skewYTo,     NULL)
  PHP_FALIAS(skewy,        swfdisplayitem_skewY,       NULL)
  PHP_FALIAS(setmatrix,    swfdisplayitem_setMatrix,   NULL)
  PHP_FALIAS(setdepth,     swfdisplayitem_setDepth,    NULL)
  PHP_FALIAS(setratio,     swfdisplayitem_setRatio,    NULL)
  PHP_FALIAS(addcolor,     swfdisplayitem_addColor,    NULL)
  PHP_FALIAS(multcolor,    swfdisplayitem_multColor,   NULL)
  PHP_FALIAS(setname,      swfdisplayitem_setName,     NULL)
  { NULL, NULL, NULL }
};

/* {{{ internal function getDisplayItem
   Returns the SWFDisplayItem contained in zval *id */

static SWFDisplayItem getDisplayItem(zval *id)
{
  void *item = SWFgetProperty(id, "displayitem", 11, le_swfdisplayitemp);

  if(!item)
    php_error(E_ERROR, "called object is not an SWFDisplayItem!");

  return (SWFDisplayItem)item;
}

/* }}} */
/* {{{ proto void swfdisplayitem_moveTo(int x, int y)
   Moves this SWFDisplayItem to movie coordinates (x,y) */

PHP_FUNCTION(swfdisplayitem_moveTo)
{
  zval **x, **y;

  if(ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &x, &y) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(x);
  convert_to_double_ex(y);

  SWFDisplayItem_moveTo(getDisplayItem(getThis()), Z_DVAL_PP(x), Z_DVAL_PP(y));
}

/* }}} */
/* {{{ proto void swfdisplayitem_move(int dx, int dy)
   Displaces this SWFDisplayItem by (dx,dy) in movie coordinates. */

PHP_FUNCTION(swfdisplayitem_move)
{
  zval **x, **y;

  if(ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &x, &y) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(x);
  convert_to_double_ex(y);

  SWFDisplayItem_move(getDisplayItem(getThis()), Z_DVAL_PP(x), Z_DVAL_PP(y));
}

/* }}} */
/* {{{ proto void swfdisplayitem_scaleTo(float xScale, [float yScale])
   Scales this SWFDisplayItem by xScale in the x direction, yScale in the y,
   or both to xScale if only one arg. */

PHP_FUNCTION(swfdisplayitem_scaleTo)
{
  zval **x, **y;

  if(ZEND_NUM_ARGS() == 1)
  {
    if(zend_get_parameters_ex(1, &x) == FAILURE)
      WRONG_PARAM_COUNT;

    convert_to_double_ex(x);

    SWFDisplayItem_scaleTo(getDisplayItem(getThis()),
			   Z_DVAL_PP(x), Z_DVAL_PP(x));
  }
  else if(ZEND_NUM_ARGS() == 2)
  {
    if(zend_get_parameters_ex(2, &x, &y) == FAILURE)
      WRONG_PARAM_COUNT;

    convert_to_double_ex(x);
    convert_to_double_ex(y);

    SWFDisplayItem_scaleTo(getDisplayItem(getThis()),
			   Z_DVAL_PP(x), Z_DVAL_PP(y));
  }
  else
    WRONG_PARAM_COUNT;
}

/* }}} */
/* {{{ proto void swfdisplayitem_scale(float xScale, float yScale)
   Multiplies this SWFDisplayItem's current x scale by xScale,
   its y scale by yScale */

PHP_FUNCTION(swfdisplayitem_scale)
{
  zval **x, **y;

  if(ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &x, &y) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(x);
  convert_to_double_ex(y);

  SWFDisplayItem_scale(getDisplayItem(getThis()), Z_DVAL_PP(x), Z_DVAL_PP(y));
}

/* }}} */
/* {{{ proto void swfdisplayitem_rotateTo(float degrees)
   Rotates this SWFDisplayItem the given (clockwise) degrees from its original
   orientation */

PHP_FUNCTION(swfdisplayitem_rotateTo)
{
  zval **degrees;

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &degrees) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(degrees);

  SWFDisplayItem_rotateTo(getDisplayItem(getThis()), Z_DVAL_PP(degrees));
}

/* }}} */
/* {{{ proto void swfdisplayitem_rotate(float degrees)
   Rotates this SWFDisplayItem the given (clockwise) degrees from its current
   orientation */

PHP_FUNCTION(swfdisplayitem_rotate)
{
  zval **degrees;

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &degrees) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(degrees);

  SWFDisplayItem_rotate(getDisplayItem(getThis()), Z_DVAL_PP(degrees));
}

/* }}} */
/* {{{ proto void swfdisplayitem_skewXTo(float xSkew)
   Sets this SWFDisplayItem's x skew value to xSkew */

PHP_FUNCTION(swfdisplayitem_skewXTo)
{
  zval **x;

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &x) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(x);

  SWFDisplayItem_skewXTo(getDisplayItem(getThis()), Z_DVAL_PP(x));
}

/* }}} */
/* {{{ proto void swfdisplayitem_skewX(float xSkew)
   Adds xSkew to this SWFDisplayItem's x skew value */

PHP_FUNCTION(swfdisplayitem_skewX)
{
  zval **x;

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &x) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(x);

  SWFDisplayItem_skewX(getDisplayItem(getThis()), Z_DVAL_PP(x));
}

/* }}} */
/* {{{ proto void swfdisplayitem_skewYTo(float ySkew)
   Sets this SWFDisplayItem's y skew value to ySkew */

PHP_FUNCTION(swfdisplayitem_skewYTo)
{
  zval **y;

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &y) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(y);

  SWFDisplayItem_skewYTo(getDisplayItem(getThis()), Z_DVAL_PP(y));
}

/* }}} */
/* {{{ proto void swfdisplayitem_skewY(float ySkew)
   Adds ySkew to this SWFDisplayItem's y skew value */

PHP_FUNCTION(swfdisplayitem_skewY)
{
  zval **y;

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &y) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(y);

  SWFDisplayItem_skewY(getDisplayItem(getThis()), Z_DVAL_PP(y));
}

/* }}} */
/* {{{ proto void swfdisplayitem_setMatrix(float a, float b, float c, float d, float x, float y)
   Sets the item's transform matrix */

PHP_FUNCTION(swfdisplayitem_setMatrix)
{
  zval **a, **b, **c, **d, **x, **y;

  if(ZEND_NUM_ARGS() != 6 ||
     zend_get_parameters_ex(6, &a, &b, &c, &d, &x, &y) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(a);
  convert_to_double_ex(b);
  convert_to_double_ex(c);
  convert_to_double_ex(d);
  convert_to_double_ex(x);
  convert_to_double_ex(y);

  SWFDisplayItem_setMatrix(getDisplayItem(getThis()),
			   Z_DVAL_PP(a),  Z_DVAL_PP(b), Z_DVAL_PP(c),
			   Z_DVAL_PP(d), Z_DVAL_PP(x), Z_DVAL_PP(y));
}

/* }}} */
/* {{{ proto void swfdisplayitem_setDepth(int depth)
   Sets this SWFDisplayItem's z-depth to depth.  Items with higher depth
   values are drawn on top of those with lower values */

PHP_FUNCTION(swfdisplayitem_setDepth)
{
  zval **depth;

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &depth) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_long_ex(depth);

  SWFDisplayItem_setDepth(getDisplayItem(getThis()), Z_LVAL_PP(depth));
}

/* }}} */
/* {{{ proto void swfdisplayitem_setRatio(float ratio)
   Sets this SWFDisplayItem's ratio to ratio.  Obviously only does anything
   if displayitem was created from an SWFMorph */

PHP_FUNCTION(swfdisplayitem_setRatio)
{
  zval **ratio;

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &ratio) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(ratio);

  SWFDisplayItem_setRatio(getDisplayItem(getThis()), Z_DVAL_PP(ratio));
}

/* }}} */
/* {{{ proto void swfdisplayitem_addColor(int r, int g, int b [, int a])
   Sets the add color part of this SWFDisplayItem's CXform to (r,g,b,[a]).
   a defaults to 0 */

PHP_FUNCTION(swfdisplayitem_addColor)
{
  zval **r, **g, **b, **za;
  int a = 0;

  if(ZEND_NUM_ARGS() == 4)
  {
    if(zend_get_parameters_ex(4, &r, &g, &b, &za) == FAILURE)
      WRONG_PARAM_COUNT;

    convert_to_long_ex(za);
    a = Z_LVAL_PP(za);
  }
  else if(ZEND_NUM_ARGS() == 3)
  {
    if(zend_get_parameters_ex(3, &r, &g, &b) == FAILURE)
      WRONG_PARAM_COUNT;
  }
  else
    WRONG_PARAM_COUNT;

  convert_to_long_ex(r);
  convert_to_long_ex(g);
  convert_to_long_ex(b);

  SWFDisplayItem_setColorAdd(getDisplayItem(getThis()),
			     Z_LVAL_PP(r), Z_LVAL_PP(g), Z_LVAL_PP(b), a);
}

/* }}} */
/* {{{ proto void swfdisplayitem_multColor(float r, float g, float b, [float a])
   Sets the multiply color part of this SWFDisplayItem's CXform to (r,g,b,[a]).
   a defaults to 1.0 */

PHP_FUNCTION(swfdisplayitem_multColor)
{
  zval **r, **g, **b, **za;
  float a = 1.0;

  if(ZEND_NUM_ARGS() == 4)
  {
    if(zend_get_parameters_ex(4, &r, &g, &b, &za) == FAILURE)
      WRONG_PARAM_COUNT;

    convert_to_double_ex(za);
    a = Z_DVAL_PP(za);
  }
  else if(ZEND_NUM_ARGS() == 3)
  {
    if(zend_get_parameters_ex(3, &r, &g, &b) == FAILURE)
      WRONG_PARAM_COUNT;
  }
  else
    WRONG_PARAM_COUNT;

  convert_to_double_ex(r);
  convert_to_double_ex(g);
  convert_to_double_ex(b);

  SWFDisplayItem_setColorMult(getDisplayItem(getThis()),
			      Z_DVAL_PP(r), Z_DVAL_PP(g), Z_DVAL_PP(b), a);
}

/* }}} */
/* {{{ proto void swfdisplayitem_setName(string name)
   Sets this SWFDisplayItem's name to name. */

PHP_FUNCTION(swfdisplayitem_setName)
{
  zval **name;
  SWFDisplayItem item = getDisplayItem(getThis());

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &name) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_string_ex(name);

  SWFDisplayItem_setName(item, Z_STRVAL_PP(name));
}

/* }}} */

/* }}} */
/* {{{ SWFFill */

static zend_function_entry swffill_functions[] = {
  PHP_FALIAS(swffill,      swffill_init,        NULL)
  PHP_FALIAS(moveto,       swffill_moveTo,      NULL)
  PHP_FALIAS(scaleto,      swffill_scaleTo,     NULL)
  PHP_FALIAS(rotateto,     swffill_rotateTo,    NULL)
  PHP_FALIAS(skewxto,      swffill_skewXTo,     NULL)
  PHP_FALIAS(skewyto,      swffill_skewYTo,     NULL)
  { NULL, NULL, NULL }
};

/* {{{ proto class swffill_init(void)
   Returns a new SWFFill object */

PHP_FUNCTION(swffill_init)
{
  php_error(E_ERROR, "Instantiating SWFFill won't do any good- use SWFShape::addFill() instead!");
}
static void destroy_SWFFill_resource(zend_rsrc_list_entry *resource)
{
  /* this only destroys the shallow wrapper for SWFFillStyle,
     which SWFShape destroys.  So everything's okay.  I hope. */

  destroySWFFill((SWFFill)resource->ptr);
}

/* }}} */
/* {{{ internal function getFill
   Returns the SWFFill object contained in zval *id */

static inline SWFFill getFill(zval *id)
{
  void *fill = SWFgetProperty(id, "fill", 4, le_swffillp);

  if(!fill)
    php_error(E_ERROR, "called object is not an SWFFill!");

  return (SWFFill)fill;
}

/* }}} */

/* {{{ proto void swffill_moveTo(int x, int y)
   Moves this SWFFill to shape coordinates (x,y) */

PHP_FUNCTION(swffill_moveTo)
{
  zval **x, **y;

  if(ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &x, &y) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(x);
  convert_to_double_ex(y);

  SWFFill_moveTo(getFill(getThis()), Z_DVAL_PP(x), Z_DVAL_PP(y));
}

/* }}} */
/* {{{ proto void swffill_scaleTo(float xScale, [float yScale])
   Scales this SWFFill by xScale in the x direction, yScale in the y,
   or both to xScale if only one arg. */

PHP_FUNCTION(swffill_scaleTo)
{
  zval **x, **y;

  if(ZEND_NUM_ARGS() == 1)
  {
    if(zend_get_parameters_ex(1, &x) == FAILURE)
      WRONG_PARAM_COUNT;

    convert_to_double_ex(x);

    SWFFill_scaleXYTo(getFill(getThis()), Z_DVAL_PP(x), Z_DVAL_PP(x));
  }
  else if(ZEND_NUM_ARGS() == 2)
  {
    if(zend_get_parameters_ex(2, &x, &y) == FAILURE)
      WRONG_PARAM_COUNT;

    convert_to_double_ex(x);
    convert_to_double_ex(y);

    SWFFill_scaleXYTo(getFill(getThis()), Z_DVAL_PP(x), Z_DVAL_PP(y));
  }
  else
    WRONG_PARAM_COUNT;
}

/* }}} */
/* {{{ proto void swffill_rotateTo(float degrees)
   Rotates this SWFFill the given (clockwise) degrees from its original
   orientation */

PHP_FUNCTION(swffill_rotateTo)
{
  zval **degrees;

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &degrees) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(degrees);

  SWFFill_rotateTo(getFill(getThis()), Z_DVAL_PP(degrees));
}

/* }}} */
/* {{{ proto void swffill_skewXTo(float xSkew)
   Sets this SWFFill's x skew value to xSkew */

PHP_FUNCTION(swffill_skewXTo)
{
  zval **x;

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &x) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(x);

  SWFFill_skewXTo(getFill(getThis()), Z_DVAL_PP(x));
}

/* }}} */
/* {{{ proto void swffill_skewYTo(float ySkew)
   Sets this SWFFill's y skew value to ySkew */

PHP_FUNCTION(swffill_skewYTo)
{
  zval **y;

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &y) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(y);

  SWFFill_skewYTo(getFill(getThis()), Z_DVAL_PP(y));
}

/* }}} */

/* }}} */
/* {{{ SWFFont */

static zend_function_entry swffont_functions[] = {
  PHP_FALIAS(swffont,          swffont_init,              NULL)
  PHP_FALIAS(getwidth,         swffont_getWidth,          NULL)
  PHP_FALIAS(getascent,        swffont_getAscent,         NULL)
  PHP_FALIAS(getdescent,       swffont_getDescent,        NULL)
  PHP_FALIAS(getleading,       swffont_getLeading,        NULL)
  { NULL, NULL, NULL }
};

/* {{{ internal function SWFText getFont(zval *id)
   Returns the Font object in zval *id */

SWFFont getFont(zval *id)
{
  void *font = SWFgetProperty(id, "font", 4, le_swffontp);

  if(!font)
    php_error(E_ERROR, "called object is not an SWFFont!");

  return (SWFFont)font;
}

/* }}} */
/* {{{ proto class swffont_init(string filename)
   Returns a new SWFFont object from given file */

PHP_FUNCTION(swffont_init)
{
  FILE *file;
  zval **zfile;
  SWFFont font;
  int ret;

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &zfile) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_string_ex(zfile);

  if(strcmp(Z_STRVAL_PP(zfile)+Z_STRLEN_PP(zfile)-4, ".fdb") == 0)
  {
    file = VCWD_FOPEN(Z_STRVAL_PP(zfile), "rb");

    if(!file)
      php_error(E_ERROR, "Couldn't find FDB file %s", Z_STRVAL_PP(zfile));

    font = loadSWFFontFromFile(file);
    fclose(file);
  }
  else
    font = newSWFBrowserFont(Z_STRVAL_PP(zfile));

  ret = zend_list_insert(font, le_swffontp);

  object_init_ex(getThis(), &font_class_entry);
  add_property_resource(getThis(), "font", ret);
  zend_list_addref(ret);
}
static void destroy_SWFFont_resource(zend_rsrc_list_entry *resource)
{
  destroySWFBlock((SWFBlock)resource->ptr);
}

/* }}} */
/* {{{ proto int swffont_getWidth(string)
   calculates the width of the given string in this font at full height */

PHP_FUNCTION(swffont_getWidth)
{
  zval **zstring;
  float width;

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &zstring) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_string_ex(zstring);

  width = SWFFont_getStringWidth(getFont(getThis()), Z_STRVAL_PP(zstring));

  RETURN_DOUBLE(width);
}

/* }}} */
/* {{{ proto int swffont_getAscent()
   returns the ascent of the font, or 0 if not available */

PHP_FUNCTION(swffont_getAscent)
{
  if(ZEND_NUM_ARGS() != 0)
    WRONG_PARAM_COUNT;

  RETURN_DOUBLE(SWFFont_getAscent(getFont(getThis())));
}

/* }}} */
/* {{{ proto int swffont_getDescent()
   returns the descent of the font, or 0 if not available */

PHP_FUNCTION(swffont_getDescent)
{
  if(ZEND_NUM_ARGS() != 0)
    WRONG_PARAM_COUNT;

  RETURN_DOUBLE(SWFFont_getDescent(getFont(getThis())));
}

/* }}} */
/* {{{ proto int swffont_getLeading()
   returns the leading of the font, or 0 if not available */

PHP_FUNCTION(swffont_getLeading)
{
  if(ZEND_NUM_ARGS() != 0)
    WRONG_PARAM_COUNT;

  RETURN_DOUBLE(SWFFont_getLeading(getFont(getThis())));
}

/* }}} */

/* }}} */
/* {{{ SWFGradient */

static zend_function_entry swfgradient_functions[] = {
  PHP_FALIAS(swfgradient,  swfgradient_init,      NULL)
  PHP_FALIAS(addentry,     swfgradient_addEntry,  NULL)
  { NULL, NULL, NULL }
};

/* {{{ proto class swfgradient_init(void)
   Returns a new SWFGradient object */

PHP_FUNCTION(swfgradient_init)
{
  SWFGradient gradient = newSWFGradient();
  int ret = zend_list_insert(gradient, le_swfgradientp);

  object_init_ex(getThis(), &gradient_class_entry);
  add_property_resource(getThis(), "gradient", ret);
  zend_list_addref(ret);
}
static void destroy_SWFGradient_resource(zend_rsrc_list_entry *resource)
{
  destroySWFGradient((SWFGradient)resource->ptr);
}

/* }}} */
/* {{{ internal function getGradient
   Returns the SWFGradient object contained in zval *id */

static inline SWFGradient getGradient(zval *id)
{
  void *gradient = SWFgetProperty(id, "gradient", 8, le_swfgradientp);

  if(!gradient)
    php_error(E_ERROR, "called object is not an SWFGradient!");

  return (SWFGradient)gradient;
}

/* }}} */
/* {{{ proto void swfgradient_addEntry(float ratio, byte r, byte g, byte b [, byte a]
   add given entry to the gradient */

PHP_FUNCTION(swfgradient_addEntry)
{
  zval **ratio, **r, **g, **b, **za;
  byte a = 0xff;

  if(ZEND_NUM_ARGS() == 4)
  {
    if(zend_get_parameters_ex(4, &ratio, &r, &g, &b) == FAILURE)
      WRONG_PARAM_COUNT;
  }
  else if(ZEND_NUM_ARGS() == 5)
  {
    zval **za;

    if(zend_get_parameters_ex(5, &ratio, &r, &g, &b, &za) == FAILURE)
      WRONG_PARAM_COUNT;

    convert_to_long_ex(za);
    a = Z_LVAL_PP(za);
  }
  else
    WRONG_PARAM_COUNT;

  convert_to_double_ex(ratio);
  convert_to_long_ex(r);
  convert_to_long_ex(g);
  convert_to_long_ex(b);

  SWFGradient_addEntry(getGradient(getThis()), Z_DVAL_PP(ratio),
		       Z_LVAL_PP(r), Z_LVAL_PP(g), Z_LVAL_PP(b), a);
}

/* }}} */

/* }}} */
/* {{{ SWFMorph */

static zend_function_entry swfmorph_functions[] = {
  PHP_FALIAS(swfmorph,              swfmorph_init,            NULL)
  PHP_FALIAS(getshape1,             swfmorph_getShape1,       NULL)
  PHP_FALIAS(getshape2,             swfmorph_getShape2,       NULL)
  { NULL, NULL, NULL }
};

/* {{{ proto object swfmorph_init()
   returns a new SWFMorph object */

PHP_FUNCTION(swfmorph_init)
{
  SWFMorph morph = newSWFMorphShape();
  int ret = zend_list_insert(morph, le_swfmorphp);

  object_init_ex(getThis(), &morph_class_entry);
  add_property_resource(getThis(), "morph", ret);
  zend_list_addref(ret);
}
static void destroy_SWFMorph_resource(zend_rsrc_list_entry *resource)
{
  destroySWFMorph((SWFMorph)resource->ptr);
}

/* }}} */
/* {{{ internal function getMorph
   Returns the SWFMorph object contained in zval *id */

static inline SWFMorph getMorph(zval *id)
{
  void *morph = SWFgetProperty(id, "morph", 5, le_swfmorphp);

  if(!morph)
    php_error(E_ERROR, "called object is not an SWFMorph!");

  return (SWFMorph)morph;
}

/* }}} */
/* {{{ proto SWFShape swfmorph_getShape1()
   return's this SWFMorph's start shape */

PHP_FUNCTION(swfmorph_getShape1)
{
  SWFMorph morph = getMorph(getThis());
  SWFShape shape = SWFMorph_getShape1(morph);
  int ret = zend_list_insert(shape, le_swfshapep);

  object_init_ex(return_value, &shape_class_entry);
  add_property_resource(return_value, "shape", ret);
  zend_list_addref(ret);
}

/* }}} */
/* {{{ proto SWFShape swfmorph_getShape2()
   return's this SWFMorph's start shape */

PHP_FUNCTION(swfmorph_getShape2)
{
  SWFMorph morph = getMorph(getThis());
  SWFShape shape = SWFMorph_getShape2(morph);
  int ret = zend_list_insert(shape, le_swfshapep);

  object_init_ex(return_value, &shape_class_entry);
  add_property_resource(return_value, "shape", ret);
  zend_list_addref(ret);
}

/* }}} */

/* }}} */
/* {{{ SWFMovie */

static zend_function_entry swfmovie_functions[] = {
  PHP_FALIAS(swfmovie,          swfmovie_init,              NULL)
  PHP_FALIAS(nextframe,         swfmovie_nextFrame,         NULL)
  PHP_FALIAS(labelframe,        swfmovie_labelFrame,        NULL)
  PHP_FALIAS(add,               swfmovie_add,               NULL)
  PHP_FALIAS(remove,            swfmovie_remove,            NULL)
  PHP_FALIAS(output,            swfmovie_output,            NULL)
  PHP_FALIAS(save,              swfmovie_save,              NULL)
  PHP_FALIAS(savetofile,        swfmovie_saveToFile,        NULL)
  PHP_FALIAS(setbackground,     swfmovie_setBackground,     NULL)
  PHP_FALIAS(setrate,           swfmovie_setRate,           NULL)
  PHP_FALIAS(setdimension,      swfmovie_setDimension,      NULL)
  PHP_FALIAS(setframes,         swfmovie_setFrames,         NULL)
  PHP_FALIAS(streammp3,         swfmovie_streamMp3,         NULL)
  { NULL, NULL, NULL }
};

/* {{{ swfmovie_init */

PHP_FUNCTION(swfmovie_init)
{
  zval **version;
  SWFMovie movie;
  int ret;

  if(ZEND_NUM_ARGS() == 1)
  {
    if(zend_get_parameters_ex(1, &version) == FAILURE)
      WRONG_PARAM_COUNT;

    convert_to_long_ex(version);

    movie = newSWFMovie(Z_LVAL_PP(version));
  }
  else
    movie = newSWFMovie(4); /* default version 4 */

  ret = zend_list_insert(movie, le_swfmoviep);

  object_init_ex(getThis(), &movie_class_entry);
  add_property_resource(getThis(), "movie", ret);
  zend_list_addref(ret);
}
static void destroy_SWFMovie_resource(zend_rsrc_list_entry *resource)
{
  destroySWFMovie((SWFMovie)resource->ptr);
}

/* }}} */
/* {{{ getMovie */

SWFMovie getMovie(zval *id)
{
  void *movie = SWFgetProperty(id, "movie", 5, le_swfmoviep);

  if(!movie)
    php_error(E_ERROR, "called object is not an SWFMovie!");

  return (SWFMovie)movie;
}

/* }}} */
/* {{{ swfmovie_nextframe */

PHP_FUNCTION(swfmovie_nextFrame)
{
  SWFMovie_nextFrame(getMovie(getThis()));
}

/* }}} */
/* {{{ swfmovie_labelframe */

PHP_FUNCTION(swfmovie_labelFrame)
{
  zval **label;

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &label) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_string_ex(label);

  SWFMovie_labelFrame(getMovie(getThis()), Z_STRVAL_PP(label));
}

/* }}} */
/* {{{ swfmovie_add */

PHP_FUNCTION(swfmovie_add)
{
  zval **zchar;
  int ret;
  SWFBlock block;
  SWFDisplayItem item;
  SWFMovie movie = getMovie(getThis());

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &zchar) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_object_ex(zchar);

  /* XXX - SWFMovie_add deals w/ all block types.  Probably will need to add that.. */
  if((*zchar)->value.obj.ce == &action_class_entry)
    block = (SWFBlock)getAction(*zchar);
  else
    block = (SWFBlock)getCharacter(*zchar);

  item = SWFMovie_add(movie, block);

  if(item != NULL)
  {
    /* try and create a displayitem object */
    ret = zend_list_insert(item, le_swfdisplayitemp);
    object_init_ex(return_value, &displayitem_class_entry);
    add_property_resource(return_value, "displayitem", ret);
  }
}

/* }}} */
/* {{{ swfmovie_remove */

PHP_FUNCTION(swfmovie_remove)
{
  zval **zchar;
  SWFDisplayItem item;
  SWFMovie movie = getMovie(getThis());

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &zchar) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_object_ex(zchar);
  item = getDisplayItem(*zchar);

  SWFMovie_remove(movie, item);
}


/* }}} */
/* {{{ swfmovie_output */

void phpByteOutputMethod(byte b, void *data)
{
  php_write(&b, 1);
}

PHP_FUNCTION(swfmovie_output)
{
  SWFMovie movie = getMovie(getThis());

  RETURN_LONG(SWFMovie_output(movie, &phpByteOutputMethod, NULL));
}


/* }}} */
/* {{{ swfmovie_saveToFile */

void phpFileOutputMethod(byte b, void *data)
{
  fwrite(&b, 1, 1, (FILE *)data);
}

PHP_FUNCTION(swfmovie_saveToFile)
{
  zval **x;
  SWFMovie movie = getMovie(getThis());
  int type;
  int le_fopen;
  void *what;

  if((ZEND_NUM_ARGS() != 1) || zend_get_parameters_ex(1, &x) == FAILURE)
    WRONG_PARAM_COUNT;

  ZEND_FETCH_RESOURCE(what, FILE *, x, -1,"File-Handle",php_file_le_fopen());
  RETURN_LONG(SWFMovie_output(movie, &phpFileOutputMethod, what));
}


/* }}} */
/* {{{ swfmovie_save */

PHP_FUNCTION(swfmovie_save)
{
  zval **x;
  FILE *file;
  long retval;

  if((ZEND_NUM_ARGS() != 1) || zend_get_parameters_ex(1, &x) == FAILURE)
    WRONG_PARAM_COUNT;

  if((*x)->type == IS_RESOURCE)
  {
    ZEND_FETCH_RESOURCE(file, FILE *, x, -1,"File-Handle",php_file_le_fopen());

    RETURN_LONG(SWFMovie_output(getMovie(getThis()),
				&phpFileOutputMethod, file));
  }

  convert_to_string_ex(x);

  file = VCWD_FOPEN(Z_STRVAL_PP(x), "wb");

  if(file == NULL)
    php_error(E_ERROR, "couldn't open file %s for writing", Z_STRVAL_PP(x));

  retval = SWFMovie_output(getMovie(getThis()),
			      &phpFileOutputMethod, (void *)file);

  fclose(file);

  RETURN_LONG(retval);
}


/* }}} */
/* {{{ swfmovie_setbackground */

PHP_FUNCTION(swfmovie_setBackground)
{
  zval **r, **g, **b;
  SWFMovie movie = getMovie(getThis());

  if((ZEND_NUM_ARGS() != 3) || zend_get_parameters_ex(3, &r, &g, &b) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_long_ex(r);
  convert_to_long_ex(g);
  convert_to_long_ex(b);

  SWFMovie_setBackground(movie, Z_LVAL_PP(r), Z_LVAL_PP(g), Z_LVAL_PP(b));
}

/* }}} */
/* {{{ swfmovie_setrate */

PHP_FUNCTION(swfmovie_setRate)
{
  zval **rate;
  SWFMovie movie = getMovie(getThis());

  if((ZEND_NUM_ARGS() != 1) || zend_get_parameters_ex(1, &rate) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(rate);

  SWFMovie_setRate(movie, Z_DVAL_PP(rate));
}

/* }}} */
/* {{{ swfmovie_setDimension */

PHP_FUNCTION(swfmovie_setDimension)
{
  zval **x, **y;
  SWFMovie movie = getMovie(getThis());

  if((ZEND_NUM_ARGS() != 2) || zend_get_parameters_ex(2, &x, &y) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(x);
  convert_to_double_ex(y);

  SWFMovie_setDimension(movie, Z_DVAL_PP(x), Z_DVAL_PP(y));
}

/* }}} */
/* {{{ swfmovie_setframes */

PHP_FUNCTION(swfmovie_setFrames)
{
  zval **frames;
  SWFMovie movie = getMovie(getThis());

  if((ZEND_NUM_ARGS() != 1) || zend_get_parameters_ex(1, &frames) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_long_ex(frames);

  SWFMovie_setNumberOfFrames(movie, Z_LVAL_PP(frames));
}

/* }}} */
/* {{{ swfmovie_streamMp3 */

PHP_FUNCTION(swfmovie_streamMp3)
{
  FILE *file;
  zval **zfile;
  SWFSound sound;
  SWFMovie movie = getMovie(getThis());

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &zfile) == FAILURE)
    WRONG_PARAM_COUNT;

  if((*zfile)->type == IS_RESOURCE)
  {
    ZEND_FETCH_RESOURCE(file, FILE *, zfile, -1,"File-Handle",php_file_le_fopen());
  }
  else
  {
    convert_to_string_ex(zfile);

    file = VCWD_FOPEN(Z_STRVAL_PP(zfile), "rb");

    if(!file)
      php_error(E_ERROR, "Couldn't find file %s", Z_STRVAL_PP(zfile));

    ZEND_REGISTER_RESOURCE(NULL, file, le_fopen);
  }

  sound = newSWFSound(file);
  SWFMovie_setSoundStream(movie, sound);
}

/* }}} */

/* }}} */
/* {{{ SWFShape */

static zend_function_entry swfshape_functions[] = {
  PHP_FALIAS(swfshape,          swfshape_init,               NULL)
  PHP_FALIAS(setline,           swfshape_setline,            NULL)
  PHP_FALIAS(addfill,           swfshape_addfill,            NULL)
  PHP_FALIAS(setleftfill,       swfshape_setleftfill,        NULL)
  PHP_FALIAS(setrightfill,      swfshape_setrightfill,       NULL)
  PHP_FALIAS(movepento,         swfshape_movepento,          NULL)
  PHP_FALIAS(movepen,           swfshape_movepen,            NULL)
  PHP_FALIAS(drawlineto,        swfshape_drawlineto,         NULL)
  PHP_FALIAS(drawline,          swfshape_drawline,           NULL)
  PHP_FALIAS(drawcurveto,       swfshape_drawcurveto,        NULL)
  PHP_FALIAS(drawcurve,         swfshape_drawcurve,          NULL)
  PHP_FALIAS(drawglyph,         swfshape_drawglyph,          NULL)
  PHP_FALIAS(drawcircle,        swfshape_drawcircle,         NULL)
  PHP_FALIAS(drawarc,           swfshape_drawarc,            NULL)
  PHP_FALIAS(drawcubic,         swfshape_drawcubic,          NULL)
  PHP_FALIAS(drawcubicto,       swfshape_drawcubicto,        NULL)
  { NULL, NULL, NULL }
};

/* {{{ proto class swfshape_init(void)
   Returns a new SWFShape object */

PHP_FUNCTION(swfshape_init)
{
  SWFShape shape = newSWFShape();
  int ret = zend_list_insert(shape, le_swfshapep);

  object_init_ex(getThis(), &shape_class_entry);
  add_property_resource(getThis(), "shape", ret);
  zend_list_addref(ret);
}
static void destroy_SWFShape_resource(zend_rsrc_list_entry *resource)
{
  destroySWFShape((SWFShape)resource->ptr);
}

/* }}} */
/* {{{ internal function getShape
   Returns the SWFShape object contained in zval *id */

static inline SWFShape getShape(zval *id)
{
  void *shape = SWFgetProperty(id, "shape", 5, le_swfshapep);

  if(!shape)
    php_error(E_ERROR, "called object is not an SWFShape!");

  return (SWFShape)shape;
}

/* }}} */
/* {{{ proto void swfshape_setline(int width, int r, int g, int b [, int a])
   Sets the current line style for this SWFShape */

PHP_FUNCTION(swfshape_setline)
{
  zval **w, **r, **g, **b, **a;

  if(ZEND_NUM_ARGS() == 4)
  {
    if(zend_get_parameters_ex(4, &w, &r, &g, &b) == FAILURE)
      WRONG_PARAM_COUNT;
  }
  else if(ZEND_NUM_ARGS() == 5)
  {
    if(zend_get_parameters_ex(5, &w, &r, &g, &b, &a) == FAILURE)
      WRONG_PARAM_COUNT;

    convert_to_long_ex(a);
  }
  else if(ZEND_NUM_ARGS() == 1)
  {
    SWFShape_setLine(getShape(getThis()), 0, 0, 0, 0, 0);
    return;
  }
  else
    WRONG_PARAM_COUNT;

  convert_to_long_ex(w);
  convert_to_long_ex(r);
  convert_to_long_ex(g);
  convert_to_long_ex(b);

  if(ZEND_NUM_ARGS() == 4)
  {
    SWFShape_setLine(getShape(getThis()),
		     Z_LVAL_PP(w), Z_LVAL_PP(r), Z_LVAL_PP(g),
		     Z_LVAL_PP(b), 0xff);
  }
  else
  {
    SWFShape_setLine(getShape(getThis()),
		     Z_LVAL_PP(w), Z_LVAL_PP(r), Z_LVAL_PP(g),
		     Z_LVAL_PP(b), Z_LVAL_PP(a));
  }
}

/* }}} */
/* {{{ proto int swfshape_addfill(fill, flags)
   Returns a fill object, for use with swfshape_setleftfill and
   swfshape_setrightfill */

PHP_FUNCTION(swfshape_addfill)
{
  SWFFill fill;
  int ret;

  if(ZEND_NUM_ARGS() == 1 || ZEND_NUM_ARGS() == 2)
  {
    /* it's a gradient or bitmap */

    zval **arg1;
    unsigned char flags = 0;

    if(ZEND_NUM_ARGS() == 2)
    {
      zval **arg2;

      if(zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE)
	WRONG_PARAM_COUNT;

      convert_to_long_ex(arg2);
      flags = (unsigned char)Z_LVAL_PP(arg2);
    }
    else
    {
      if(zend_get_parameters_ex(1, &arg1) == FAILURE)
	WRONG_PARAM_COUNT;
    }

    convert_to_object_ex(arg1);

    if((*arg1)->value.obj.ce == &gradient_class_entry)
    {
      if(flags == 0)
	flags = SWFFILL_LINEAR_GRADIENT;

      fill = SWFShape_addGradientFill(getShape(getThis()), getGradient(*arg1),
				      flags);
    }
    else if((*arg1)->value.obj.ce == &bitmap_class_entry)
    {
      if(flags == 0)
	flags = SWFFILL_TILED_BITMAP;

      fill = SWFShape_addBitmapFill(getShape(getThis()), getBitmap(*arg1),
				    flags);
    }
    else
      php_error(E_ERROR, "argument to addfill not a bitmap nor a gradient");
  }

  else if(ZEND_NUM_ARGS() == 3 || ZEND_NUM_ARGS() == 4)
  {
    /* it's a solid fill */
    zval **r, **g, **b, **za;
    int a = 0xff;

    if(ZEND_NUM_ARGS() == 3)
    {
      if(zend_get_parameters_ex(3, &r, &g, &b) == FAILURE)
	WRONG_PARAM_COUNT;
    }
    else if(ZEND_NUM_ARGS() == 4)
    {
      if(zend_get_parameters_ex(4, &r, &g, &b, &za) == FAILURE)
	WRONG_PARAM_COUNT;

      convert_to_long_ex(za);
      a = Z_LVAL_PP(za);
    }
    else
      WRONG_PARAM_COUNT;

    convert_to_long_ex(r);
    convert_to_long_ex(g);
    convert_to_long_ex(b);

    fill = SWFShape_addSolidFill(getShape(getThis()),
				 Z_LVAL_PP(r), Z_LVAL_PP(g),
				 Z_LVAL_PP(b), a);
  }

  else
    WRONG_PARAM_COUNT;

  if(!fill)
    php_error(E_ERROR, "Error adding fill to shape!");


  /* return an SWFFill object */
  ret = zend_list_insert(fill, le_swffillp);
  object_init_ex(return_value, &fill_class_entry);
  add_property_resource(return_value, "fill", ret);
}

/* }}} */
/* {{{ proto void swfshape_setleftfill(SWFFill fill)
   Sets the left side fill style to fill */

PHP_FUNCTION(swfshape_setleftfill)
{
  zval **zfill, **r, **g, **b, **a;
  SWFFill fill;

  if(ZEND_NUM_ARGS() == 3)
  {
    if(zend_get_parameters_ex(3, &r, &g, &b) == FAILURE)
      WRONG_PARAM_COUNT;

    convert_to_long_ex(r);
    convert_to_long_ex(g);
    convert_to_long_ex(b);

    fill = SWFShape_addSolidFill(getShape(getThis()), Z_LVAL_PP(r),
				 Z_LVAL_PP(g), Z_LVAL_PP(b), 0xff);
  }

  else if(ZEND_NUM_ARGS() == 4)
  {
    if(zend_get_parameters_ex(4, &r, &g, &b, &a) == FAILURE)
      WRONG_PARAM_COUNT;

    convert_to_long_ex(r);
    convert_to_long_ex(g);
    convert_to_long_ex(b);
    convert_to_long_ex(a);

    fill = SWFShape_addSolidFill(getShape(getThis()), Z_LVAL_PP(r),
				 Z_LVAL_PP(g), Z_LVAL_PP(b), Z_LVAL_PP(a));
  }

  else if(ZEND_NUM_ARGS() == 1)
  {
    if(zend_get_parameters_ex(1, &zfill) == FAILURE)
      WRONG_PARAM_COUNT;

    if(Z_LVAL_PP(zfill) != 0)
    {
      convert_to_object_ex(zfill);
      fill = getFill(*zfill);
    }
    else
      fill = NULL;
  }

  SWFShape_setLeftFill(getShape(getThis()), fill);
}

/* }}} */
/* {{{ proto void swfshape_setrightfill(SWFFill fill)
   Sets the right side fill style to fill */

PHP_FUNCTION(swfshape_setrightfill)
{
  zval **zfill, **r, **g, **b, **a;
  SWFFill fill;

  if(ZEND_NUM_ARGS() == 3)
  {
    if(zend_get_parameters_ex(3, &r, &g, &b) == FAILURE)
      WRONG_PARAM_COUNT;

    convert_to_long_ex(r);
    convert_to_long_ex(g);
    convert_to_long_ex(b);

    fill = SWFShape_addSolidFill(getShape(getThis()), Z_LVAL_PP(r),
				 Z_LVAL_PP(g), Z_LVAL_PP(b), 0xff);
  }

  else if(ZEND_NUM_ARGS() == 4)
  {
    if(zend_get_parameters_ex(4, &r, &g, &b, &a) == FAILURE)
      WRONG_PARAM_COUNT;

    convert_to_long_ex(r);
    convert_to_long_ex(g);
    convert_to_long_ex(b);
    convert_to_long_ex(a);

    fill = SWFShape_addSolidFill(getShape(getThis()), Z_LVAL_PP(r),
				 Z_LVAL_PP(g), Z_LVAL_PP(b), Z_LVAL_PP(a));
  }

  else if(ZEND_NUM_ARGS() == 1)
  {
    if(zend_get_parameters_ex(1, &zfill) == FAILURE)
      WRONG_PARAM_COUNT;

    if(Z_LVAL_PP(zfill) != 0)
    {
      convert_to_object_ex(zfill);
      fill = getFill(*zfill);
    }
    else
      fill = NULL;
  }

  SWFShape_setRightFill(getShape(getThis()), fill);
}

/* }}} */
/* {{{ proto void swfshape_movepento(double x, double y)
   Moves the pen to shape coordinates (x,y) */

PHP_FUNCTION(swfshape_movepento)
{
  zval **x, **y;

  if((ZEND_NUM_ARGS() != 2) || zend_get_parameters_ex(2, &x, &y) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(x);
  convert_to_double_ex(y);

  SWFShape_movePenTo(getShape(getThis()),
		     Z_DVAL_PP(x), Z_DVAL_PP(y));
}

/* }}} */
/* {{{ proto void swfshape_movepen(double x, double y)
   Moves the pen from its current location by vector (x,y) */

PHP_FUNCTION(swfshape_movepen)
{
  zval **x, **y;

  if((ZEND_NUM_ARGS() != 2) || zend_get_parameters_ex(2, &x, &y) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(x);
  convert_to_double_ex(y);

  SWFShape_movePen(getShape(getThis()),
		   Z_DVAL_PP(x), Z_DVAL_PP(y));
}

/* }}} */
/* {{{ proto void swfshape_drawlineto(double x, double y)
   Draws a line from the current pen position to shape coordinates (x,y)
   in the current line style */

PHP_FUNCTION(swfshape_drawlineto)
{
  zval **x, **y;

  if ((ZEND_NUM_ARGS() != 2) || zend_get_parameters_ex(2, &x, &y) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(x);
  convert_to_double_ex(y);

  SWFShape_drawLineTo(getShape(getThis()), Z_DVAL_PP(x), Z_DVAL_PP(y));
}

/* }}} */
/* {{{ proto void swfshape_drawline(double dx, double dy)
   Draws a line from the current pen position (x,y) to the point (x+dx,y+dy)
   in the current line style */

PHP_FUNCTION(swfshape_drawline)
{
  zval **x, **y;

  if ((ZEND_NUM_ARGS() != 2) || zend_get_parameters_ex(2, &x, &y) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(x);
  convert_to_double_ex(y);

  SWFShape_drawLine(getShape(getThis()), Z_DVAL_PP(x), Z_DVAL_PP(y));
}

/* }}} */
/* {{{ proto void swfshape_drawcurveto(double ax, double ay, double bx, double by [, double dx, double dy])
   Draws a curve from the current pen position (x,y) to the point (bx,by)
   in the current line style, using point (ax,ay) as a control point.
   Or draws a cubic bezier to point (dx,dy) with control points (ax,ay) and (bx,by)
*/

PHP_FUNCTION(swfshape_drawcurveto)
{
  if(ZEND_NUM_ARGS() == 4)
  {
    zval **cx, **cy, **ax, **ay;

    if(zend_get_parameters_ex(4, &cx, &cy, &ax, &ay) == FAILURE)
      WRONG_PARAM_COUNT;

    convert_to_double_ex(cx);
    convert_to_double_ex(cy);
    convert_to_double_ex(ax);
    convert_to_double_ex(ay);

    SWFShape_drawCurveTo(getShape(getThis()),
			 Z_DVAL_PP(cx), Z_DVAL_PP(cy),
			 Z_DVAL_PP(ax), Z_DVAL_PP(ay));
  }
  else if(ZEND_NUM_ARGS() == 6)
  {
    zval **bx, **by, **cx, **cy, **dx, **dy;

    if(zend_get_parameters_ex(6, &bx, &by, &cx, &cy, &dx, &dy) == FAILURE)
      WRONG_PARAM_COUNT;

    convert_to_double_ex(bx);
    convert_to_double_ex(by);
    convert_to_double_ex(cx);
    convert_to_double_ex(cy);
    convert_to_double_ex(dx);
    convert_to_double_ex(dy);

    RETURN_LONG(SWFShape_drawCubicTo(getShape(getThis()),
				     Z_DVAL_PP(bx), Z_DVAL_PP(by),
				     Z_DVAL_PP(cx), Z_DVAL_PP(cy),
				     Z_DVAL_PP(dx), Z_DVAL_PP(dy)));
  }
  else
    WRONG_PARAM_COUNT;
}

/* }}} */
/* {{{ proto void swfshape_drawcurve(double adx, double ady, double bdx, double bdy [, double cdx, double cdy])
   Draws a curve from the current pen position (x,y) to the point (x+bdx,y+bdy)
   in the current line style, using point (x+adx,y+ady) as a control point
   Or draws a cubic bezier to point (x+cdx,x+cdy) with control points
   (x+adx,y+ady) and (x+bdx,y+bdy)
*/

PHP_FUNCTION(swfshape_drawcurve)
{
  if(ZEND_NUM_ARGS() == 4)
  {
    zval **cx, **cy, **ax, **ay;

    if(zend_get_parameters_ex(4, &cx, &cy, &ax, &ay) == FAILURE)
      WRONG_PARAM_COUNT;

    convert_to_double_ex(cx);
    convert_to_double_ex(cy);
    convert_to_double_ex(ax);
    convert_to_double_ex(ay);

    SWFShape_drawCurve(getShape(getThis()),
		       Z_DVAL_PP(cx), Z_DVAL_PP(cy),
		       Z_DVAL_PP(ax), Z_DVAL_PP(ay));
  }
  else if(ZEND_NUM_ARGS() == 6)
  {
    zval **bx, **by, **cx, **cy, **dx, **dy;

    if(zend_get_parameters_ex(6, &bx, &by, &cx, &cy, &dx, &dy) == FAILURE)
      WRONG_PARAM_COUNT;

    convert_to_double_ex(bx);
    convert_to_double_ex(by);
    convert_to_double_ex(cx);
    convert_to_double_ex(cy);
    convert_to_double_ex(dx);
    convert_to_double_ex(dy);

    RETURN_LONG(SWFShape_drawCubic(getShape(getThis()),
				   Z_DVAL_PP(bx), Z_DVAL_PP(by),
				   Z_DVAL_PP(cx), Z_DVAL_PP(cy),
				   Z_DVAL_PP(dx), Z_DVAL_PP(dy)));
  }
  else
    WRONG_PARAM_COUNT;
}

/* }}} */
/* {{{ proto void swfshape_drawglyph(SWFFont font, string character)
   Draws the first character in the given string into the shape using the
   glyph definition from the given font */

PHP_FUNCTION(swfshape_drawglyph)
{
  zval **font, **c;

  if((ZEND_NUM_ARGS() != 2) || zend_get_parameters_ex(2, &font, &c) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_string_ex(c);

  SWFShape_drawFontGlyph(getShape(getThis()), getFont(*font),
			 Z_STRVAL_PP(c)[0]);
}

/* }}} */
/* {{{ proto void swfshape_drawcircle(int r)
   Draws a circle of radius r centered at the current location,
   in a counter-clockwise fashion */

PHP_FUNCTION(swfshape_drawcircle)
{
  zval **r;

  if((ZEND_NUM_ARGS() != 1) || zend_get_parameters_ex(1, &r) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(r);

  SWFShape_drawCircle(getShape(getThis()), Z_DVAL_PP(r));
}

/* }}} */
/* {{{ proto void swfshape_drawarc(int r, float startAngle, float endAngle)
   Draws an arc of radius r centered at the current location, from angle
   startAngle to angle endAngle measured counterclockwise from 12 o'clock */

PHP_FUNCTION(swfshape_drawarc)
{
  zval **r, **start, **end;

  if((ZEND_NUM_ARGS() != 3) ||
     zend_get_parameters_ex(3, &r, &start, &end) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(r);
  convert_to_double_ex(start);
  convert_to_double_ex(end);

  /* convert angles to radians, since that's what php uses elsewhere */
  SWFShape_drawArc(getShape(getThis()), Z_DVAL_PP(r),
		   Z_DVAL_PP(start)*M_PI/180, Z_DVAL_PP(end)*M_PI/180);
}

/* }}} */
/* {{{ proto void swfshape_drawcubic(double bx, double by, double cx, double cy, double dx, double dy)
   Draws a cubic bezier curve using the current position and the three given
   points as control points */

PHP_FUNCTION(swfshape_drawcubic)
{
  zval **bx, **by, **cx, **cy, **dx, **dy;

  if((ZEND_NUM_ARGS() != 6) ||
     zend_get_parameters_ex(6, &bx, &by, &cx, &cy, &dx, &dy) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(bx);
  convert_to_double_ex(by);
  convert_to_double_ex(cx);
  convert_to_double_ex(cy);
  convert_to_double_ex(dx);
  convert_to_double_ex(dy);

  RETURN_LONG(SWFShape_drawCubic(getShape(getThis()),
				 Z_DVAL_PP(bx), Z_DVAL_PP(by),
				 Z_DVAL_PP(cx), Z_DVAL_PP(cy),
				 Z_DVAL_PP(dx), Z_DVAL_PP(dy)));
}

/* }}} */
/* {{{ proto void swfshape_drawcubic(double bx, double by, double cx, double cy, double dx, double dy)
   Draws a cubic bezier curve using the current position and the three given
   points as control points */

PHP_FUNCTION(swfshape_drawcubicto)
{
  zval **bx, **by, **cx, **cy, **dx, **dy;

  if((ZEND_NUM_ARGS() != 6) ||
     zend_get_parameters_ex(6, &bx, &by, &cx, &cy, &dx, &dy) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(bx);
  convert_to_double_ex(by);
  convert_to_double_ex(cx);
  convert_to_double_ex(cy);
  convert_to_double_ex(dx);
  convert_to_double_ex(dy);

  RETURN_LONG(SWFShape_drawCubicTo(getShape(getThis()),
				   Z_DVAL_PP(bx), Z_DVAL_PP(by),
				   Z_DVAL_PP(cx), Z_DVAL_PP(cy),
				   Z_DVAL_PP(dx), Z_DVAL_PP(dy)));
}

/* }}} */


/* }}} */
/* {{{ SWFSprite */

static zend_function_entry swfsprite_functions[] = {
  PHP_FALIAS(swfsprite,          swfsprite_init,              NULL)
  PHP_FALIAS(add,                swfsprite_add,               NULL)
  PHP_FALIAS(remove,             swfsprite_remove,            NULL)
  PHP_FALIAS(nextframe,          swfsprite_nextFrame,         NULL)
  PHP_FALIAS(labelframe,         swfsprite_labelFrame,        NULL)
  PHP_FALIAS(setframes,          swfsprite_setFrames,         NULL)
  { NULL, NULL, NULL }
};

/* {{{ proto class swfsprite_init()
   Returns a new SWFSprite object */

PHP_FUNCTION(swfsprite_init)
{
  SWFMovieClip sprite = newSWFMovieClip();
  int ret = zend_list_insert(sprite, le_swfspritep);

  object_init_ex(getThis(), &sprite_class_entry);
  add_property_resource(getThis(), "sprite", ret);
  zend_list_addref(ret);
}
static void destroy_SWFSprite_resource(zend_rsrc_list_entry *resource)
{
  destroySWFMovieClip((SWFMovieClip)resource->ptr);
}

/* }}} */
/* {{{ internal function SWFSprite getSprite(zval *id)
   Returns the SWFSprite object in zval *id */

SWFMovieClip getSprite(zval *id)
{
  void *sprite = SWFgetProperty(id, "sprite", 6, le_swfspritep);

  if(!sprite)
    php_error(E_ERROR, "called object is not an SWFSprite!");

  return (SWFMovieClip)sprite;
}

/* }}} */
/* {{{ proto SWFDisplayItem swfsprite_add(SWFCharacter)
   Adds the character to the sprite, returns a displayitem */

PHP_FUNCTION(swfsprite_add)
{
  zval **zchar;
  int ret;
  SWFBlock block;
  SWFDisplayItem item;
  SWFMovieClip sprite = getSprite(getThis());

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &zchar) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_object_ex(zchar);

  if((*zchar)->value.obj.ce == &action_class_entry)
    block = (SWFBlock)getAction(*zchar);
  else
    block = (SWFBlock)getCharacter(*zchar);

  item = SWFMovieClip_add(sprite, block);

  if(item != NULL)
  {
    /* try and create a displayitem object */
    ret = zend_list_insert(item, le_swfdisplayitemp);
    object_init_ex(return_value, &displayitem_class_entry);
    add_property_resource(return_value, "displayitem", ret);
  }
}

/* }}} */
/* {{{ proto void swfsprite_remove(SWFDisplayItem)
   Remove the named character from the sprite's display list */

PHP_FUNCTION(swfsprite_remove)
{
  zval **zchar;
  SWFDisplayItem item;
  SWFMovieClip movie = getSprite(getThis());

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &zchar) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_object_ex(zchar);
  item = getDisplayItem(*zchar);

  SWFMovieClip_remove(movie, item);
}

/* }}} */
/* {{{ proto void swfsprite_nextFrame()
   moves the sprite to the next frame */

PHP_FUNCTION(swfsprite_nextFrame)
{
  SWFMovieClip_nextFrame(getSprite(getThis()));
}

/* }}} */
/* {{{ swfmovie_labelframe */

PHP_FUNCTION(swfsprite_labelFrame)
{
  zval **label;

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &label) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_string_ex(label);

  SWFMovieClip_labelFrame(getSprite(getThis()), Z_STRVAL_PP(label));
}

/* }}} */
/* {{{ proto void swfsprite_setFrames(int)
   sets the number of frames in this SWFSprite */

PHP_FUNCTION(swfsprite_setFrames)
{
  zval **frames;
  SWFMovieClip sprite = getSprite(getThis());

  if((ZEND_NUM_ARGS() != 1) || zend_get_parameters_ex(1, &frames) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_long_ex(frames);

  SWFMovieClip_setNumberOfFrames(sprite, Z_LVAL_PP(frames));
}

/* }}} */

/* }}} */
/* {{{ SWFText */

static zend_function_entry swftext_functions[] = {
  PHP_FALIAS(swftext,                swftext_init,              NULL)
  PHP_FALIAS(setfont,                swftext_setFont,           NULL)
  PHP_FALIAS(setheight,              swftext_setHeight,         NULL)
  PHP_FALIAS(setspacing,             swftext_setSpacing,        NULL)
  PHP_FALIAS(setcolor,               swftext_setColor,          NULL)
  PHP_FALIAS(moveto,                 swftext_moveTo,            NULL)
  PHP_FALIAS(addstring,              swftext_addString,         NULL)
  PHP_FALIAS(getwidth,               swftext_getWidth,          NULL)
  PHP_FALIAS(getascent,              swftext_getAscent,         NULL)
  PHP_FALIAS(getdescent,             swftext_getDescent,        NULL)
  PHP_FALIAS(getleading,             swftext_getLeading,        NULL)
  { NULL, NULL, NULL }
};

/* {{{ proto class swftext_init(void)
   Returns new SWFText object */

PHP_FUNCTION(swftext_init)
{
  SWFText text = newSWFText2();
  int ret = zend_list_insert(text, le_swftextp);

  object_init_ex(getThis(), &text_class_entry);
  add_property_resource(getThis(), "text", ret);
  zend_list_addref(ret);
}
static void destroy_SWFText_resource(zend_rsrc_list_entry *resource)
{
  destroySWFText((SWFText)resource->ptr);
}

/* }}} */
/* {{{ internal function SWFText getText(zval *id)
   Returns the SWFText contained in zval *id */

SWFText getText(zval *id)
{
  void *text = SWFgetProperty(id, "text", 4, le_swftextp);

  if(!text)
    php_error(E_ERROR, "called object is not an SWFText!");

  return (SWFText)text;
}

/* }}} */
/* {{{ proto void swftext_setFont(class font)
   Sets this SWFText object's current font to given font */

PHP_FUNCTION(swftext_setFont)
{
  zval **zfont;
  SWFText text = getText(getThis());
  SWFFont font;

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &zfont) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_object_ex(zfont);
  font = getFont(*zfont);

  SWFText_setFont(text, font);
}

/* }}} */
/* {{{ proto void swftext_setHeight(double height)
   Sets this SWFText object's current height to given height */

PHP_FUNCTION(swftext_setHeight)
{
  zval **height;
  SWFText text = getText(getThis());

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &height) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(height);

  SWFText_setHeight(text, Z_DVAL_PP(height));
}

/* }}} */
/* {{{ proto void swftext_setSpacing(float spacing)
   Sets this SWFText object's current letterspacing to given spacing */

PHP_FUNCTION(swftext_setSpacing)
{
  zval **spacing;
  SWFText text = getText(getThis());

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &spacing) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(spacing);

  SWFText_setSpacing(text, Z_DVAL_PP(spacing));
}

/* }}} */
/* {{{ proto void swftext_setColor(int r, int g, int b, [int a])
   Sets this SWFText object's current color to the given color */

PHP_FUNCTION(swftext_setColor)
{
  zval **r, **g, **b, **a;
  SWFText text = getText(getThis());

  if(ZEND_NUM_ARGS() == 3)
  {
    if(zend_get_parameters_ex(3, &r, &g, &b) == FAILURE)
      WRONG_PARAM_COUNT;
  }
  else if(ZEND_NUM_ARGS() == 4)
  {
    if(zend_get_parameters_ex(4, &r, &g, &b, &a) == FAILURE)
      WRONG_PARAM_COUNT;

    convert_to_long_ex(a);
  }
  else
    WRONG_PARAM_COUNT;

  convert_to_long_ex(r);
  convert_to_long_ex(g);
  convert_to_long_ex(b);

  if(ZEND_NUM_ARGS() == 4)
  {
    SWFText_setColor(text, Z_LVAL_PP(r), Z_LVAL_PP(g), Z_LVAL_PP(b), Z_LVAL_PP(a));
  }
  else
  {
    SWFText_setColor(text, Z_LVAL_PP(r), Z_LVAL_PP(g), Z_LVAL_PP(b), 0xff);
  }
}

/* }}} */
/* {{{ proto void swftext_moveTo(double x, double y)
   Moves this SWFText object's current pen position to (x,y) in text
   coordinates */

PHP_FUNCTION(swftext_moveTo)
{
  zval **x, **y;
  SWFText text = getText(getThis());

  if(ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &x, &y) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(x);
  convert_to_double_ex(y);

  SWFText_setXY(text, Z_DVAL_PP(x), Z_DVAL_PP(y));
}

/* }}} */
/* {{{ proto void swftext_addString(string text)
   Writes the given text into this SWFText object at the current pen position,
   using the current font, height, spacing, and color */

PHP_FUNCTION(swftext_addString)
{
  zval **s;
  SWFText text = getText(getThis());

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &s) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_string_ex(s);

  SWFText_addString(text, Z_STRVAL_PP(s), NULL);
}

/* }}} */
/* {{{ proto double swftext_getWidth(string)
   calculates the width of the given string in this text objects current font and size */

PHP_FUNCTION(swftext_getWidth)
{
  zval **zstring;
  int width;

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &zstring) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_string_ex(zstring);

  width = SWFText_getStringWidth(getText(getThis()), Z_STRVAL_PP(zstring));

  RETURN_DOUBLE(width);
}

/* }}} */
/* {{{ proto double swftext_getAscent()
   returns the ascent of the current font at its current size, or 0 if not available */

PHP_FUNCTION(swftext_getAscent)
{
  if(ZEND_NUM_ARGS() != 0)
    WRONG_PARAM_COUNT;

  RETURN_DOUBLE(SWFText_getAscent(getText(getThis())));
}

/* }}} */
/* {{{ proto double swftext_getDescent()
   returns the descent of the current font at its current size, or 0 if not available */

PHP_FUNCTION(swftext_getDescent)
{
  if(ZEND_NUM_ARGS() != 0)
    WRONG_PARAM_COUNT;

  RETURN_DOUBLE(SWFText_getDescent(getText(getThis())));
}

/* }}} */
/* {{{ proto double swftext_getLeading()
   returns the leading of the current font at its current size, or 0 if not available */

PHP_FUNCTION(swftext_getLeading)
{
  if(ZEND_NUM_ARGS() != 0)
    WRONG_PARAM_COUNT;

  RETURN_DOUBLE(SWFText_getLeading(getText(getThis())));
}

/* }}} */

/* }}} */
/* {{{ SWFTextField */

static zend_function_entry swftextfield_functions[] = {
  PHP_FALIAS(swftextfield,      swftextfield_init,            NULL)
  PHP_FALIAS(setfont,           swftextfield_setFont,         NULL)
  PHP_FALIAS(setbounds,         swftextfield_setBounds,       NULL)
  PHP_FALIAS(align,             swftextfield_align,           NULL)
  PHP_FALIAS(setheight,         swftextfield_setHeight,       NULL)
  PHP_FALIAS(setleftmargin,     swftextfield_setLeftMargin,   NULL)
  PHP_FALIAS(setrightmargin,    swftextfield_setRightMargin,  NULL)
  PHP_FALIAS(setmargins,        swftextfield_setMargins,      NULL)
  PHP_FALIAS(setindentation,    swftextfield_setIndentation,  NULL)
  PHP_FALIAS(setlinespacing,    swftextfield_setLineSpacing,  NULL)
  PHP_FALIAS(setcolor,          swftextfield_setColor,        NULL)
  PHP_FALIAS(setname,           swftextfield_setName,         NULL)
  PHP_FALIAS(addstring,         swftextfield_addString,       NULL)
  { NULL, NULL, NULL }
};

/* {{{ proto object swftextfield_init(void)
   returns a new SWFTextField object */

PHP_FUNCTION(swftextfield_init)
{
  zval **flags;
  SWFTextField field = newSWFTextField();
  int ret = zend_list_insert(field, le_swftextfieldp);

  object_init_ex(getThis(), &textfield_class_entry);
  add_property_resource(getThis(), "textfield", ret);
  zend_list_addref(ret);

  if(ZEND_NUM_ARGS() == 1)
  {
    if(zend_get_parameters_ex(1, &flags) == FAILURE)
      WRONG_PARAM_COUNT;

    convert_to_long_ex(flags);
    SWFTextField_setFlags(field, Z_LVAL_PP(flags));
  }
}
static void destroy_SWFTextField_resource(zend_rsrc_list_entry *resource)
{
  destroySWFTextField((SWFTextField)resource->ptr);
}

/* }}} */
/* {{{ internal function getTextField
   Returns the SWFTextField object contained in zval *id */

static inline SWFTextField getTextField(zval *id)
{
  void *field = SWFgetProperty(id, "textfield", 9, le_swftextfieldp);

  if(!field)
    php_error(E_ERROR, "called object is not an SWFTextField!");

  return (SWFTextField)field;
}

/* }}} */
/* {{{ proto void swftextfield_setFont
   set the font for this textfield */

PHP_FUNCTION(swftextfield_setFont)
{
  zval **font;
  SWFTextField field = getTextField(getThis());

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &font) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_object_ex(font);

  SWFTextField_setFont(field, getFont(*font));
}

/* }}} */
/* {{{ proto void swftextfield_setBounds(double width, double height)
   sets the width and height of this textfield */

PHP_FUNCTION(swftextfield_setBounds)
{
  zval **width, **height;
  SWFTextField field = getTextField(getThis());

  if(ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &width, &height) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(width);
  convert_to_double_ex(height);

  SWFTextField_setBounds(field, Z_DVAL_PP(width), Z_DVAL_PP(height));
}

/* }}} */
/* {{{ proto void swftextfield_align(alignment)
   sets the alignment of this textfield */

PHP_FUNCTION(swftextfield_align)
{
  zval **align;
  SWFTextField field = getTextField(getThis());

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &align) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_long_ex(align);

  SWFTextField_setAlignment(field, Z_LVAL_PP(align));
}

/* }}} */
/* {{{ proto void swftextfield_setHeight(double height)
   sets the font height of this textfield */

PHP_FUNCTION(swftextfield_setHeight)
{
  zval **height;
  SWFTextField field = getTextField(getThis());

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &height) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(height);

  SWFTextField_setHeight(field, Z_DVAL_PP(height));
}

/* }}} */
/* {{{ proto void swftextfield_setLeftMargin(double)
   sets the left margin of this textfield */

PHP_FUNCTION(swftextfield_setLeftMargin)
{
  zval **margin;
  SWFTextField field = getTextField(getThis());

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &margin) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(margin);

  SWFTextField_setLeftMargin(field, Z_DVAL_PP(margin));
}

/* }}} */
/* {{{ proto void swftextfield_setRightMargin(double)
   sets the right margin of this textfield */

PHP_FUNCTION(swftextfield_setRightMargin)
{
  zval **margin;
  SWFTextField field = getTextField(getThis());

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &margin) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(margin);

  SWFTextField_setRightMargin(field, Z_DVAL_PP(margin));
}

/* }}} */
/* {{{ proto void swftextfield_setMargins(double left, double right)
   sets both margins of this textfield */

PHP_FUNCTION(swftextfield_setMargins)
{
  zval **left, **right;
  SWFTextField field = getTextField(getThis());

  if(ZEND_NUM_ARGS() != 2 ||
     zend_get_parameters_ex(2, &left, &right) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(left);
  convert_to_double_ex(right);

  SWFTextField_setLeftMargin(field, Z_DVAL_PP(left));
  SWFTextField_setRightMargin(field, Z_DVAL_PP(right));
}

/* }}} */
/* {{{ proto void swftextfield_setIndentation(double)
   sets the indentation of the first line of this textfield */

PHP_FUNCTION(swftextfield_setIndentation)
{
  zval **indent;
  SWFTextField field = getTextField(getThis());

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &indent) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(indent);

  SWFTextField_setIndentation(field, Z_DVAL_PP(indent));
}

/* }}} */
/* {{{ proto void swftextfield_setLineSpacing(double)
   sets the line spacing of this textfield */

PHP_FUNCTION(swftextfield_setLineSpacing)
{
  zval **spacing;
  SWFTextField field = getTextField(getThis());

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &spacing) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_double_ex(spacing);

  SWFTextField_setLineSpacing(field, Z_DVAL_PP(spacing));
}

/* }}} */
/* {{{ proto void swftextfield_setColor(int r, int g, int b [, int a])
   sets the color of this textfield */

PHP_FUNCTION(swftextfield_setColor)
{
  zval **r, **g, **b, **a;
  SWFTextField field = getTextField(getThis());
  int alpha = 0xff;

  if(ZEND_NUM_ARGS() == 3)
  {
    if(zend_get_parameters_ex(3, &r, &g, &b) == FAILURE)
      WRONG_PARAM_COUNT;
  }
  else if(ZEND_NUM_ARGS() == 4)
  {
    if(zend_get_parameters_ex(4, &r, &g, &b, &a) == FAILURE)
      WRONG_PARAM_COUNT;

    convert_to_long_ex(a);
    alpha = Z_LVAL_PP(a);
  }
  else
    WRONG_PARAM_COUNT;

  convert_to_long_ex(r);
  convert_to_long_ex(g);
  convert_to_long_ex(b);

  SWFTextField_setColor(field, Z_LVAL_PP(r), Z_LVAL_PP(g), Z_LVAL_PP(b), alpha);
}

/* }}} */
/* {{{ proto void swftextfield_setName(string)
   sets the variable name of this textfield */

PHP_FUNCTION(swftextfield_setName)
{
  zval **name;
  SWFTextField field = getTextField(getThis());

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &name) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_string_ex(name);

  SWFTextField_setVariableName(field, Z_STRVAL_PP(name));
}

/* }}} */
/* {{{ proto void swftextfield_addString(string)
   adds the given string to this textfield */

PHP_FUNCTION(swftextfield_addString)
{
  zval **string;
  SWFTextField field = getTextField(getThis());

  if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &string) == FAILURE)
    WRONG_PARAM_COUNT;

  convert_to_string_ex(string);

  SWFTextField_addString(field, Z_STRVAL_PP(string));
}

/* }}} */

/* }}} */

zend_module_entry ming_module_entry = {
	"ming",
	ming_functions,
	PHP_MINIT(ming),
	NULL,
	NULL,
	NULL,
	PHP_MINFO(ming),
	STANDARD_MODULE_PROPERTIES
};

#if defined(COMPILE_DL) || defined(COMPILE_DL_MING)
ZEND_GET_MODULE(ming)
#endif

/* {{{ proto PHP_MINFO_FUNCTION(ming)
*/

PHP_MINFO_FUNCTION(ming)
{
  php_info_print_table_start();
  php_info_print_table_row(2, "Ming SWF output library", "the funk in your trunk");
  php_info_print_table_row(2, "Version", MING_VERSION_TEXT);
  php_info_print_table_end();
}

/* }}} */
/* {{{ proto PHP_MINIT_FUNCTION(ming)
*/

PHP_MINIT_FUNCTION(ming)
{
  if(Ming_init() != 0)
    php_error(E_ERROR, "Error initializing Ming module");

  le_fopen = php_file_le_fopen();


#define CONSTANT(s,c) REGISTER_LONG_CONSTANT((s), (c), CONST_CS | CONST_PERSISTENT)

  /* flags for SWFButton_addShape */
  CONSTANT("SWFBUTTON_HIT",               SWFBUTTONRECORD_HITSTATE);
  CONSTANT("SWFBUTTON_DOWN",              SWFBUTTONRECORD_DOWNSTATE);
  CONSTANT("SWFBUTTON_OVER",              SWFBUTTONRECORD_OVERSTATE);
  CONSTANT("SWFBUTTON_UP",                SWFBUTTONRECORD_UPSTATE);

  /* flags for SWFButton_addAction */
  CONSTANT("SWFBUTTON_MOUSEUPOUTSIDE",    SWFBUTTON_MOUSEUPOUTSIDE);
  CONSTANT("SWFBUTTON_DRAGOVER",          SWFBUTTON_DRAGOVER);
  CONSTANT("SWFBUTTON_DRAGOUT",           SWFBUTTON_DRAGOUT);
  CONSTANT("SWFBUTTON_MOUSEUP",           SWFBUTTON_MOUSEUP);
  CONSTANT("SWFBUTTON_MOUSEDOWN",         SWFBUTTON_MOUSEDOWN);
  CONSTANT("SWFBUTTON_MOUSEOUT",          SWFBUTTON_MOUSEOUT);
  CONSTANT("SWFBUTTON_MOUSEOVER",         SWFBUTTON_MOUSEOVER);

  /* flags for SWFFill */
  CONSTANT("SWFFILL_RADIAL_GRADIENT",     SWFFILL_RADIAL_GRADIENT);
  CONSTANT("SWFFILL_LINEAR_GRADIENT",     SWFFILL_LINEAR_GRADIENT);
  CONSTANT("SWFFILL_TILED_BITMAP",        SWFFILL_TILED_BITMAP);
  CONSTANT("SWFFILL_CLIPPED_BITMAP",      SWFFILL_CLIPPED_BITMAP);

  /* flags for SWFTextField_init */
  CONSTANT("SWFTEXTFIELD_HASLENGTH",      SWFTEXTFIELD_HASLENGTH);
  CONSTANT("SWFTEXTFIELD_NOEDIT",         SWFTEXTFIELD_NOEDIT);
  CONSTANT("SWFTEXTFIELD_PASSWORD",       SWFTEXTFIELD_PASSWORD);
  CONSTANT("SWFTEXTFIELD_MULTILINE",      SWFTEXTFIELD_MULTILINE);
  CONSTANT("SWFTEXTFIELD_WORDWRAP",       SWFTEXTFIELD_WORDWRAP);
  CONSTANT("SWFTEXTFIELD_DRAWBOX",        SWFTEXTFIELD_DRAWBOX);
  CONSTANT("SWFTEXTFIELD_NOSELECT",       SWFTEXTFIELD_NOSELECT);
#ifdef SWFTEXTFIELD_HTML
  CONSTANT("SWFTEXTFIELD_HTML",           SWFTEXTFIELD_HTML);
#endif

  /* flags for SWFTextField_align */
  CONSTANT("SWFTEXTFIELD_ALIGN_LEFT",     SWFTEXTFIELD_ALIGN_LEFT);
  CONSTANT("SWFTEXTFIELD_ALIGN_RIGHT",    SWFTEXTFIELD_ALIGN_RIGHT);
  CONSTANT("SWFTEXTFIELD_ALIGN_CENTER",   SWFTEXTFIELD_ALIGN_CENTER);
  CONSTANT("SWFTEXTFIELD_ALIGN_JUSTIFY",  SWFTEXTFIELD_ALIGN_JUSTIFY);

  le_swfmoviep = zend_register_list_destructors_ex(destroy_SWFMovie_resource, NULL, "SWFMovie", module_number);
  le_swfshapep = zend_register_list_destructors_ex(destroy_SWFShape_resource, NULL, "SWFShape", module_number);
  le_swffillp = zend_register_list_destructors_ex(destroy_SWFFill_resource, NULL, "SWFFill", module_number);
  le_swfgradientp = zend_register_list_destructors_ex(destroy_SWFGradient_resource, NULL, "SWFGradient", module_number);
  le_swfbitmapp = zend_register_list_destructors_ex(destroy_SWFBitmap_resource, NULL, "SWFBitmap", module_number);
  le_swftextp = zend_register_list_destructors_ex(destroy_SWFText_resource, NULL, "SWFText", module_number);
  le_swftextfieldp = zend_register_list_destructors_ex(destroy_SWFTextField_resource, NULL, "SWFTextField", module_number);
  le_swffontp = zend_register_list_destructors_ex(destroy_SWFFont_resource, NULL, "SWFFont", module_number);
  le_swfbuttonp = zend_register_list_destructors_ex(destroy_SWFButton_resource, NULL, "SWFButton", module_number);
  le_swfmorphp = zend_register_list_destructors_ex(destroy_SWFMorph_resource, NULL, "SWFMorph", module_number);
  le_swfspritep = zend_register_list_destructors_ex(destroy_SWFSprite_resource, NULL, "SWFSprite", module_number);

  le_swfdisplayitemp = zend_register_list_destructors_ex(NULL, NULL, "SWFDisplayItem", module_number);
  le_swfactionp = zend_register_list_destructors_ex(NULL, NULL, "SWFAction", module_number);

  INIT_CLASS_ENTRY(shape_class_entry, "swfshape", swfshape_functions);
  INIT_CLASS_ENTRY(fill_class_entry, "swffill", swffill_functions);
  INIT_CLASS_ENTRY(gradient_class_entry, "swfgradient", swfgradient_functions);
  INIT_CLASS_ENTRY(bitmap_class_entry, "swfbitmap", swfbitmap_functions);
  INIT_CLASS_ENTRY(text_class_entry, "swftext", swftext_functions);
  INIT_CLASS_ENTRY(textfield_class_entry, "swftextfield",
		   swftextfield_functions);
  INIT_CLASS_ENTRY(font_class_entry, "swffont", swffont_functions);
  INIT_CLASS_ENTRY(displayitem_class_entry, "swfdisplayitem",
		   swfdisplayitem_functions);
  INIT_CLASS_ENTRY(movie_class_entry, "swfmovie", swfmovie_functions);
  INIT_CLASS_ENTRY(button_class_entry, "swfbutton", swfbutton_functions);
  INIT_CLASS_ENTRY(action_class_entry, "swfaction", swfaction_functions);
  INIT_CLASS_ENTRY(morph_class_entry, "swfmorph", swfmorph_functions);
  INIT_CLASS_ENTRY(sprite_class_entry, "swfsprite", swfsprite_functions);

  zend_register_internal_class(&shape_class_entry);
  zend_register_internal_class(&fill_class_entry);
  zend_register_internal_class(&gradient_class_entry);
  zend_register_internal_class(&bitmap_class_entry);
  zend_register_internal_class(&text_class_entry);
  zend_register_internal_class(&textfield_class_entry);
  zend_register_internal_class(&font_class_entry);
  zend_register_internal_class(&displayitem_class_entry);
  zend_register_internal_class(&movie_class_entry);
  zend_register_internal_class(&button_class_entry);
  zend_register_internal_class(&action_class_entry);
  zend_register_internal_class(&morph_class_entry);
  zend_register_internal_class(&sprite_class_entry);

  return SUCCESS;
}

/* }}} */

#endif
