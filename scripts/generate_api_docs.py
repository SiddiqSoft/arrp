#!/usr/bin/env python3
"""
generate_api_docs.py

Generates API reference documentation for {{PROJECT_NAME}} by:
1. Executing Doxygen using docs/Doxyfile to produce intermediate XML (docs/doxygen_xml/)
2. Parsing XML intermediate files to extract classes, methods, inheritance, and source lines.
3. Generating clean, structured Markdown C++ API reference pages for MkDocs Material.
4. Generating system-wide UML class diagram and Source Code Mapping table for overview & maintainer pages.
5. Generating focused, class-specific UML diagrams on individual class pages right below description.
6. Parameter folding on commas/types and filepath folding on right-most '/' for clean tabular layout.
7. Pure ASCII text with zero unicode symbols or emojis.
"""

import os
import sys
import re
import shutil
import subprocess
import xml.etree.ElementTree as ET
from pathlib import Path


def find_doxygen():
    """Locate doxygen executable in PATH or standard installation locations."""
    candidates = [
        shutil.which("doxygen"),
        "/opt/homebrew/bin/doxygen",
        "/usr/local/bin/doxygen",
        "/usr/bin/doxygen",
    ]
    for c in candidates:
        if c and Path(c).is_file() and os.access(c, os.X_OK):
            return str(c)
    return None


def run_doxygen(root_dir: Path) -> bool:
    """Run Doxygen to generate XML files in docs/doxygen_xml."""
    doxyfile = root_dir / "docs" / "Doxyfile"
    xml_dir = root_dir / "docs" / "doxygen_xml"

    doxy_bin = find_doxygen()
    if doxy_bin and doxyfile.exists():
        try:
            res = subprocess.run(
                [doxy_bin, str(doxyfile)],
                cwd=str(root_dir),
                capture_output=True,
                text=True,
                check=False,
            )
            if res.returncode == 0:
                print(f"[generate_api_docs] Doxygen XML successfully generated in {xml_dir}")
                return True
            else:
                print(f"[generate_api_docs] Warning: Doxygen exited with code {res.returncode}: {res.stderr.strip()[:200]}")
        except Exception as e:
            print(f"[generate_api_docs] Warning: Failed to execute doxygen: {e}")
    else:
        print("[generate_api_docs] Doxygen binary not found. Checking if existing XML files are present...")

    if xml_dir.exists() and any(xml_dir.glob("*.xml")):
        print(f"[generate_api_docs] Found existing XML files in {xml_dir}, proceeding with parse.")
        return True

    print("[generate_api_docs] Note: Doxygen XML not found; using project template baseline.")
    return False


def detect_project_info(root_dir: Path):
    default_branch = "main"
    """Detects project name and GitHub org from CMakeLists.txt or mkdocs.yml."""
    project_name = "{{PROJECT_NAME}}"
    github_org = "{{GITHUB_ORG}}"
    project_desc = "{{PROJECT_DESCRIPTION}}"

    cm_file = root_dir / "CMakeLists.txt"
    if cm_file.exists():
        cm_text = cm_file.read_text(encoding="utf-8")
        m = re.search(r"project\s*\(\s*([A-Za-z0-9_]+|\{\{[A-Za-z0-9_]+\}\})", cm_text)
        if m and not m.group(1).startswith("{{"):
            project_name = m.group(1)

    mkdocs_file = root_dir / "mkdocs.yml"
    if mkdocs_file.exists():
        mk_text = mkdocs_file.read_text(encoding="utf-8")
        m = re.search(r"repo_url:\s*[\"']?https://github\.com/([^/]+)/([^\s\n\"']+)", mk_text)
        if m:
            if not m.group(1).startswith("{{"):
                github_org = m.group(1)
            if not m.group(2).startswith("{{"):
                project_name = m.group(2)
        m_desc = re.search(r"site_description:\s*[\"']?([^\"'\n]+)", mk_text)
        if m_desc and not m_desc.group(1).startswith("{{"):
            project_desc = m_desc.group(1).strip()

    return project_name, github_org, project_desc, default_branch


def xml_text(elem) -> str:
    """Recursively extract plain text from XML element."""
    if elem is None:
        return ""
    return "".join(elem.itertext()).strip()


def fold_filepath_html(path: str) -> str:
    """Fold file path at right-most '/' separator for compact table rendering."""
    if "/" not in path:
        return f"<code>{path}</code>"
    r_idx = path.rfind("/")
    dir_part = path[: r_idx + 1]
    name_part = path[r_idx + 1 :]
    return f'<code><span class="filepath-dir">{dir_part}</span><wbr><span class="filepath-name">{name_part}</span></code>'


