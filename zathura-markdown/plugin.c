/* SPDX-License-Identifier: Zlib */

#include "plugin.h"

ZATHURA_PLUGIN_REGISTER_WITH_FUNCTIONS("markdown", VERSION,
                                       ZATHURA_PLUGIN_FUNCTIONS({
                                           .document_open           = markdown_document_open,
                                           .document_free           = markdown_document_free,
                                           .document_index_generate = markdown_document_index_generate,
                                           .page_init               = markdown_page_init,
                                           .page_clear              = markdown_page_clear,
                                           .page_search_text        = markdown_page_search_text,
                                           .page_links_get          = markdown_page_links_get,
                                           .page_get_text           = markdown_page_get_text,
                                           .page_get_selection      = markdown_page_get_selection,
                                           .page_render_cairo       = markdown_page_render_cairo,
                                       }),
                                       ZATHURA_PLUGIN_MIMETYPES({
                                           /* text/x-markdown is an alias of text/markdown in shared-mime-info,
                                            * so registering the canonical type alone covers both. */
                                           "text/markdown",
                                           /* Markdown files usually contain plain prose, so both libmagic and
                                            * glib's content-based sniff report them as text/plain. Newer zathura
                                            * prefers that certain content-based guess over the uncertain
                                            * filename-based text/markdown guess, so we must claim text/plain too
                                            * or such files fail with "Could not determine file type." */
                                           "text/plain",
                                       }))
