/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2001 Zend Technologies Ltd. (http://www.zend.com) |
   +----------------------------------------------------------------------+
   | This source file is subject to version 0.92 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        | 
   | available at through the world-wide-web at                           |
   | http://www.zend.com/license/0_92.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: Andi Gutmans <andi@zend.com>                                |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/


#include "zend.h"
#include "zend_llist.h"

ZEND_API void zend_llist_init(zend_llist *l, size_t size, llist_dtor_func_t dtor, unsigned char persistent)
{
	l->head = NULL;
	l->tail = NULL;
	l->size = size;
	l->dtor = dtor;
	l->persistent = persistent;
}


ZEND_API void zend_llist_add_element(zend_llist *l, void *element)
{
	zend_llist_element *tmp = pemalloc(sizeof(zend_llist_element)+l->size-1, l->persistent);

	tmp->prev = l->tail;
	tmp->next = NULL;
	if (l->tail) {
		l->tail->next = tmp;
	} else {
		l->head = tmp;
	}
	l->tail = tmp;
	memcpy(tmp->data, element, l->size);
}


ZEND_API void zend_llist_prepend_element(zend_llist *l, void *element)
{
	zend_llist_element *tmp = pemalloc(sizeof(zend_llist_element)+l->size-1, l->persistent);

	tmp->next = l->head;
	tmp->prev = NULL;
	if (l->head) {
		l->head->prev = tmp;
	} else {
		l->tail = tmp;
	}
	l->head = tmp;
	memcpy(tmp->data, element, l->size);
}


#define DEL_LLIST_ELEMENT(current,l) \
			if (current->prev) {\
				current->prev->next = current->next;\
			} else {\
				l->head = current->next;\
			}\
			if (current->next) {\
				current->next->prev = current->prev;\
			} else {\
				l->tail = current->prev;\
			}\
			if (l->dtor) {\
				l->dtor(current->data);\
				pefree(current, l->persistent);\
			}


ZEND_API void zend_llist_del_element(zend_llist *l, void *element, int (*compare)(void *element1, void *element2))
{
	zend_llist_element *current=l->head;

	while (current) {
		if (compare(current->data, element)) {
			DEL_LLIST_ELEMENT(current,l);
			break;
		}
		current = current->next;
	}
}


ZEND_API void zend_llist_destroy(zend_llist *l)
{
	zend_llist_element *current=l->head, *next;
	
	while (current) {
		next = current->next;
		if (l->dtor) {
			l->dtor(current->data);
		}
		pefree(current, l->persistent);
		current = next;
	}
}


ZEND_API void zend_llist_clean(zend_llist *l)
{
	zend_llist_destroy(l);
	l->head = l->tail = NULL;
}


ZEND_API void zend_llist_remove_tail(zend_llist *l)
{
	zend_llist_element *old_tail;

	if ((old_tail = l->tail)) {
		if (l->tail->prev) {
			l->tail->prev->next = NULL;
		}
		l->tail = l->tail->prev;
		pefree(old_tail, l->persistent);
	}
}


ZEND_API void zend_llist_copy(zend_llist *dst, zend_llist *src)
{
	zend_llist_element *ptr;

	zend_llist_init(dst, src->size, src->dtor, src->persistent);
	ptr = src->head;
	while (ptr) {
		zend_llist_add_element(dst, ptr->data);
		ptr = ptr->next;
	}
}


ZEND_API void zend_llist_apply_with_del(zend_llist *l, int (*func)(void *data))
{
	zend_llist_element *element, *next;

	element=l->head;
	while (element) {
		next = element->next;
		if (func(element->data)) {
			DEL_LLIST_ELEMENT(element,l);
		}
		element = next;
	}
}


ZEND_API void zend_llist_apply(zend_llist *l, void (*func)(void *data))
{
	zend_llist_element *element;

	for (element=l->head; element; element=element->next) {
		func(element->data);
	}
}

ZEND_API void zend_llist_sort(zend_llist *l, llist_compare_func_t comp_func)
{
	int list_size=0, i;

	zend_llist_element **elements;
	zend_llist_element *element, **ptr;

	for (element=l->head; element; element=element->next) {
		list_size++;
	}

	if (list_size == 0) {
		return;
	}

	elements = (zend_llist_element **) emalloc(list_size*sizeof(zend_llist_element *));

	ptr = &elements[0];

	for (element=l->head; element; element=element->next) {
		*ptr++ = element;
	}

	qsort(elements, list_size, sizeof(zend_llist_element *), (int (*)(const void *, const void *)) comp_func);

	l->head = elements[0];
	elements[0]->prev = NULL;

	for (i=1; i<list_size; i++) {
		elements[i]->prev = elements[i-1];
		elements[i-1]->next = elements[i];
	}
	elements[i-1]->next = NULL;
	l->tail = elements[i-1];
	efree(elements);
}


ZEND_API void zend_llist_apply_with_argument(zend_llist *l, void (*func)(void *data, void *arg), void *arg)
{
	zend_llist_element *element;

	for (element=l->head; element; element=element->next) {
		func(element->data, arg);
	}
}


ZEND_API void zend_llist_apply_with_arguments(zend_llist *l, llist_apply_with_args_func_t func, int num_args, ...)
{
	zend_llist_element *element;
	va_list args;

	va_start(args, num_args);
	for (element=l->head; element; element=element->next) {
		func(element->data, num_args, args);
	}
	va_end(args);
}


ZEND_API int zend_llist_count(zend_llist *l)
{
	zend_llist_element *element;
	int element_count=0;

	for (element=l->head; element; element=element->next) {
		element_count++;
	}
	return element_count;
}


ZEND_API void *zend_llist_get_first_ex(zend_llist *l, zend_llist_position *pos)
{
	zend_llist_position *current = pos ? pos : &l->traverse_ptr;

	*current = l->head;
	if (*current) {
		return (*current)->data;
	} else {
		return NULL;
	}
}


ZEND_API void *zend_llist_get_last_ex(zend_llist *l, zend_llist_position *pos)
{
	zend_llist_position *current = pos ? pos : &l->traverse_ptr;

	*current = l->tail;
	if (*current) {
		return (*current)->data;
	} else {
		return NULL;
	}
}


ZEND_API void *zend_llist_get_next_ex(zend_llist *l, zend_llist_position *pos)
{
	zend_llist_position *current = pos ? pos : &l->traverse_ptr;

	if (*current) {
		*current = (*current)->next;
		if (*current) {
			return (*current)->data;
		}
	}
	return NULL;
}


ZEND_API void *zend_llist_get_prev_ex(zend_llist *l, zend_llist_position *pos)
{
	zend_llist_position *current = pos ? pos : &l->traverse_ptr;

	if (*current) {
		*current = (*current)->prev;
		if (*current) {
			return (*current)->data;
		}
	}
	return NULL;
}