def split_params(args_str: str) -> list:
    """Split argument string into individual parameters, respecting template brackets and parentheses."""
    if not args_str:
        return []
    s = args_str.strip()
    if s.startswith("(") and s.endswith(")"):
        s = s[1:-1].strip()
    if not s or s == "void":
        return []

    params = []
    current = []
    depth_angle = 0
    depth_paren = 0
    depth_brace = 0

    for ch in s:
        if ch == "<":
            depth_angle += 1
            current.append(ch)
        elif ch == ">":
            depth_angle = max(0, depth_angle - 1)
            current.append(ch)
        elif ch == "(":
            depth_paren += 1
            current.append(ch)
        elif ch == ")":
            depth_paren = max(0, depth_paren - 1)
            current.append(ch)
        elif ch == "{":
            depth_brace += 1
            current.append(ch)
        elif ch == "}":
            depth_brace = max(0, depth_brace - 1)
            current.append(ch)
        elif ch == "," and depth_angle == 0 and depth_paren == 0 and depth_brace == 0:
            p = "".join(current).strip()
            if p:
                params.append(p)
            current = []
        else:
            current.append(ch)

    tail = "".join(current).strip()
    if tail:
        params.append(tail)
    return params



def extract_detailed_desc(detail_node, params_map=None) -> str:
    if params_map is None: params_map = {}
    if detail_node is None:
        return ""
        
    out = []
    
    # Simple recursive text extraction for mixed content
    def get_text(node):
        if node is None: return ""
        res = (node.text or "")
        for child in node:
            if child.tag == "ref":
                res += f"{child.text or ''}"
            elif child.tag == "computeroutput":
                res += f"<code>{get_text(child)}</code>"
            elif child.tag == "sp":
                res += " "
            else:
                res += get_text(child)
            res += (child.tail or "")
        return res

    for child in detail_node:
        if child.tag == "para":
            # Check if this para has a parameterlist or simplesect inside
            has_special = False
            for pchild in child:
                if pchild.tag in ("parameterlist", "simplesect", "programlisting"):
                    has_special = True
                    break
            
            if not has_special:
                txt = get_text(child).strip()
                if txt:
                    out.append(txt + "\n")
            else:
                # Handle mixed para content
                if child.text and child.text.strip():
                    out.append(child.text.strip() + "\n")
                    
                for pchild in child:
                    if pchild.tag == "parameterlist":
                        kind = pchild.get("kind", "")
                        if kind == "param":
                            out.append('<div class="memdoc-section-title">Parameters</div>\n')
                            out.append('<ul>')
                            for pitem in pchild.findall("parameteritem"):
                                name = get_text(pitem.find("parameternamelist/parametername"))
                                desc = get_text(pitem.find("parameterdescription/para"))
                                out.append(f'  <li><code>{name}</code> &mdash; {desc}</li>')
                            out.append('</ul>\n')
                        elif kind == "templateparam":
                            out.append('<div class="memdoc-section-title">Template Parameters</div>\n')
                            out.append('<table class="params" markdown="0">\n')
                            for pitem in pchild.findall("parameteritem"):
                                name = get_text(pitem.find("parameternamelist/parametername"))
                                desc = get_text(pitem.find("parameterdescription/para"))
                                out.append(f'  <tr>\n')
                                out.append(f'    <td class="paramtype"><code>typename</code></td>\n')
                                out.append(f'    <td class="paramname">{name}</td>\n')
                                out.append(f'    <td class="paramdesc">{desc}</td>\n')
                                out.append(f'  </tr>\n')
                            out.append('</table>\n')
                    elif pchild.tag == "simplesect":
                        kind = pchild.get("kind", "")
                        if kind == "return":
                            out.append('<div class="memdoc-section-title">Returns</div>\n')
                            out.append(get_text(pchild.find("para")) + "\n")
                        elif kind == "note":
                            out.append('<div class="memdoc-section-title">Note</div>\n')
                            out.append(get_text(pchild.find("para")) + "\n")
                        elif kind == "par":
                            title = get_text(pchild.find("title"))
                            out.append(f'<div class="memdoc-section-title">{title}</div>\n')
                            out.append(get_text(pchild.find("para")) + "\n")
                    elif pchild.tag == "programlisting":
                        filename = pchild.get("filename")
                        code = "\n".join(get_text(line) for line in pchild.findall("codeline"))
                        out.append(f"\n```cpp\n{code}\n```\n")
                        if filename:
                            out.append(f'<div class="mdesc">Source reference: <code>{filename}</code></div>\n')
                    else:
                        txt = get_text(pchild).strip()
                        if txt:
                            out.append(txt)
                        
                if child.tail and child.tail.strip():
                    out.append(child.tail.strip() + "\n")
        elif child.tag == "programlisting":
            filename = child.get("filename")
            code = "\n".join(get_text(line) for line in child.findall("codeline"))
            out.append(f"\n```cpp\n{code}\n```\n")
            if filename:
                out.append(f'<div class="mdesc">Source reference: <code>{filename}</code></div>\n')

    return "\n".join(out)

