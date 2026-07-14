/* SPDX-License-Identifier: Zlib */

#include "plugin.h"

zathura_error_t markdown_page_init(zathura_page_t* page) {
  if (page == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_document_t* document           = zathura_page_get_document(page);
  markdown_document_t* markdown_document = zathura_document_get_data(document);
  markdown_page_t* markdown_page         = calloc(1, sizeof(markdown_page_t));
  unsigned int index                     = zathura_page_get_index(page);

  if (markdown_page == NULL) {
    return ZATHURA_ERROR_OUT_OF_MEMORY;
  }

  g_mutex_lock(&markdown_document->mutex);
  markdown_page->ctx = markdown_document->ctx;
  if (markdown_page->ctx == NULL) {
    goto error_free;
  }

  /* load page */
  fz_try(markdown_page->ctx) {
    markdown_page->page = fz_load_page(markdown_document->ctx, markdown_document->document, index);
    markdown_page->bbox = fz_bound_page(markdown_document->ctx, (fz_page*)markdown_page->page);
    markdown_page->text = fz_new_stext_page(markdown_page->ctx, markdown_page->bbox);
  }
  fz_catch(markdown_page->ctx) {
    goto error_free;
  }

  markdown_page->extracted_text = false;

  if (markdown_page->text == NULL) {
    goto error_free;
  }
  g_mutex_unlock(&markdown_document->mutex);

  zathura_page_set_data(page, markdown_page);

  /* get page dimensions */
  zathura_page_set_width(page, markdown_page->bbox.x1 - markdown_page->bbox.x0);
  zathura_page_set_height(page, markdown_page->bbox.y1 - markdown_page->bbox.y0);

  return ZATHURA_ERROR_OK;

error_free:
  g_mutex_unlock(&markdown_document->mutex);

  markdown_page_clear(page, markdown_page);

  return ZATHURA_ERROR_UNKNOWN;
}

zathura_error_t markdown_page_clear(zathura_page_t* page, void* data) {
  if (page == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  markdown_page_t* markdown_page         = data;
  zathura_document_t* document           = zathura_page_get_document(page);
  markdown_document_t* markdown_document = zathura_document_get_data(document);

  g_mutex_lock(&markdown_document->mutex);
  if (markdown_page != NULL) {
    if (markdown_page->text != NULL) {
      fz_drop_stext_page(markdown_page->ctx, markdown_page->text);
    }

    if (markdown_page->page != NULL) {
      fz_drop_page(markdown_document->ctx, markdown_page->page);
    }

    free(markdown_page);
  }
  g_mutex_unlock(&markdown_document->mutex);

  return ZATHURA_ERROR_OK;
}
