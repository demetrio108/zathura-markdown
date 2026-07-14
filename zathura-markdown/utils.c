/* SPDX-License-Identifier: Zlib */

#include "utils.h"

void markdown_page_extract_text(markdown_document_t* markdown_document, markdown_page_t* markdown_page) {
  if (markdown_document == NULL || markdown_document->ctx == NULL || markdown_page == NULL ||
      markdown_page->text == NULL) {
    return;
  }

  fz_device* volatile text_device = NULL;

  fz_try(markdown_page->ctx) {
    fz_stext_options stext_options;
    stext_options.flags = FZ_STEXT_PRESERVE_IMAGES;
    text_device         = fz_new_stext_device(markdown_page->ctx, markdown_page->text, &stext_options);

    fz_run_page(markdown_page->ctx, markdown_page->page, text_device, fz_identity, NULL);
  }
  fz_always(markdown_document->ctx) {
    fz_close_device(markdown_page->ctx, text_device);
    fz_drop_device(markdown_page->ctx, text_device);
  }
  fz_catch(markdown_document->ctx) {}

  markdown_page->extracted_text = true;
}
