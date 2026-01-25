Small fun project, with my first functional interpreter (simple tree walker)

Supports `+`, `-`, `/`, `*`, `^` (pow), `<` (max), `>` (min), parenthesized expressions and constants - `e`, `pi`, `tau`

Quirk: no unary operators -> so instead of `-(2 + 2)` do `-1 * (2 + 2)` (no space after the `-`)

Usage example:

`./plot "x * 0.5"`

Range can be specified by using `-x` and `-y` flags

`./plot -x 10 -y 3 "x * 0.5"`

A/D (scancode) or LeftArrow/RightArrow keys can be used to move the cursor by one point at a time, F1 toggles the UI

Disclaimer: Constructing an ast from the expression is most likely not the ideal approach in this case, but its what I know :)