def escape_html(s: str) -> str:

    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;").replace('"', "&quot;")


def format_summary_params(name: str, anchor: str, args: str) -> str:
    """Format method name and folded parameters for API summary table cell."""
    params = split_params(args)
    if not params:
        return f'<a href="#{anchor}"><strong>{name}</strong></a> ()'

    if len(params) == 1 and len(params[0]) < 35:
        return f'<a href="#{anchor}"><strong>{name}</strong></a> ({escape_html(params[0])})'

    lines = [f'<a href="#{anchor}"><strong>{name}</strong></a> (']
    for i, p in enumerate(params):
        comma = "," if i < len(params) - 1 else ""
        escaped_p = escape_html(p)
        lines.append(f'<div class="param-wrap">{escaped_p}{comma}</div>')
    lines.append(")")
    return "".join(lines)


class ClassMeta:
    def __init__(self, name: str, refid: str = ""):
        self.name = name
        self.short_name = name.split("::")[-1]
        self.refid = refid
        self.brief = ""
        self.header_file = ""
        self.line = 1
        self.bases = []
        self.derived = []
        self.static_methods = []
        self.methods = []


def parse_classes_from_doxygen(xml_dir: Path) -> list:
    """Parses all class/struct definitions from Doxygen XML in xml_dir."""
    index_file = xml_dir / "index.xml"
    if not index_file.exists():
        return []

    classes = {}
    try:
        tree = ET.parse(index_file)
        root = tree.getroot()
        for comp in root.findall(".//compound"):
            kind = comp.get("kind")
            if kind in ("class", "struct"):
                cname = comp.findtext("name", "")
                if cname.startswith("std::") or "formatter<" in cname:
                    continue
                refid = comp.get("refid")
                cm = ClassMeta(cname, refid)
                classes[refid] = cm
    except Exception as e:
        print(f"[generate_api_docs] Error reading index.xml: {e}")
        return []

    for refid, cm in classes.items():
        c_xml = xml_dir / f"{refid}.xml"
        if not c_xml.exists():
            continue
        try:
            ctree = ET.parse(c_xml)
            cdef = ctree.find(".//compounddef")
            if cdef is None:
                continue

            cm.brief = xml_text(cdef.find("briefdescription"))
            loc = cdef.find("location")
            if loc is not None:
                cm.header_file = loc.get("file", "")
                try:
                    cm.line = int(loc.get("line", "1"))
                except ValueError:
                    cm.line = 1

            for b in cdef.findall("basecompoundref"):
                if b.text:
                    cm.bases.append(b.text.strip())

            for d in cdef.findall("derivedcompoundref"):
                if d.text:
                    cm.derived.append(d.text.strip())

            for m in cdef.findall(".//memberdef[@kind='function']"):
                prot = m.get("prot")
                if prot != "public":
                    continue
                is_static = m.get("static") == "yes"
                mname = m.findtext("name", "")
                mtype = xml_text(m.find("type"))
                args = m.findtext("argsstring", "()")
                mbrief = xml_text(m.find("briefdescription"))
                params_map = {}
                for p in m.findall("param"):
                    pname = p.findtext("declname")
                    ptype = xml_text(p.find("type"))
                    if pname and ptype:
                        params_map[pname] = ptype

                entry = {
                    "name": mname,
                    "id": m.get("id", mname.lower()),
                    "return_type": mtype or "void",
                    "args": args,
                    "brief": mbrief,
                    "detailed": extract_detailed_desc(m.find("detaileddescription"), params_map),
                    "static": is_static,
                }
                if is_static:
                    cm.static_methods.append(entry)
                else:
                    cm.methods.append(entry)
        except Exception as e:
            print(f"[generate_api_docs] Error reading {c_xml}: {e}")

    return list(classes.values())


