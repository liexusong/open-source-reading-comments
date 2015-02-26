/*
   +----------------------------------------------------------------------+
   | PHP HTML Embedded Scripting Language Version 3.0                     |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-1999 PHP Development Team (See Credits file)      |
   +----------------------------------------------------------------------+
   | This program is free software; you can redistribute it and/or modify |
   | it under the terms of one of the following licenses:                 |
   |                                                                      |
   |  A) the GNU General Public License as published by the Free Software |
   |     Foundation; either version 2 of the License, or (at your option) |
   |     any later version.                                               |
   |                                                                      |
   |  B) the PHP License as published by the PHP Development Team and     |
   |     included in the distribution in the file: LICENSE                |
   |                                                                      |
   | This program is distributed in the hope that it will be useful,      |
   | but WITHOUT ANY WARRANTY; without even the implied warranty of       |
   | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        |
   | GNU General Public License for more details.                         |
   |                                                                      |
   | You should have received a copy of both licenses referred to here.   |
   | If you did not, or have any questions about PHP licensing, please    |
   | contact core@php.net.                                                |
   +----------------------------------------------------------------------+
   | Authors: Uwe Steinmann                                               |
   +----------------------------------------------------------------------+
 */
/* $Id: php_cpdf.h,v 1.8 2001/03/30 20:36:18 ohrn Exp $ */

#ifndef PHP_CPDF_H
#define PHP_CPDF_H

#if HAVE_CPDFLIB

/* The macro T is defined in the IMAP headers and clashes with a function
   declaration here. Get rid of it. */

#ifdef T
#undef T
#endif

#include <cpdflib.h>

extern zend_module_entry cpdf_module_entry;
#define cpdf_module_ptr &cpdf_module_entry

extern PHP_MINFO_FUNCTION(cpdf);
extern PHP_MINIT_FUNCTION(cpdf);
extern PHP_RINIT_FUNCTION(cpdf);
extern PHP_MSHUTDOWN_FUNCTION(cpdf);
PHP_FUNCTION(cpdf_global_set_document_limits);
PHP_FUNCTION(cpdf_set_creator);
PHP_FUNCTION(cpdf_set_title);
PHP_FUNCTION(cpdf_set_subject);
PHP_FUNCTION(cpdf_set_keywords);
PHP_FUNCTION(cpdf_set_viewer_preferences);
PHP_FUNCTION(cpdf_open);
PHP_FUNCTION(cpdf_close);
PHP_FUNCTION(cpdf_page_init);
PHP_FUNCTION(cpdf_finalize_page);
PHP_FUNCTION(cpdf_set_current_page);
PHP_FUNCTION(cpdf_begin_text);
PHP_FUNCTION(cpdf_end_text);
PHP_FUNCTION(cpdf_show);
PHP_FUNCTION(cpdf_show_xy);
PHP_FUNCTION(cpdf_continue_text);
PHP_FUNCTION(cpdf_text);
PHP_FUNCTION(cpdf_set_font);
PHP_FUNCTION(cpdf_set_font_directories);
PHP_FUNCTION(cpdf_set_font_map_file);
PHP_FUNCTION(cpdf_set_leading);
PHP_FUNCTION(cpdf_set_text_rendering);
PHP_FUNCTION(cpdf_set_horiz_scaling);
PHP_FUNCTION(cpdf_set_text_rise);
PHP_FUNCTION(cpdf_set_text_matrix);
PHP_FUNCTION(cpdf_set_text_pos);
PHP_FUNCTION(cpdf_rotate_text);
PHP_FUNCTION(cpdf_set_char_spacing);
PHP_FUNCTION(cpdf_set_word_spacing);
PHP_FUNCTION(cpdf_continue_text);
PHP_FUNCTION(cpdf_stringwidth);
PHP_FUNCTION(cpdf_save);
PHP_FUNCTION(cpdf_restore);
PHP_FUNCTION(cpdf_translate);
PHP_FUNCTION(cpdf_scale);
PHP_FUNCTION(cpdf_rotate);
PHP_FUNCTION(cpdf_setflat);
PHP_FUNCTION(cpdf_setlinejoin);
PHP_FUNCTION(cpdf_setlinecap);
PHP_FUNCTION(cpdf_setmiterlimit);
PHP_FUNCTION(cpdf_setlinewidth);
PHP_FUNCTION(cpdf_setdash);
PHP_FUNCTION(cpdf_moveto);
PHP_FUNCTION(cpdf_rmoveto);
PHP_FUNCTION(cpdf_curveto);
PHP_FUNCTION(cpdf_lineto);
PHP_FUNCTION(cpdf_rlineto);
PHP_FUNCTION(cpdf_circle);
PHP_FUNCTION(cpdf_arc);
PHP_FUNCTION(cpdf_rect);
PHP_FUNCTION(cpdf_newpath);
PHP_FUNCTION(cpdf_closepath);
PHP_FUNCTION(cpdf_closepath_stroke);
PHP_FUNCTION(cpdf_stroke);
PHP_FUNCTION(cpdf_fill);
PHP_FUNCTION(cpdf_fill_stroke);
PHP_FUNCTION(cpdf_closepath_fill_stroke);
PHP_FUNCTION(cpdf_endpath);
PHP_FUNCTION(cpdf_clip);
PHP_FUNCTION(cpdf_setgray_fill);
PHP_FUNCTION(cpdf_setgray_stroke);
PHP_FUNCTION(cpdf_setgray);
PHP_FUNCTION(cpdf_setrgbcolor_fill);
PHP_FUNCTION(cpdf_setrgbcolor_stroke);
PHP_FUNCTION(cpdf_setrgbcolor);
PHP_FUNCTION(cpdf_add_outline);
PHP_FUNCTION(cpdf_set_page_animation);
PHP_FUNCTION(cpdf_finalize);
PHP_FUNCTION(cpdf_output_buffer);
PHP_FUNCTION(cpdf_save_to_file);
PHP_FUNCTION(cpdf_add_annotation);
PHP_FUNCTION(cpdf_import_jpeg);
#if HAVE_LIBGD13
PHP_FUNCTION(cpdf_place_inline_image);
#endif
PHP_FUNCTION(cpdf_set_action_url);
#else
#define cpdf_module_ptr NULL
#endif
#define phpext_cpdf_ptr cpdf_module_ptr
#endif /* _PHP_PDF_H */
