/* SPDX-License-Identifier: Zlib */

#include <mupdf/fitz.h>

#include <glib-2.0/glib.h>
#include <cmark-gfm.h>
#include <cmark-gfm-core-extensions.h>

#include "plugin.h"
#include <girara/log.h>
#include <girara/utils.h>

/* Default page geometry used to lay out the reflowable HTML document. */
#define MARKDOWN_PAGE_WIDTH 600.0f
#define MARKDOWN_PAGE_HEIGHT 800.0f
#define MARKDOWN_PAGE_EM 11.0f

/* route mupdf warnings to the girara log instead of raw stderr */
static void mupdf_warning_callback(void* GIRARA_UNUSED(user), const char* message) {
  girara_debug("mupdf: %s", message);
}

/* route mupdf errors to the girara log instead of raw stderr */
static void mupdf_error_callback(void* GIRARA_UNUSED(user), const char* message) {
  girara_error("mupdf: %s", message);
}

/* Convert the markdown source to an HTML fragment using cmark's GitHub
 * Flavored Markdown extensions. The caller takes ownership of the returned
 * string (free with free). */
static char* markdown_to_html(const char* markdown, size_t length) {
  static const char* const extension_names[] = {"table", "strikethrough", "autolink", "tagfilter", "tasklist"};
  static gsize extensions_registered         = 0;

  if (g_once_init_enter(&extensions_registered)) {
    cmark_gfm_core_extensions_ensure_registered();
    g_once_init_leave(&extensions_registered, 1);
  }

  const int options       = CMARK_OPT_DEFAULT | CMARK_OPT_UNSAFE;
  cmark_mem* mem          = cmark_get_default_mem_allocator();
  cmark_parser* parser    = cmark_parser_new(options);
  cmark_llist* extensions = NULL;

  if (parser == NULL) {
    return NULL;
  }

  for (size_t i = 0; i < G_N_ELEMENTS(extension_names); i++) {
    cmark_syntax_extension* extension = cmark_find_syntax_extension(extension_names[i]);
    if (extension == NULL) {
      continue;
    }
    cmark_parser_attach_syntax_extension(parser, extension);
    extensions = cmark_llist_append(mem, extensions, extension);
  }

  cmark_parser_feed(parser, markdown, length);
  cmark_node* document = cmark_parser_finish(parser);
  if (document == NULL) {
    cmark_parser_free(parser);
    cmark_llist_free(mem, extensions);
    return NULL;
  }

  char* html = cmark_render_html(document, options, extensions);

  cmark_node_free(document);
  cmark_parser_free(parser);
  cmark_llist_free(mem, extensions);

  return html;
}

/* Wrap the HTML fragment produced by cmark in a minimal document with some
 * default styling so that mupdf lays it out in a readable fashion. The caller
 * takes ownership of the returned string (free with g_free). */
static char* markdown_wrap_html(const char* body) {
  static const char* header =
      "<!DOCTYPE html>\n"
      "<html>\n<head>\n<meta charset=\"utf-8\">\n<style>\n"
      "body { margin: 1em 1.5em; font-family: serif; line-height: 1.4; }\n"
      "h1, h2, h3, h4, h5, h6 { font-family: sans-serif; }\n"
      "pre, code { font-family: monospace; }\n"
      "pre { background: #f4f4f4; padding: 0.5em; }\n"
      "blockquote { margin-left: 1em; padding-left: 1em; border-left: 3px solid #ccc; color: #555; }\n"
      "img { max-width: 100%; }\n"
      "del { text-decoration: line-through; }\n"
      "table { border-collapse: collapse; }\n"
      "th, td { border: 1px solid #ccc; padding: 0.2em 0.5em; }\n"
      "</style>\n</head>\n<body>\n";
  static const char* footer = "\n</body>\n</html>\n";

  return g_strconcat(header, body, footer, NULL);
}