def generate_class_uml_diagram(
    target_class: ClassMeta, project_name: str, github_org: str
) -> str:
    """
    Generates a focused, per-class Mermaid UML diagram displaying only the target class
    highlighted with its public methods and immediate inheritance/usage relationships.
    """
    base_src_url = f"https://github.com/{github_org}/{project_name}/blob/{default_branch}"
    clean_id = re.sub(r"[^A-Za-z0-9_]", "_", target_class.short_name)

    lines = [
        "```mermaid",
        "classDiagram",
        "    direction TB",
        "",
        "    classDef coreClass fill:rgba(35,73,109,0.08),stroke:#23496d,stroke-width:2px;",
        "    classDef exceptionClass fill:rgba(185,28,28,0.06),stroke:#b91c1c,stroke-width:1.5px;",
        "    classDef externalClass fill:rgba(100,116,139,0.06),stroke:#64748b,stroke-width:1.5px,stroke-dasharray: 4 3;",
        "    classDef highlightClass fill:rgba(2,132,199,0.18),stroke:#0284c7,stroke-width:3px;",
        "",
    ]

    for base in target_class.bases:
        base_id = re.sub(r"[^A-Za-z0-9_]", "_", base.split("::")[-1])
        lines.append(f'    class {base_id}["{base}"]')
        lines.append(f"    class {base_id}:::externalClass")

    lines.append(f'    class {clean_id}["{target_class.name}"] {{')
    for sm in target_class.static_methods[:4]:
        ret = f" {sm['return_type']}" if sm["return_type"] else ""
        lines.append(f"        +{sm['name']}{sm['args']}${ret}")
    for m in target_class.methods[:8]:
        ret = f" {m['return_type']}" if m["return_type"] else ""
        lines.append(f"        +{m['name']}{m['args']}{ret}")
    lines.append("    }")
    lines.append(f"    class {clean_id}:::highlightClass")
    lines.append("")

    for base in target_class.bases:
        base_id = re.sub(r"[^A-Za-z0-9_]", "_", base.split("::")[-1])
        lines.append(f"    {clean_id} --|> {base_id} : inherits")

    for drv in target_class.derived[:4]:
        drv_id = re.sub(r"[^A-Za-z0-9_]", "_", drv.split("::")[-1])
        lines.append(f'    class {drv_id}["{drv}"]')
        lines.append(f"    class {drv_id}:::coreClass")
        lines.append(f"    {drv_id} --|> {clean_id} : specializes")

    lines.append("")
    hdr = target_class.header_file or f"include/{root_namespace}/{project_name}.hpp"
    line_num = f"#L{target_class.line}" if target_class.line else ""
    lines.append(f'    link {clean_id} "{base_src_url}/{hdr}{line_num}" "Source: {hdr}"')
    for base in target_class.bases:
        base_id = re.sub(r"[^A-Za-z0-9_]", "_", base.split("::")[-1])
        if "std::" in base:
            lines.append(f'    link {base_id} "https://en.cppreference.com/" "Standard C++ Library"')
    lines.append("```")

    return "\n".join(lines)


