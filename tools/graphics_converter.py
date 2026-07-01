from __future__ import annotations

from dataclasses import dataclass
from io import BytesIO
from pathlib import Path
import re
import shutil
import subprocess

try:
    from PIL import Image
except ImportError as exc:
    raise SystemExit("Pillow is required: python -m pip install Pillow") from exc


ROOT_DIR = Path(__file__).resolve().parents[1]
SOURCE_DIR = ROOT_DIR / "gfx" / "screens_240"
HEADER_PATH = ROOT_DIR / "includes" / "sprites.h"
SOURCE_PATH = ROOT_DIR / "src" / "sprites.cpp"

IMAGE_EXTENSIONS = {".jpg", ".jpeg", ".png"}


@dataclass(frozen=True)
class Sprite:
    path: Path
    symbol: str
    width: int
    height: int
    data: bytes

    @property
    def constant_prefix(self) -> str:
        return self.symbol.upper()


def image_files() -> list[Path]:
    return sorted(
        (
            path
            for path in SOURCE_DIR.iterdir()
            if path.is_file() and path.suffix.lower() in IMAGE_EXTENSIONS
        ),
        key=lambda path: path.name.lower(),
    )


def symbol_name(path: Path) -> str:
    filename = path.name
    if filename.lower().endswith(".fw.png"):
        base = filename[: -len(".fw.png")]
    else:
        base = path.stem

    base = re.sub(r"[^A-Za-z0-9_]", "_", base)
    base = re.sub(r"_+", "_", base).strip("_")
    if not base:
        base = "image"

    return f"sprite_{base}"


def stripped_png(path: Path) -> tuple[int, int, bytes]:
    with Image.open(path) as image:
        image.load()
        width, height = image.size
        output = BytesIO()

        image.save(output, format="PNG", optimize=True, compress_level=9)
        return width, height, output.getvalue()


def stripped_jpeg(path: Path) -> tuple[int, int, bytes]:
    with Image.open(path) as image:
        image.load()
        width, height = image.size
        output = BytesIO()

        try:
            image.save(
                output,
                format="JPEG",
                quality="keep",
                subsampling="keep",
                optimize=True,
            )
        except ValueError:
            output = BytesIO()
            image.convert("RGB").save(output, format="JPEG", quality=95, optimize=True)

        return width, height, output.getvalue()


def load_sprite(path: Path) -> Sprite:
    extension = path.suffix.lower()
    if extension == ".png":
        width, height, data = stripped_png(path)
    elif extension in {".jpg", ".jpeg"}:
        width, height, data = stripped_jpeg(path)
    else:
        raise ValueError(f"unsupported image type: {path}")

    if width > 0xFFFF or height > 0xFFFF:
        raise ValueError(f"{path} is too large for uint16_t dimensions")

    return Sprite(
        path=path,
        symbol=symbol_name(path),
        width=width,
        height=height,
        data=data,
    )


def format_byte_array(data: bytes) -> str:
    lines: list[str] = []

    for start in range(0, len(data), 16):
        chunk = data[start : start + 16]
        lines.append("    " + ", ".join(f"0x{value:02X}" for value in chunk) + ",")

    return "\n".join(lines)


def write_header(sprites: list[Sprite]) -> None:
    lines = [
        "#pragma once",
        "",
        "#include <stddef.h>",
        "#include <stdint.h>",
        "",
        "#if defined(ARDUINO)",
        "#include <pgmspace.h>",
        "#endif",
        "",
        "#ifndef PROGMEM",
        "#define PROGMEM",
        "#endif",
        "",
    ]

    for sprite in sprites:
        prefix = sprite.constant_prefix
        lines.extend(
            [
                f"#define {prefix}_WIDTH {sprite.width}u",
                f"#define {prefix}_HEIGHT {sprite.height}u",
                f"#define {prefix}_BYTES {len(sprite.data)}u",
                f"extern const uint8_t {sprite.symbol}[];",
                "",
            ]
        )

    HEADER_PATH.write_text("\n".join(lines), encoding="utf-8")


def write_source(sprites: list[Sprite]) -> None:
    lines = [
        '#include "sprites.h"',
        "",
    ]

    for sprite in sprites:
        lines.extend(
            [
                f"const uint8_t {sprite.symbol}[] PROGMEM = {{",
                format_byte_array(sprite.data),
                "};",
                "",
            ]
        )

    SOURCE_PATH.write_text("\n".join(lines), encoding="utf-8")


def format_source(path: Path) -> None:
    if not path.exists() or not shutil.which("clang-format"):
        return

    subprocess.run(
        ["clang-format", "-i", str(path)],
        cwd=ROOT_DIR,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        check=False,
    )


def main() -> None:
    if not SOURCE_DIR.is_dir():
        raise SystemExit(f"sprite source directory not found: {SOURCE_DIR}")

    sprites = [load_sprite(path) for path in image_files()]
    symbols = [sprite.symbol for sprite in sprites]

    duplicates = sorted({symbol for symbol in symbols if symbols.count(symbol) > 1})
    if duplicates:
        names = ", ".join(duplicates)
        raise SystemExit(f"duplicate sprite symbol name(s): {names}")

    write_header(sprites)
    write_source(sprites)
    format_source(SOURCE_PATH)

    print(f"generated {len(sprites)} sprite(s)")
    for sprite in sprites:
        print(
            f"{sprite.path.relative_to(ROOT_DIR)} -> {sprite.symbol} "
            f"{sprite.width}x{sprite.height}, {len(sprite.data)} bytes"
        )
    print(f"wrote {HEADER_PATH.relative_to(ROOT_DIR)}")
    print(f"wrote {SOURCE_PATH.relative_to(ROOT_DIR)}")


if __name__ == "__main__":
    main()
