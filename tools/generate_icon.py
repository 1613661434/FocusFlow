"""Generate the multi-resolution Windows icon used by FocusFlow."""

from pathlib import Path

from PIL import Image, ImageDraw


def create_icon(size: int) -> Image.Image:
    oversample = 4
    scale = size * oversample / 64
    canvas_size = size * oversample
    image = Image.new("RGBA", (canvas_size, canvas_size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)

    inset = 2 * scale
    draw.rounded_rectangle(
        (inset, inset, canvas_size - inset, canvas_size - inset),
        radius=round(15 * scale),
        fill="#4F6EF7",
    )

    ring_box = (10 * scale, 10 * scale, 54 * scale, 54 * scale)
    draw.arc(
        ring_box,
        start=132,
        end=380,
        fill=(255, 255, 255, 108),
        width=round(5 * scale),
    )
    draw.line(
        [(17.5 * scale, 32.5 * scale), (27 * scale, 42 * scale), (47 * scale, 21 * scale)],
        fill="#FFFFFF",
        width=round(6 * scale),
        joint="curve",
    )

    return image.resize((size, size), Image.Resampling.LANCZOS)


def main() -> None:
    resources = Path(__file__).resolve().parents[1] / "resources" / "icons"
    resources.mkdir(parents=True, exist_ok=True)
    sizes = [16, 20, 24, 32, 40, 48, 64, 128, 256]
    images = [create_icon(size) for size in sizes]
    images[-1].save(resources / "focusflow.png", format="PNG")
    images[-1].save(
        resources / "focusflow.ico",
        format="ICO",
        append_images=images[:-1],
        sizes=[(size, size) for size in sizes],
    )


if __name__ == "__main__":
    main()
