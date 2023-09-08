# Repository Structure

This document describes the filesystem layout of the Telodendria source
code repository.

- `Telodendria/`
    - `Cytoplasm/`: The source code for Cytoplasm, Telodendria's
    general-purpose support library that provides core functionality.
    - `contrib/`: Supplemental files, such as example configurations.
    - `docs/`: All user and developer documentation as Markdown.
    - `site/`: The official website source code as HTML.
    - `src/': The C source code and headers for Telodendria.
        - `Routes/`: Where the Matrix API endpoints are implemented.
        - `Static/`: Endpoints that just generate static HTML pages.
        - `include/`: Header files.
    - `tools/`: Development environment and tools.

