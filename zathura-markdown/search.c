/* SPDX-License-Identifier: Zlib */

#define N_SEARCH_RESULTS 512

#include <glib.h>

#include "plugin.h"
#include "utils.h"

girara_list_t* markdown_page_search_text(zathura_page_t* page, void* data, const char* text, zathura_error_t* error) {
  if (page == NULL || text == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
    }
    goto error_ret;
  }

  markdown_page_t* markdown_page = data;
  zathura_document_t* document   = zathura_page_get_document(page);
  if (document == NULL || markdown_page == NULL || markdown_page->text == NULL) {
    goto error_ret;
  }

  markdown_document_t* markdown_document = zathura_document_get_data(document);

  girara_list_t* list = girara_list_new_with_free(g_free);
  if (list == NULL) {
    if (error != NULL) {
      *error = ZATHURA_ERROR_OUT_OF_MEMORY;
    }
    goto error_free;
  }

  g_mutex_lock(&markdown_document->mutex);

  /* extract text */
  if (markdown_page->extracted_text == false) {
    markdown_page_extract_text(markdown_document, markdown_page);
  }

  fz_quad* hit_bbox = fz_malloc_array(markdown_page->ctx, N_SEARCH_RESULTS, fz_quad);
  int num_results =
      fz_search_stext_page(markdown_page->ctx, markdown_page->text, text, NULL, hit_bbox, N_SEARCH_RESULTS);

  fz_rect r;
  for (int i = 0; i < num_results; i++) {
    zathura_rectangle_t* rectangle = g_malloc0(sizeof(zathura_rectangle_t));

    r             = fz_rect_from_quad(hit_bbox[i]);
    rectangle->x1 = r.x0;
    rectangle->x2 = r.x1;
    rectangle->y1 = r.y0;
    rectangle->y2 = r.y1;

    girara_list_append(list, rectangle);
  }

  fz_free(markdown_page->ctx, hit_bbox);
  g_mutex_unlock(&markdown_document->mutex);

  return list;

error_free:

  if (list != NULL) {
    girara_list_free(list);
  }

error_ret:

  if (error != NULL && *error == ZATHURA_ERROR_OK) {
    *error = ZATHURA_ERROR_UNKNOWN;
  }

  return NULL;
}