zathura_error_t markdown_document_open(zathura_document_t* document) {
  zathura_error_t error = ZATHURA_ERROR_OK;
  if (document == NULL) {
    error = ZATHURA_ERROR_INVALID_ARGUMENTS;
    goto error_ret;
  }

  markdown_document_t* markdown_document = calloc(1, sizeof(markdown_document_t));
  if (markdown_document == NULL) {
    error = ZATHURA_ERROR_OUT_OF_MEMORY;
    goto error_ret;
  }

  g_mutex_init(&markdown_document->mutex);

  markdown_document->ctx = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);
  if (markdown_document->ctx == NULL) {
    error = ZATHURA_ERROR_UNKNOWN;
    goto error_free;
  }

  fz_set_warning_callback(markdown_document->ctx, mupdf_warning_callback, NULL);
  fz_set_error_callback(markdown_document->ctx, mupdf_error_callback, NULL);

  /* read the markdown source from disk */
  const char* path = zathura_document_get_path(document);

  gchar* markdown = NULL;
  gsize markdown_length = 0;
  if (g_file_get_contents(path, &markdown, &markdown_length, NULL) == FALSE) {
    error = ZATHURA_ERROR_UNKNOWN;
    goto error_free;
  }

  /* convert markdown -> HTML fragment -> full HTML document */
  char* html_body = markdown_to_html(markdown, markdown_length);
  g_free(markdown);
  if (html_body == NULL) {
    error = ZATHURA_ERROR_UNKNOWN;
    goto error_free;
  }

  char* html = markdown_wrap_html(html_body);
  free(html_body);

  /* hand the HTML off to mupdf */
  fz_buffer* volatile buffer = NULL;
  fz_try(markdown_document->ctx) {
    fz_register_document_handlers(markdown_document->ctx);

    buffer = fz_new_buffer_from_copied_data(markdown_document->ctx, (const unsigned char*)html, strlen(html));
    /* "text/html" makes mupdf use its lenient HTML5 parser, which handles the
     * HTML that cmark emits without the strict-XHTML syntax warnings. */
    markdown_document->document = fz_open_document_with_buffer(markdown_document->ctx, "text/html", buffer);
  }
  fz_always(markdown_document->ctx) {
    fz_drop_buffer(markdown_document->ctx, buffer);
  }
  fz_catch(markdown_document->ctx) {
    g_free(html);
    error = ZATHURA_ERROR_UNKNOWN;
    goto error_free;
  }
  g_free(html);

  if (markdown_document->document == NULL) {
    error = ZATHURA_ERROR_UNKNOWN;
    goto error_free;
  }

  /* lay out the reflowable document and count its pages */
  fz_try(markdown_document->ctx) {
    fz_layout_document(markdown_document->ctx, markdown_document->document, MARKDOWN_PAGE_WIDTH, MARKDOWN_PAGE_HEIGHT,
                       MARKDOWN_PAGE_EM);
    zathura_document_set_number_of_pages(document,
                                         fz_count_pages(markdown_document->ctx, markdown_document->document));
  }
  fz_catch(markdown_document->ctx) {
    error = ZATHURA_ERROR_UNKNOWN;
    goto error_free;
  }

  zathura_document_set_data(document, markdown_document);

  return ZATHURA_ERROR_OK;

error_free:

  if (markdown_document != NULL) {
    g_mutex_clear(&markdown_document->mutex);
    if (markdown_document->document != NULL) {
      fz_drop_document(markdown_document->ctx, markdown_document->document);
    }
    if (markdown_document->ctx != NULL) {
      fz_drop_context(markdown_document->ctx);
    }

    free(markdown_document);
  }

  zathura_document_set_data(document, NULL);

error_ret:

  return error;
}

zathura_error_t markdown_document_free(zathura_document_t* document, void* data) {
  markdown_document_t* markdown_document = data;

  if (document == NULL || markdown_document == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  g_mutex_lock(&markdown_document->mutex);

  fz_drop_document(markdown_document->ctx, markdown_document->document);
  fz_drop_context(markdown_document->ctx);

  g_mutex_unlock(&markdown_document->mutex);
  g_mutex_clear(&markdown_document->mutex);

  free(markdown_document);
  zathura_document_set_data(document, NULL);

  return ZATHURA_ERROR_OK;
}