def generate_system_uml_diagram(
    classes: list, project_name: str, github_org: str, default_branch: str = "main"
) -> str:
    """
    Generates a full-system Mermaid UML class diagram of all classes in the library.
    """
    base_src_url = f"https://github.com/{github_org}/{project_name}/blob/{default_branch}"

    lines = [
        "```mermaid",
        "classDiagram",
        "    direction TB",
        "",
        "    classDef coreClass fill:rgba(35,73,109,0.08),stroke:#23496d,stroke-width:2px;",
        "    classDef utilityClass fill:rgba(15,118,110,0.08),stroke:#0f766e,stroke-width:2px;",
        "    classDef exceptionClass fill:rgba(185,28,28,0.06),stroke:#b91c1c,stroke-width:1.5px;",
        "    classDef externalClass fill:rgba(100,116,139,0.06),stroke:#64748b,stroke-width:1.5px,stroke-dasharray: 4 3;",
        "    classDef highlightClass fill:rgba(2,132,199,0.18),stroke:#0284c7,stroke-width:3px;",
        "",
    ]

    all_bases = set()
    for cm in classes:
        for b in cm.bases:
            all_bases.add(b)

    for b in sorted(all_bases):
        base_id = re.sub(r"[^A-Za-z0-9_]", "_", b.split("::")[-1])
        lines.append(f'    class {base_id}["{b}"] {{')
        lines.append("        <<external base>>")
        lines.append("    }")
        lines.append(f"    class {base_id}:::externalClass")
        lines.append("")

    for cm in classes:
        clean_id = re.sub(r"[^A-Za-z0-9_]", "_", cm.short_name)
        cls_style = "exceptionClass" if "exception" in cm.short_name.lower() or "error" in cm.short_name.lower() else "coreClass"
        lines.append(f'    class {clean_id}["{cm.name}"] {{')
        for sm in cm.static_methods[:3]:
            ret = f" {sm['return_type']}" if sm["return_type"] else ""
            sanitized_args = re.sub(r'=[^,)]+', '', sm['args']).replace("<", "~").replace(">", "~").replace("{", "[").replace("}", "]")
            sanitized_ret = ret.replace("<", "~").replace(">", "~")
            clean_name = sm['name'].replace("operator=", "operator_assign")
            lines.append(f"        +{clean_name}{sanitized_args}${sanitized_ret}")
        for m in cm.methods[:5]:
            ret = f" {m['return_type']}" if m["return_type"] else ""
            sanitized_args = re.sub(r'=[^,)]+', '', m['args']).replace("<", "~").replace(">", "~").replace("{", "[").replace("}", "]")
            sanitized_ret = ret.replace("<", "~").replace(">", "~")
            clean_name = m['name'].replace("operator=", "operator_assign")
            lines.append(f"        +{clean_name}{sanitized_args}{sanitized_ret}")
        lines.append("    }")
        lines.append(f"    class {clean_id}:::{cls_style}")
        lines.append("")

    for cm in classes:
        clean_id = re.sub(r"[^A-Za-z0-9_]", "_", cm.short_name)
        for b in cm.bases:
            base_id = re.sub(r"[^A-Za-z0-9_]", "_", b.split("::")[-1])
            lines.append(f"    {clean_id} --|> {base_id} : inherits")

    lines.append("")
    for cm in classes:
        clean_id = re.sub(r"[^A-Za-z0-9_]", "_", cm.short_name)
        hdr = cm.header_file or f"include/{root_namespace}/{project_name}.hpp"
        line_num = f"#L{cm.line}" if cm.line else ""
        lines.append(f'    link {clean_id} "{base_src_url}/{hdr}{line_num}" "Source: {hdr}"')

    for b in sorted(all_bases):
        base_id = re.sub(r"[^A-Za-z0-9_]", "_", b.split("::")[-1])
        if "std::" in b:
            lines.append(f'    link {base_id} "https://en.cppreference.com/" "Standard C++ Library"')

    lines.append("```")
    return "\n".join(lines)


def generate_source_mapping_table(
    classes: list, project_name: str, github_org: str, default_branch: str = "main", api_prefix: str = ""
) -> str:
    """
    Generate markdown source code mapping table linking classes, headers,
    GitHub source files, and API documentation.
    """
    base_gh = f"https://github.com/{github_org}/{project_name}/blob/{default_branch}"

    lines = [
        "### Source Code Mapping",
        "",
        "| Component / Class | Header File | Source Link | Purpose & Architectural Role |",
        "| :--- | :--- | :--- | :--- |",
    ]

    for cm in classes:
        hdr = cm.header_file or f"include/{root_namespace}/{project_name}.hpp"
        line_num = f"#L{cm.line}" if cm.line else ""
        gh_link = f"[`{Path(hdr).name}`]({base_gh}/{hdr}{line_num})"
        doc_link = f"[`{cm.name}`]({api_prefix}{cm.short_name}.md)"
        brief = cm.brief or "Core component implementation."
        folded_hdr = fold_filepath_html(hdr)
        lines.append(f"| {doc_link} | {folded_hdr} | {gh_link} | {brief} |")

    return "\n".join(lines)



def get_cleaned_svg(html_dir, refid, aspect="coll"):
    svg_path = html_dir / f"{refid}__{aspect}__graph.svg"
    if not svg_path.exists():
        return ""
    try:
        svg = svg_path.read_text(encoding="utf-8")
        # Strip xml and doctype
        svg = re.sub(r'<\?xml[^>]*>', '', svg)
        svg = re.sub(r'<!DOCTYPE[^>]*>', '', svg)
        # Strip trailing newlines
        svg = svg.strip()
        if not svg: return ""
        aspect_title = "Collaboration" if aspect == "coll" else "Inheritance"
        return f'''<div class="uml-diagram-container graphviz-uml" data-graph-type="{aspect}">
<span class="uml-diagram-figure" style="display: block;">
<span class="uml-diagram-viewport" style="display: block;">
{svg}
</span>
<span class="uml-diagram-figcaption" style="display: block; text-align: center; font-style: italic; margin-top: 0.5em;">Figure: GraphViz UML {aspect_title} diagram</span>
</span>
</div>'''
    except Exception:
        return ""

