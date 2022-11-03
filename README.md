ansi2iwii
=========

Tool to convert ANSI escape sequences to those compatible with the Apple
ImageWriter II.

Example
-------

Example of a printout generated using `test.sh` and a color ribbon:
![Test print from test.sh](images/test_print.jpg)

Supported ANSI Escape Codes
---------------------------

Currently only a subset of SGR (Screen Graphic Rendition) ANSI escape sequences
(following the form of `ESC[<num>[...]m`) are supported:

| SGR   | Description               | IWII        | Notes                                                 |
|-------|---------------------------|-------------|-------------------------------------------------------|
| 1     | Bold                      | `ESC !`     |                                                       |
| 3     | Italic                    | `ESC w`     | Approximating using half height text on the IWII side |
| 4     | Underline                 | `ESC X`     |                                                       |
| 10    | Primary Font              | (multiple)  | Set via `--font`, defaults to Elite                   |
| 11-18 | Set Font                  | (multiple)  | See `--help` for details                              |
| 22    | Normal Intensity/Bold Off | `ESC "`     |                                                       |
| 23    | Italic Off                | `ESC W`     | See note on 3                                         |
| 24    | Underline Off             | `ESC Y`     |                                                       |
| 30-37 | Set Foreground Color      | (multiple)  | Cyan (6) represented by orange                        |
| 73    | Superscript               | `ESC x`     |                                                       |
| 74    | Subscript                 | `ESC y`     |                                                       |
| 75    | Superscript/Subscript Off | `ESC z`     |                                                       |
