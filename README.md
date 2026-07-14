zathura-markdown
================

zathura is a highly customizable and functional document viewer based on the girara user interface
library and several document libraries. This plugin for zathura provides Markdown support.

The Markdown source is converted to HTML with [cmark](https://github.com/commonmark/cmark) and the
resulting document is laid out and rendered by the `mupdf` library, following the same code pattern
as [zathura-pdf-mupdf](https://github.com/pwmt/zathura-pdf-mupdf).

Requirements
------------

The following dependencies are required:

* `zathura` (>= 2026.01.30)
* `girara`
* `mupdf` (>= 1.26)
* `libcmark` (>= 0.29)

For building plugin, the following dependencies are also required:

* `meson` (>= 1)

Installation
------------

To build and install the plugin using meson's ninja backend:

    meson build
    cd build
    ninja
    ninja install

> **Note:** The default backend for meson might vary based on the platform. Please
refer to the meson documentation for platform specific dependencies.

Usage
-----

Open any `.md` / `.markdown` file with zathura:

    zathura README.md

Bugs
----

Please report bugs at https://github.com/pwmt/zathura.