def generate_class_markdown(
    cm: ClassMeta, project_name: str, github_org: str, root_namespace: str, default_branch: str = "main", html_dir = None
) -> str:
    """Generates complete reference markdown for a single class."""
    hdr = cm.header_file or f"include/{root_namespace}/{project_name}.hpp"
    base_gh = f"https://github.com/{github_org}/{project_name}/blob/{default_branch}"

    lines = [
        f"# {cm.name} Class Reference",
        "",
        '<div class="grid" markdown="1">',
        '<div class="api-intro-col" markdown="1">',
        '<div class="api-header-block">',
        '  <div class="api-module-name">Namespace {root_namespace}</div>',
        f'  <div class="api-header-file">#include &lt;{hdr.replace("include/", "")}&gt;</div>',
        '</div>',
        "",
        cm.brief or f"`{cm.short_name}` component of `{project_name}`.",
        "",
        '</div>',
        '<div class="api-diag-col" markdown="1">',
        "",
        "**Class Hierarchy & Inheritance**",
        "",
        f"The following UML class diagram highlights `{cm.name}` and its direct relationships. Click the node to navigate to its source file on GitHub.",
        "",
        f"<!-- @@uml-diag:{cm.short_name} -->",
        "",
        '</div>',
        '</div>',
        "",
    ]

    if cm.static_methods:
        lines.extend([
            "## Static Public Member Functions",
            "",
            '<table class="api-summary-table" markdown="1">',
        ])
        for sm in cm.static_methods:
            anchor = sm.get("id", sm["name"].lower())
            formatted_params = format_summary_params(sm["name"], anchor, sm["args"])
            ret_type = escape_html(sm["return_type"] or "void")
            brief_desc = sm["brief"] or "Static member function."
            lines.extend([
                "  <tr>",
                f'    <td class="memtype"><code>{ret_type}</code></td>',
                f'    <td class="memitemleft">{formatted_params}',
                f'      <div class="mdesc">{brief_desc}</div>',
                "    </td>",
                "  </tr>",
            ])
        lines.extend(["</table>", ""])

    constructors = []
    normal_methods = []
    for m in cm.methods:
        if m["name"] == cm.short_name or m["name"] == f"~{cm.short_name}":
            constructors.append(m)
        else:
            normal_methods.append(m)

    lines.extend([
        "## Member Functions Summary",
        "",
    ])

    if constructors:
        lines.extend([
            "### Constructors & Destructors",
            "",
            '<table class="api-summary-table" markdown="1">',
        ])
        for m in constructors:
            anchor = m.get("id", m["name"].lower())
            formatted_params = format_summary_params(m["name"], anchor, m["args"])
            ret_type = escape_html(m["return_type"] or "")
            if not ret_type:
                ret_html = ""
            else:
                ret_html = f'<code>{ret_type}</code>'
            brief_desc = m["brief"] or "Lifecycle method."
            lines.extend([
                "  <tr>",
                f'    <td class="memtype">{ret_html}</td>',
                f'    <td class="memitemleft">{formatted_params}',
                f'      <div class="mdesc">{brief_desc}</div>',
                "    </td>",
                "  </tr>",
            ])
        lines.extend(["</table>", ""])

    if normal_methods:
        lines.extend([
            "### Core Accessors & Modifiers",
            "",
            '<table class="api-summary-table" markdown="1">',
        ])
        for m in normal_methods:
            anchor = m.get("id", m["name"].lower())
            formatted_params = format_summary_params(m["name"], anchor, m["args"])
            ret_type = escape_html(m["return_type"] or "void")
            brief_desc = m["brief"] or "Member function."
            lines.extend([
                "  <tr>",
                f'    <td class="memtype"><code>{ret_type}</code></td>',
                f'    <td class="memitemleft">{formatted_params}',
                f'      <div class="mdesc">{brief_desc}</div>',
                "    </td>",
                "  </tr>",
            ])
        lines.extend(["</table>", ""])

    all_methods = cm.static_methods + cm.methods
    if all_methods:
        lines.extend([
            "## Member Function Documentation",
            "",
        ])
        for m in all_methods:
            anchor = m.get("id", m["name"].lower())
            qual = "static " if m.get("static") else ""
            ret_type = m["return_type"]
            args_str = m["args"]
            
            # Format the prototype block intelligently to handle multi-line params
            proto_args = args_str
            if "," in args_str and len(args_str) > 50:
                # Basic multi-line formatting for long args
                proto_args = args_str.replace("(", "(\n    ").replace(", ", ",\n    ").replace(")", "\n)")
                
            decl_str = f"{qual}{ret_type} {cm.short_name}::{m['name']}{proto_args};"
            if not ret_type:
                decl_str = f"{qual}{cm.short_name}::{m['name']}{proto_args};"
            else:
                decl_str = f"{qual}{ret_type} {cm.short_name}::{m['name']}{proto_args};"

            lines.extend([
                f'<div class="memitem" id="{anchor}" markdown="1">',
                '<div class="memitem-header">',
                '  <span class="memitem-diamond">&#9670;</span>',
                f'  <h4 class="memitem-title">{m["name"]}()</h4>',
                '</div>',
                '<div class="memproto" markdown="1">',
                "",
                "```cpp",
                decl_str,
                "```",
                "",
                "</div>",
                '<div class="memdoc" markdown="1">',
                "",
                (m.get("brief", "") + "\n" + m.get("detailed", "")).strip() or "Executes component operation.",
                "",
                "</div>",
                "</div>",
                "",
            ])

    lines.extend([
        "## Source Code Reference",
        "",
        f"- Header: [`{hdr}`]({base_gh}/{hdr}#L{cm.line})",
        "",
    ])

    return "\n".join(lines)


def generate_index_markdown(
    classes: list, project_name: str, github_org: str, project_desc: str, root_namespace: str, default_branch: str = "main"
) -> str:
    """Generates docs/api/index.md overview."""
    src_map = generate_source_mapping_table(
        classes, project_name, github_org, api_prefix=""
    )

    lines = [
        "# API Reference Overview",
        "",
        '<div class="api-header-block">',
        f'  <div class="api-module-name">{project_name} C++ Reference</div>',
        '  <div class="api-header-file">Generated from intermediate Doxygen XML</div>',
        '</div>',
        "",
        f"The `{root_namespace}` namespace provides data structures and utilities for `{project_name}`.",
        "",
        "## Classes & Structures",
        "",
        '<table class="api-summary-table" markdown="1">',
    ]

    for cm in classes:
        lines.extend([
            '  <tr>',
            '    <td class="memtype"><code>class</code></td>',
            f'    <td class="memitemleft"><a href="{cm.short_name}/"><strong>{root_namespace}::{cm.short_name}</strong></a><div class="mdesc">{cm.brief or f"Component of {project_name}"}</div></td>',
            '  </tr>'
        ])

    lines.extend([
        "</table>",
        "",
        "## Header Files",
        "",
        "| Header File | Include Path | Description |",
        "| :--- | :--- | :--- |",
    ])

    for cm in classes:
        hdr = cm.header_file or f"include/{root_namespace}/{project_name}.hpp"
        fname = hdr.split("/")[-1]
        brief = cm.brief or f'Definitions for {cm.short_name}'
        lines.append(f"| **`{fname}`** | `#include <{hdr.replace("include/", "")}>` | {brief} |")

    lines.extend([
        "",
        "## System UML Class Diagram",
        "",
        f"The following diagram illustrates the primary classes, inheritance, and relationships in `{project_name}`. Click any node to navigate to its GitHub source location:",
        "",
        "<!-- @@uml-diag:complete -->",
        "",
        src_map,
        "",
    ])
    return "\n".join(lines)


def update_maintainer_uml(maintainer_file: Path, classes: list, project_name: str, github_org: str, sys_uml: str, default_branch: str = "main"):
    """Injects or updates the UML class diagram and source code mapping in docs/maintainers/pipelines.md."""
    if not maintainer_file.exists():
        return
    text = maintainer_file.read_text(encoding="utf-8")
    section_title = "## Codebase Architecture & UML Class Diagram\n\n"
    lead_text = (
        f"The following UML class diagram illustrates the primary classes, relationships, and inheritance in `{project_name}`. "
        "The diagram is auto-generated from the C++ source AST via Doxygen XML. Each node in the diagram links directly to its source header file on GitHub.\n\n"
    )
    start_tag = "<!-- UML_CLASS_DIAGRAM_START -->"
    end_tag = "<!-- UML_CLASS_DIAGRAM_END -->"

    source_table = generate_source_mapping_table(
        classes, project_name, github_org, api_prefix="../api/"
    )
    combined_content = "The following UML class diagram illustrates the primary classes, relationships, and inheritance. The diagram is auto-generated from the C++ source AST via Doxygen XML. Each node in the diagram links directly to its source header file on GitHub.\n\n<!-- @@uml-diag:complete -->\n\n<!-- @@uml-diag:source-table -->"
    new_block = f"{start_tag}\n{combined_content}\n{end_tag}"

    if start_tag in text and end_tag in text:
        pattern = re.compile(rf"{re.escape(start_tag)}.*?{re.escape(end_tag)}", re.DOTALL)
        updated = pattern.sub(new_block, text)
    else:
        target = "## CMake Presets Matrix"
        if target in text:
            target_idx = text.find(target)
            updated = (
                text[:target_idx]
                + f"{section_title}{new_block}\n\n"
                + text[target_idx:]
            )
        else:
            updated = text + f"\n\n{section_title}{new_block}\n"

    if updated != text:
        maintainer_file.write_text(updated, encoding="utf-8")
        print(f"[generate_api_docs] Updated UML Class Diagram in {maintainer_file}")


