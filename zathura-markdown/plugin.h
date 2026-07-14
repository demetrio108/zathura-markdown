/* SPDX-License-Identifier: Zlib */

#ifndef MARKDOWN_H
#define MARKDOWN_H

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <zathura/plugin-api.h>
#include <mupdf/fitz.h>
#include <cairo.h>

typedef struct markdown_document_s {
  fz_context* ctx;       /**< Context */
  fz_document* document; /**< mupdf document (HTML rendered from markdown) */
  GMutex mutex;
} markdown_document_t;

typedef struct markdown_page_s {
  fz_page* page;       /**< Reference to the mupdf page */
  fz_context* ctx;     /**< Context */
  fz_stext_page* text; /**< Page text */
  fz_rect bbox;        /**< Bbox */
  bool extracted_text; /**< If text has already been extracted */
} markdown_page_t;

/**
 * Open a markdown document
 *
 * The markdown source is converted to HTML and handed to mupdf, which lays it
 * out into pages and renders it just like any other reflowable document.
 *
 * @param document Zathura document
 * @return true if no error occurred, otherwise false
 */
zathura_error_t markdown_document_open(zathura_document_t* document);

/**
 * Closes and frees the internal document structure
 *
 * @param document Zathura document
 * @return true if no error occurred, otherwise false
 */
zathura_error_t markdown_document_free(zathura_document_t* document, void* markdown_document);

/**
 * Generates the index of the document
 *
 * @param document Zathura document
 * @param error Set to an error value (see zathura_error_t) if an
 *   error occurred
 * @return Tree node object or NULL if an error occurred (e.g.: the document has
 *   no index)
 */
girara_tree_node_t* markdown_document_index_generate(zathura_document_t* document, void* markdown_document,
                                                     zathura_error_t* error);

/**
 * Returns a reference to a page
 *
 * @param page Page object
 * @return A page object or NULL if an error occurred
 */
zathura_error_t markdown_page_init(zathura_page_t* page);

/**
 * Frees a markdown page
 *
 * @param page Page
 * @return true if no error occurred, otherwise false
 */
zathura_error_t markdown_page_clear(zathura_page_t* page, void* markdown_page);

/**
 * Searches for a specific text on a page and returns a list of results
 *
 * @param page Page
 * @param text Search item
 * @param error Set to an error value (see zathura_error_t) if an
 *   error occurred
 * @return List of search results or NULL if an error occurred
 */
girara_list_t* markdown_page_search_text(zathura_page_t* page, void* markdown_page, const char* text,
                                         zathura_error_t* error);

/**
 * Returns a list of internal/external links that are shown on the given page
 *
 * @param page Page
 * @param error Set to an error value (see zathura_error_t) if an
 *   error occurred
 * @return List of links or NULL if an error occurred
 */
girara_list_t* markdown_page_links_get(zathura_page_t* page, void* markdown_page, zathura_error_t* error);

/**
 * Get text for selection
 * @param page Page
 * @param rectangle Selection
 * @error Set to an error value (see \ref zathura_error_t) if an error
 * occurred
 * @return The selected text (needs to be deallocated with g_free)
 */
char* markdown_page_get_text(zathura_page_t* page, void* markdown_page, zathura_rectangle_t rectangle,
                             zathura_error_t* error);

/**
 * Gets rectangles of highlighted text
 * @param page Page
 * @param rectangle Selection
 * @error Set to an error value (see \ref zathura_error_t) if an error
 * occurred
 * @return List of rectangles or NULL if an error occurred.
 */
girara_list_t* markdown_page_get_selection(zathura_page_t* page, void* markdown_page, zathura_rectangle_t rectangle,
                                           zathura_error_t* error);

/**
 * Renders a page onto a cairo object
 *
 * @param page Page
 * @param cairo Cairo object
 * @return  true if no error occurred, otherwise false
 */
zathura_error_t markdown_page_render_cairo(zathura_page_t* page, void* markdown_page, cairo_t* cairo, bool printing);

#endif // MARKDOWN_H
