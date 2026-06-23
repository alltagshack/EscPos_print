# EscPos print


The project is an easy and simple tool for a direct access to a ESC/POS
58mm thermo printer on e.g. `/dev/usb/lp0`.

The tool prints 3 kinds of documents:

- png (scaled to max width)
- bmp (256 gray values, scaled to max width)
- md text UTF-8 files (Markdown)


For *Markdown* and UTF-8 the default `iconv.h` is used to convert it to CP858.
It is tested the german special chars. For *png* Images the libpng library is used.

![screenshot](examples/screenshot.png)

The **Atkinson dithering** is used for pictures.

## The limitted Markdown features

### Text

- UTF-8 text without BOM and `LF`
- a single newline is joined by a space
- 2 newlines caused into a line break
- 3 newlines caused into a new paragraph
- 2 or 3 spaces are shriked into a single one

### Headlines


Three versions of headlines via `#`, `##` and `###` are supported.

- a newline before and space behind the last `#` is needed
- the headline ends with 2 newlines

### Unordered Lists


Only an unordered list with the deep 1 is supported.

- starts with `-` and a single space
- a list ends with 2 newlines

### Font


A text (without newline) which is between these characters...

- grave accent: a bit smaller font `(condensed)`
- two asterisks: **bold**
- one asterisk: *underline and a bit bold*

### Not Supported

- Ordered Lists
- Blockquote or code blocks (a *to do*?)
- Link
- Image (a *to do* for png?)
- more then list deep 1 or othe kinds of headline formats
- horizontal rule (a *to do*?)

## Compile


```
cmake -B build
cmake --build build
```

## Usage


Use an example: `build/escposPrint examples/keyboard.png /dev/usb/lp0` or...

```
cd build
./escposPrint ./escposPrint <bmp, png or md file> <device>
```


It is possible, that you have to add yourself to the `lp` or `device` group first,
to get access to the printer device.

## Limited


The format support and maybe the font sizes varry on each device. I am
sorry about that. A 100% support is not guaranteed.