def update_architecture_uml(
    arch_file: Path,
    classes: list,
    project_name: str,
    github_org: str,
    sys_uml: str,
    default_branch: str = "main",
):
    """Injects or updates the UML class diagram in docs/architecture/index.md."""
    if not arch_file.exists():
        return
    text = arch_file.read_text(encoding="utf-8")
    start_tag = "<!-- UML_CLASS_DIAGRAM_START -->"
    end_tag = "<!-- UML_CLASS_DIAGRAM_END -->"

    source_table = generate_source_mapping_table(
        classes, project_name, github_org, api_prefix="../api/"
    )
    combined_content = "<!-- @@uml-diag:complete -->\n\n<!-- @@uml-diag:source-table -->"
    new_block = f"{start_tag}\n{combined_content}\n{end_tag}"

    if start_tag in text and end_tag in text:
        pattern = re.compile(rf"{re.escape(start_tag)}.*?{re.escape(end_tag)}", re.DOTALL)
        updated = pattern.sub(new_block, text)
    else:
        updated = text + f"\n\n## UML Class Diagram\n\n{new_block}\n"

    if updated != text:
        arch_file.write_text(updated, encoding="utf-8")
        print(f"[generate_api_docs] Updated UML Class Diagram in {arch_file}")


def write_if_changed(file_path: Path, content: str):
    """Writes content to file_path only if it differs from current contents."""
    file_path.parent.mkdir(parents=True, exist_ok=True)
    if file_path.exists():
        old_content = file_path.read_text(encoding="utf-8")
        if old_content == content:
            return
    file_path.write_text(content, encoding="utf-8")
    print(f"[generate_api_docs] Wrote {file_path}")


def main():
    root_dir = Path(__file__).resolve().parent.parent
    project_name, github_org, project_desc, default_branch = detect_project_info(root_dir)
    root_namespace = github_org.lower()

    xml_dir = root_dir / "docs" / "doxygen_xml"
    run_doxygen(root_dir)

    classes = parse_classes_from_doxygen(xml_dir)

    # If no classes discovered from Doxygen XML (e.g. un-instantiated template or fresh clone),
    # construct the baseline class for {{PROJECT_NAME}}
    if not classes:
        default_cm = ClassMeta(f"{root_namespace}::{project_name}")
        default_cm.brief = project_desc
        default_cm.header_file = f"include/{root_namespace}/{project_name}.hpp"
        default_cm.line = 26
        default_cm.methods = [
            {
                "name": project_name,
                "return_type": "constexpr",
                "args": "() noexcept = default",
                "brief": "Default constructor.",
                "static": False,
            },
            {
                "name": f"~{project_name}",
                "return_type": "",
                "args": "() = default",
                "brief": "Default destructor.",
                "static": False,
            },
        ]
        classes = [default_cm]

    sys_uml = generate_system_uml_diagram(classes, project_name, github_org, default_branch)
    (root_dir / "docs" / "snippets" / "system_uml_diagram.md").parent.mkdir(parents=True, exist_ok=True)
    write_if_changed(root_dir / "docs" / "snippets" / "system_uml_diagram.md", sys_uml)

    # 1. Generate docs/api/index.md
    index_md = generate_index_markdown(
        classes, project_name, github_org, project_desc, root_namespace, default_branch
    )
    write_if_changed(root_dir / "docs" / "api" / "index.md", index_md)

    # 2. Generate per-class reference pages
    for cm in classes:
        html_dir = root_dir / "docs" / "doxygen_html"
        cls_md = generate_class_markdown(cm, project_name, github_org, root_namespace, default_branch, html_dir)
        write_if_changed(root_dir / "docs" / "api" / f"{cm.short_name}.md", cls_md)

    # 3. Update maintainer guide
    maintainer_file = root_dir / "docs" / "maintainers" / "maintainer_guide.md"
    if not maintainer_file.exists():
        maintainer_file = root_dir / "docs" / "maintainers" / "pipelines.md"
    update_maintainer_uml(
        maintainer_file,
        classes,
        project_name,
        github_org,
        sys_uml,
    )

    # 4. Update architecture guide if present
    update_architecture_uml(
        root_dir / "docs" / "architecture" / "index.md",
        classes,
        project_name,
        github_org,
        sys_uml,
    )

    print("[generate_api_docs] API documentation generation complete.")


if __name__ == "__main__":
    main()
