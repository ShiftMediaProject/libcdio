/*
    $Id: ds.c,v 1.1 2004/12/18 17:29:32 rocky Exp $

    Copyright (C) 2000 Herbert Valerio Riedel <hvr@gnu.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include <cdio/ds.h>
#include <cdio/util.h>
#include <cdio/types.h>
#include "cdio_assert.h"

static const char _rcsid[] = "$Id: ds.c,v 1.1 2004/12/18 17:29:32 rocky Exp $";

struct _CdioList
{
  unsigned length;

  CdioListNode *begin;
  CdioListNode *end;
};

struct _CdioListNode
{
  CdioList *list;

  CdioListNode *next;

  void *data;
};

/* impl */

CdioList *
_cdio_list_new (void)
{
  CdioList *new_obj = _cdio_malloc (sizeof (CdioList));

  return new_obj;
}

void
_cdio_list_free (CdioList *list, int free_data)
{
  while (_cdio_list_length (list))
    _cdio_list_node_free (_cdio_list_begin (list), free_data);

  free (list);
}

unsigned
_cdio_list_length (const CdioList *list)
{
  cdio_assert (list != NULL);

  return list->length;
}

void
_cdio_list_prepend (CdioList *list, void *data)
{
  CdioListNode *new_node;

  cdio_assert (list != NULL);

  new_node = _cdio_malloc (sizeof (CdioListNode));
  
  new_node->list = list;
  new_node->next = list->begin;
  new_node->data = data;

  list->begin = new_node;
  if (list->length == 0)
    list->end = new_node;

  list->length++;
}

void
_cdio_list_append (CdioList *list, void *data)
{
  cdio_assert (list != NULL);

  if (list->length == 0)
    {
      _cdio_list_prepend (list, data);
    }
  else
    {
      CdioListNode *new_node = _cdio_malloc (sizeof (CdioListNode));
      
      new_node->list = list;
      new_node->next = NULL;
      new_node->data = data;

      list->end->next = new_node;
      list->end = new_node;

      list->length++;
    }
}

void 
_cdio_list_foreach (CdioList *list, _cdio_list_iterfunc func, void *user_data)
{
  CdioListNode *node;

  cdio_assert (list != NULL);
  cdio_assert (func != 0);
  
  for (node = _cdio_list_begin (list);
       node != NULL;
       node = _cdio_list_node_next (node))
    func (_cdio_list_node_data (node), user_data);
}

CdioListNode *
_cdio_list_find (CdioList *list, _cdio_list_iterfunc cmp_func, void *user_data)
{
  CdioListNode *node;

  cdio_assert (list != NULL);
  cdio_assert (cmp_func != 0);
  
  for (node = _cdio_list_begin (list);
       node != NULL;
       node = _cdio_list_node_next (node))
    if (cmp_func (_cdio_list_node_data (node), user_data))
      break;

  return node;
}

CdioListNode *
_cdio_list_begin (const CdioList *list)
{
  cdio_assert (list != NULL);

  return list->begin;
}

CdioListNode *
_cdio_list_end (CdioList *list)
{
  cdio_assert (list != NULL);

  return list->end;
}

CdioListNode *
_cdio_list_node_next (CdioListNode *node)
{
  if (node)
    return node->next;

  return NULL;
}

void 
_cdio_list_node_free (CdioListNode *node, int free_data)
{
  CdioList *list;
  CdioListNode *prev_node;

  cdio_assert (node != NULL);
  
  list = node->list;

  cdio_assert (_cdio_list_length (list) > 0);

  if (free_data)
    free (_cdio_list_node_data (node));

  if (_cdio_list_length (list) == 1)
    {
      cdio_assert (list->begin == list->end);

      list->end = list->begin = NULL;
      list->length = 0;
      free (node);
      return;
    }

  cdio_assert (list->begin != list->end);

  if (list->begin == node)
    {
      list->begin = node->next;
      free (node);
      list->length--;
      return;
    }

  for (prev_node = list->begin; prev_node->next; prev_node = prev_node->next)
    if (prev_node->next == node)
      break;

  cdio_assert (prev_node->next != NULL);

  if (list->end == node)
    list->end = prev_node;

  prev_node->next = node->next;

  list->length--;

  free (node);
}

void *
_cdio_list_node_data (CdioListNode *node)
{
  if (node)
    return node->data;

  return NULL;
}

/* eof */


/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */

