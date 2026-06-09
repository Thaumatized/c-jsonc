# c-jsonc
c-jsonc is an JSONC parser implementation
What is JSONC? JSONC stands for JSON with Comments.
JSONC differs from JSON by allowing JavaScript style comments and trailing commas.
It is a superset of JSON and thus the parser can also parse JSON.
The stringifyer also outputs as JSON.

JSONC is unsuprisingly a name used for several projects.
Here it is specifically referring to https://github.com/JSONC-org/JSONC

## Compatibility
*For now* we only support ASCII charset, which is not inline with the JSON spec which requires utf-8.
This is ofcourse fine as long as the data does not include any unicode characters 
and should be okay even with unicode charactres if none of them can be confused for ASCII when looking at single bytes.

This implementation is also locale dependendt. By default C defaults to the POSIX locale which is okay.
The bit that matters for this implementation is the decimal separator, which must be a dot.

## Testing

> gcc test.c json.c -o test.bin && ./test.bin