# SiddiqSoft Documentation Guidelines

This document captures the "Golden Template" established by the `sip2json` project. All SiddiqSoft C++ repositories (such as `arrp` and `cxxtemplate`) must strictly adhere to these guidelines to ensure a clean, crisp, unified, and highly performant documentation experience.

## 1. Toolchain & Theme
*   **Generator**: [MkDocs Material](https://squidfunk.github.io/mkdocs-material/) must be used for all documentation generation.
*   **Configuration (`mkdocs.yml`)**:
    *   Must use the `material` theme.
    *   Must include the light/dark mode palette toggle (indigo primary/accent).
    *   Must use Roboto (text) and JetBrains Mono (code).
    *   Must enable specific Markdown extensions: `admonition`, `pymdownx.superfences` (with Mermaid support), `pymdownx.highlight`, `pymdownx.tabbed`, `pymdownx.details`, `md_in_html`, `attr_list`, and `toc` (permalink).
    *   Must actively exclude intermediate Doxygen and Markdown generation artifacts from the build via the `exclude_docs` configuration (e.g., `Doxyfile`, `doxygen_xml/*`, `doxygen_html/*`, `snippets/*`).
    *   Must include `docs/hooks.py` for advanced SVG GraphViz diagram injection.
*   **Logo & Favicon**: The repository must contain `docs/assets/Siddiq-Software-Avatar.png` and reference it in `mkdocs.yml` under `theme.logo` and `theme.favicon`.

## 2. Style & Styling (`custom.css`)
*   The `docs/css/custom.css` must precisely match the comprehensive stylesheet from `sip2json` (~1,200 lines).
*   **API Reference Design**: The API documentation must use the custom `.memitem`, `.memproto`, `.memdoc`, and `.memitem-title` CSS classes. It must render clean borders, distinctive diamond markers (`&#9670;`), and high-contrast grid layouts.
*   *Do NOT* invent new CSS classes for member functions or classes; stick to the exact Doxygen-mimicking HTML structure utilized in `sip2json`.

## 3. API Reference Generation (`generate_api_docs.py`)
*   API documentation must be auto-generated from Doxygen XML via `scripts/generate_api_docs.py`.
*   **Doxygen Settings**: The `docs/Doxyfile` must have `GENERATE_XML = YES`, `UML_LOOK = YES`, `CLASS_GRAPH = YES`, `COLLABORATION_GRAPH = YES`, `MACRO_EXPANSION = YES`, and evaluate relevant preprocessor macros (`PREDEFINED`).
*   **Class Diagrams**: Doxygen's generated GraphViz SVGs (`*_coll_graph.svg`) must be seamlessly injected into the class reference pages using `<!-- @@uml-diag:CLASSNAME -->` markers in the markdown, intercepted by `docs/hooks.py`.
*   **Member Function Formatting**: The script must extract the `<detaileddescription>` from Doxygen XML. This payload includes parameters (`<parameterlist>`), return values (`<simplesect kind="return">`), notes (`<simplesect kind="note">`), and code examples (`<programlisting>`).
*   **Code Examples**: Code examples parsed from Doxygen must be converted into standard fenced Markdown blocks (````cpp ... ````) inside the `.memdoc` section.

## 4. Documentation Structure & Layout
The `docs/` directory should follow this exact navigation hierarchy:
1.  **Home** (`index.md`): Clean landing page with feature badges, introduction, and high-level overview.
2.  **Getting Started / Quickstart** (`quickstart/index.md`): CMake integration instructions (`FetchContent`, dependencies).
3.  **API Reference** (`api/index.md` and `api/CLASSNAME.md`): 
    *   The `api/index.md` acts as an overview with a System UML Class Diagram (Mermaid) and Source Mapping Table.
    *   Individual class pages have a specific grid layout (`.api-intro-col`, `.api-diag-col`) showing the class hierarchy diagram alongside the header information, followed by "Member Functions Summary" and "Member Function Documentation".
4.  **Related Pages**:
    *   **Architecture & Design** (`architecture/index.md`): Deep dives into system design, async mechanics, or optimizations.
    *   **Maintainer Guide** (`maintainers/index.md`): Documentation on CI/CD pipelines, CMake presets, release lifecycle, etc.

## 5. Tone & Conciseness
*   **Crisp and Clear**: Avoid fluff. Be direct and technical.
*   **Rich but Readable**: Use tables, admonitions, and tabbed code blocks liberally.
*   **Repeatability**: Any updates to the documentation generation pipeline must be synchronized back to the `cxxtemplate` repository to benefit all projects.
