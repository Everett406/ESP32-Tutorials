#!/usr/bin/env python3
"""
ESP32 教程站点构建脚本

功能：
1. 扫描仓库中的教程内容
2. 把 README.md 和 .ino 文件复制到 docs/content/
3. 生成 docs/content/index.json 索引文件

运行方式：
    python docs/build.py
"""

import os
import json
import shutil
from pathlib import Path

# 仓库根目录
ROOT = Path(__file__).parent.parent
DOCS = ROOT / "docs"
CONTENT = DOCS / "content"


def clean_content():
    """清空 content 目录"""
    if CONTENT.exists():
        shutil.rmtree(CONTENT)
    CONTENT.mkdir(parents=True)


def copy_file(src: Path, dst: Path):
    """复制文件，保持相对路径"""
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)


def extract_title_from_md(content: str, default: str) -> str:
    """从 Markdown 内容中提取第一个 # 标题"""
    for line in content.splitlines():
        line = line.strip()
        if line.startswith("# "):
            return line[2:].strip()
    return default


def extract_title_from_ino(content: str, default: str) -> str:
    """从 .ino 文件顶部注释中提取标题"""
    for line in content.splitlines()[:30]:
        line = line.strip().lstrip("/*").lstrip("*").strip()
        if "ESP32" in line and ("教程" in line or "项目" in line or "示例" in line):
            return line
    return default


def scan_series(series_dir: Path, category: str) -> list:
    """扫描一个教程系列目录"""
    items = []

    if not series_dir.exists():
        return items

    # 遍历子目录，找 .ino 文件
    for subdir in sorted(series_dir.iterdir()):
        if not subdir.is_dir():
            continue

        ino_files = list(subdir.glob("*.ino"))
        if not ino_files:
            continue

        ino_file = ino_files[0]
        readme_file = subdir / "README.md"

        # 优先复制 README.md，如果没有就复制 .ino
        if readme_file.exists():
            src = readme_file
            rel_path = readme_file.relative_to(ROOT)
        else:
            src = ino_file
            rel_path = ino_file.relative_to(ROOT)

        dst = CONTENT / rel_path
        copy_file(src, dst)

        # 读取标题
        content = src.read_text(encoding="utf-8")
        if src.suffix == ".md":
            title = extract_title_from_md(content, subdir.name)
        else:
            title = extract_title_from_ino(content, subdir.name)

        # 生成副标题
        if category == "arduino":
            subtitle = "Arduino 教程"
        elif category == "c":
            subtitle = "C 语言教程"
        else:
            subtitle = "综合项目"

        items.append({
            "path": str(rel_path).replace("\\", "/"),
            "title": title,
            "subtitle": subtitle,
            "category": category
        })

    return items


def scan_root_readme() -> list:
    """扫描顶层 README"""
    items = []
    readme = ROOT / "README.md"
    if readme.exists():
        dst = CONTENT / "README.md"
        copy_file(readme, dst)
        content = readme.read_text(encoding="utf-8")
        title = extract_title_from_md(content, "仓库说明")
        items.append({
            "path": "README.md",
            "title": title,
            "subtitle": "仓库首页",
            "category": "root"
        })
    return items


def scan_special_dirs():
    """扫描 extensions 和 peripherals 下的子目录"""
    items = []

    special_dirs = [
        (ROOT / "ESP32_Tutorial_Series" / "extensions", "arduino", "扩展项目"),
        (ROOT / "ESP32_Tutorial_Series" / "peripherals", "arduino", "外设项目"),
    ]

    for base_dir, category, subtitle_prefix in special_dirs:
        if not base_dir.exists():
            continue

        for subdir in sorted(base_dir.iterdir()):
            if not subdir.is_dir():
                continue

            ino_files = list(subdir.glob("*.ino"))
            if not ino_files:
                continue

            ino_file = ino_files[0]
            readme_file = subdir / "README.md"

            if readme_file.exists():
                src = readme_file
                rel_path = readme_file.relative_to(ROOT)
            else:
                src = ino_file
                rel_path = ino_file.relative_to(ROOT)

            dst = CONTENT / rel_path
            copy_file(src, dst)

            content = src.read_text(encoding="utf-8")
            if src.suffix == ".md":
                title = extract_title_from_md(content, subdir.name)
            else:
                title = extract_title_from_ino(content, subdir.name)

            items.append({
                "path": str(rel_path).replace("\\", "/"),
                "title": title,
                "subtitle": f"{subtitle_prefix}",
                "category": category
            })

    return items


def scan_series_top_readme(series_dir: Path, category: str, subtitle: str) -> list:
    """扫描教程系列顶层 README.md"""
    items = []
    readme = series_dir / "README.md"

    if readme.exists():
        rel_path = readme.relative_to(ROOT)
        dst = CONTENT / rel_path
        copy_file(readme, dst)

        content = readme.read_text(encoding="utf-8")
        title = extract_title_from_md(content, series_dir.name)

        items.append({
            "path": str(rel_path).replace("\\", "/"),
            "title": title,
            "subtitle": subtitle,
            "category": category
        })

    return items


def main():
    print("开始构建站点内容...")
    clean_content()

    index = {
        "root": scan_root_readme(),
        "arduino": [],
        "c": [],
        "projects": []
    }

    # 扫描 Arduino 教程
    print("扫描 Arduino 教程...")
    arduino_dir = ROOT / "ESP32_Tutorial_Series"
    index["arduino"].extend(scan_series_top_readme(arduino_dir, "arduino", "Arduino 教程说明"))
    index["arduino"].extend(scan_series(arduino_dir, "arduino"))

    # 扫描扩展和外设项目
    print("扫描扩展和外设项目...")
    extensions_dir = arduino_dir / "extensions"
    peripherals_dir = arduino_dir / "peripherals"
    index["arduino"].extend(scan_series_top_readme(extensions_dir, "arduino", "扩展项目说明"))
    index["arduino"].extend(scan_series_top_readme(peripherals_dir, "arduino", "外设项目说明"))
    index["arduino"].extend(scan_special_dirs())

    # 扫描 C 语言教程
    print("扫描 C 语言教程...")
    c_dir = ROOT / "ESP32_C_Tutorial"
    index["c"].extend(scan_series_top_readme(c_dir, "c", "C 语言教程说明"))
    index["c"].extend(scan_series(c_dir, "c"))

    # 扫描综合项目
    print("扫描综合项目...")
    proj_dir = ROOT / "ESP32_Projects"
    index["projects"].extend(scan_series_top_readme(proj_dir, "projects", "综合项目说明"))
    index["projects"].extend(scan_series(proj_dir, "projects"))

    # 写入索引
    index_file = CONTENT / "index.json"
    with open(index_file, "w", encoding="utf-8") as f:
        json.dump(index, f, ensure_ascii=False, indent=2)

    # 统计
    total = len(index["root"]) + len(index["arduino"]) + len(index["c"]) + len(index["projects"])
    print(f"构建完成！共 {total} 个内容文件")
    print(f"  - 仓库说明：{len(index['root'])}")
    print(f"  - Arduino 教程：{len(index['arduino'])}")
    print(f"  - C 语言教程：{len(index['c'])}")
    print(f"  - 综合项目：{len(index['projects'])}")


if __name__ == "__main__":
    main()
