/* SPDX-License-Identifier: Zlib */

#include <glib.h>

#include "plugin.h"
#include "math.h"

girara_list_t* markdown_page_links_get(zathura_page_t* page, void* data, zathura_error_t* error) {
  if (page == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
    }
    goto error_ret;
  }

  markdown_page_t* markdown_page = data;
  zathura_document_t* document   = zathura_page_get_document(page);
  if (document == NULL || markdown_page == NULL || markdown_page->page == NULL) {
    goto error_ret;
  }

  markdown_document_t* markdown_document = zathura_document_get_data(document);

  girara_list_t* list = girara_list_new_with_free((girara_free_function_t)zathura_link_free);
  if (list == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_OUT_OF_MEMORY;
    }
    goto error_free;
  }

  g_mutex_lock(&markdown_document->mutex);

  fz_link* link_head = fz_load_links(markdown_document->ctx, markdown_page->page);
  for (fz_link* link = link_head; link != NULL; link = link->next) {
    /* extract position */
    zathura_rectangle_t position;
    position.x1 = link->rect.x0;
    position.x2 = link->rect.x1;
    position.y1 = link->rect.y0;
    position.y2 = link->rect.y1;

    zathura_link_type_t type     = ZATHURA_LINK_INVALID;
    zathura_link_target_t target = {ZATHURA_LINK_DESTINATION_UNKNOWN, NULL, 0, -1, -1, -1, -1, 0};

    if (fz_is_external_link(markdown_document->ctx, link->uri) == 1) {
      if (strstr(link->uri, "file://") == link->uri) {
        type         = ZATHURA_LINK_GOTO_REMOTE;
        target.value = link->uri;
      } else {
        type         = ZATHURA_LINK_URI;
        target.value = link->uri;
      }
    } else {
      float x = 0;
      float y = 0;

      fz_location location = fz_resolve_link(markdown_document->ctx, markdown_document->document, link->uri, &x, &y);

      type                    = ZATHURA_LINK_GOTO_DEST;
      target.destination_type = ZATHURA_LINK_DESTINATION_XYZ;
      target.page_number = fz_page_number_from_location(markdown_document->ctx, markdown_document->document, location);
      if (!isnan(x)) {
        target.left = x;
      }
      if (!isnan(y)) {
        target.top = y;
      }
      target.zoom = 0.0;
    }

    zathura_link_t* zathura_link = zathura_link_new(type, position, target);
    if (zathura_link != NULL) {
      girara_list_append(list, zathura_link);
    }
  }
  fz_drop_link(markdown_document->ctx, link_head);
  g_mutex_unlock(&markdown_document->mutex);

  return list;

error_free:

  if (list != NULL) {
    girara_list_free(list);
  }

error_ret:

  return NULL;
}
