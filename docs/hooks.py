import os
import re

def on_config(config, **kwargs):
    version = (
        os.getenv("GITVERSION_SEMVER")
        or os.getenv("GITVERSION_MAJORMINORPATCH")
        or os.getenv("BUILD_VERSION")
        or os.getenv("BUILD_BUILDNUMBER")
        or os.getenv("CI_BUILDID")
        or "0.0.0-dev"
    )
    config["extra"]["version"] = version
    print(f"[docs/hooks.py] Resolved build version: {version}")
    return config

def on_page_markdown(markdown, page, config, files):
    version = config.get("extra", {}).get("version", "0.0.0-dev")
    tag_ver = version.split("-")[0] if "-" in version else version
    semver  = tag_ver[1:] if tag_ver.startswith("v") else tag_ver
    if not tag_ver.startswith("v"):
        tag_ver = f"v{tag_ver}"

    placeholders = {
        "{{ version }}": version,
        "{ version }": version,
        "{{version}}": version,
        "{version}": version,
        "{{ tag_version }}": tag_ver,
        "{ tag_version }": tag_ver,
        "{{tag_version}}": tag_ver,
        "{tag_version}": tag_ver,
        "{{ semver }}": semver,
        "{ semver }": semver,
        "{{semver}}": semver,
        "{semver}": semver,
    }

    for p, val in placeholders.items():
        markdown = markdown.replace(p, val)

    return markdown

def fold_filepath_code(inner: str) -> str:
    if "filepath-dir" in inner or "filepath-name" in inner:
        return inner
    if "/" not in inner or inner.endswith("/"):
        return inner
    if inner.startswith(("/", "\"", "SIP/")):
        return inner

    if inner.startswith("#include") and "/" in inner:
        r_idx = inner.rfind("/")
        dir_part = inner[:r_idx + 1]
        name_part = inner[r_idx + 1:]
        if dir_part and name_part:
            return f'<span class="filepath-dir">{dir_part}</span><wbr><span class="filepath-name">{name_part}</span>'

    r_idx = inner.rfind("/")
    dir_part = inner[:r_idx + 1]
    name_part = inner[r_idx + 1:]
    if not name_part or not dir_part:
        return inner

    has_ext = bool(re.search(r"\.[a-zA-Z0-9_-]+>?$", name_part))
    has_multiple_dirs = dir_part.count("/") >= 1 and bool(re.search(r"[a-zA-Z0-9_.-]+/[a-zA-Z0-9_.-]+", inner))
    if not (has_ext or (has_multiple_dirs and len(inner) > 18)):
        return inner

    return f'<span class="filepath-dir">{dir_part}</span><wbr><span class="filepath-name">{name_part}</span>'

def on_page_content(html, page, config, files):
    def _replace_code(match):
        inner = match.group(1)
        folded = fold_filepath_code(inner)
        return f"<code>{folded}</code>"
    return re.sub(r"<code>([^<>\n]+)</code>", _replace_code, html)
