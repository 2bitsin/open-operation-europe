/* OPEN 02a0:0002..07c1 (image 0x2a02..0x31c2): keyboard read, the formatter
 * and the Japanese text console of the opening program. It is the OPEN build
 * of MAIN's TEXTOUT (0480, src/main/core/textout.c): input_flush,
 * format_decimal, put_unsigned, put_long, text_clamp, army_position,
 * text_set_x/y, text_color, text_paper_set, text_control, cursor_print,
 * text_put_range and text_erase_char keep MAIN's C text. The two bytes at
 * 02a0:0000 end the preceding assembly segment (028d).
 *
 * Differences from MAIN: key_read returns the character without the keypad
 * mapping; opeu_format passes every output character to a far callback
 * (opeu_text_put for cursor_print, opeu_format_store for request_format)
 * instead of buffering 200 bytes, and ignores %l numbers (put_long is
 * linked but no longer called); there is no word-wrapping text, scroll or
 * window state; lines are 16 pixels, 32 in large mode; glyph drawing is an
 * empty routine; Shift-JIS lead bytes (80-9f, e0-fc) are buffered and the
 * pair is converted to JIS X 0208 (opeu_open_sjis_to_jis); ESC Y n selects
 * large mode (n == '1'); ESC S, ESC E and ESC W call the empty stubs
 * 062d:0416/0418/041a (they take the place of MAIN's 0541:011e, sound reset
 * and pending finish).
 *
 * DGROUP: DS:015a x (0..79), 015c y (pixel row 0..399), 015e left column,
 * 0160 paper, 0161 ink, 0162 background, 0164 large mode, 0172 pending
 * lead byte, 0174 escape-sequence state, 0176 the destination of
 * opeu_request_format, 0142 the erase string.
 *
 * Escape sequences (ESC = 1bh) understood by opeu_text_put:
 *   ESC = row col  cursor y (row-20h)*32, x col-20h
 *   ESC C n  ink n-'0'     ESC M n  paper n-'0'    ESC B n  background n-'0'
 *   ESC R n  left n-20h    ESC Y n  large mode     ESC T    clear to line end
 *   ESC *    clear screen  ESC S    stub a(1)
 *   ESC E n  stub b(6), then stub c(n-'0')    ESC W n  stub c(n-'0')
 * Formatter directives: %[-][+][0][width][.precision][l](d|u|x|o|s|c), case
 * insensitive; any other character after % is copied literally.
 */
#include "types.hpp"
/* Host model: DS is explicit and near pointers are 16-bit DS offsets. The
 * original's automatic conversion buffer lives on the DGROUP stack (SS =
 * DS); the host allocates it below host->sp so that callees receive DS
 * offsets too. Variadic entries receive the DS offset of their argument
 * words, which is the original stack layout after the return address.
 * Services outside this unit are events: 0 keyboard read (INT 16h AH=0,
 * returns AX), 1 keyboard poll (0000:13d1), 2 stub 062d:0416(value),
 * 3 box (x,y,w,h,color) 0000:0939, 4 screen clear 0000:0913, 5 stub
 * 062d:0418(value), 6 stub 062d:041a(value). STRINGS helpers execute as
 * recovered code. */
typedef uint16_t (*TextEvent)(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count);
typedef struct { TextEvent event; uint16_t sp; } TextHost;
typedef void (*TextOut)(uint8_t *ds, TextHost *host, uint16_t c);
extern uint16_t opeu_string_length(uint8_t ds[65536], uint16_t string);
extern uint8_t opeu_ascii_upper(uint8_t code);
#define STATE uint8_t ds[65536] __attribute__((unused)), TextHost *host __attribute__((unused)),
#define STATE0 uint8_t ds[65536], TextHost *host __attribute__((unused))
#define ARGS ds, host,
#define ARGS0 ds, host
#define CDECL
#define I16 int16_t
#define I32 int32_t
#define PCHAR uint16_t
#define PBYTE uint16_t
#define PPCHAR uint16_t *
#define PARGS uint16_t
#define OUT_FN(name) TextOut name
#define OUT(name, v) name(ds, host, (uint16_t)(v))
#define BUFFER(type, name, size) uint16_t name
#define ENTER(name, size) name = (uint16_t)(host->sp -= (size));
#define LEAVE(size) host->sp += (size);
#define UNUSED(v) (void)(v);
#define AT(p) ds[(uint16_t)(p)]
#define U16(e) ((uint16_t)(e))
#define NEGATE_LONG(e) ((int32_t)(0u-(uint32_t)(e)))
#define WORD(n) (*(uint16_t *)(ds+(uint16_t)(n)))
#define SWORD(n) (*(int16_t *)(ds+(uint16_t)(n)))
#define ARG_PTR(p) WORD(p)
#define ARG_REST(p) ((uint16_t)((p)+2))
#define ARG_INT(p) SWORD(p)
#define ARG_LONG(p) (*(int32_t *)(ds+(uint16_t)(p)))
#define ARG_INT_NEXT(p) ((p) += 2, SWORD((p)-2))
#define ARG_PTR_NEXT(p) ((p) += 2, WORD((p)-2))
#define NARGS(...) NARGS_(__VA_ARGS__,8,7,6,5,4,3,2,1,0)
#define NARGS_(a1,a2,a3,a4,a5,a6,a7,a8,n,...) n
#define EVENT(id,...) host->event(ds,id,(const uint16_t[]){__VA_ARGS__},NARGS(__VA_ARGS__))
#define EVENT0(id) host->event(ds,id,0,0)
#define KEY_REGS uint16_t r
#define KEY_READ(r) r = EVENT0(0)
#define KEY_CHAR(r) (uint8_t)(r)
#define KEY_WAITING() EVENT0(1)
#define CODE_0541_011E(v) EVENT(2, (uint16_t)(v))
#define BOX(x, y, w, h, c) EVENT(3, (uint16_t)(x), (uint16_t)(y), (uint16_t)(w), (uint16_t)(h), (uint16_t)(c))
#define CLEAR() EVENT0(4)
#define SOUND_RESET(v) EVENT(5, (uint16_t)(v))
#define PENDING_FINISH(v) EVENT(6, (uint16_t)(uint8_t)(v))
#define LENGTH(s) opeu_string_length(ds, s)
#define STORE_OUT host_store_out
#define PUT_OUT host_put_out
#define REQUEST_PARAMS uint16_t frame
#define REQUEST_DESTINATION WORD(frame)
#define REQUEST_ARGS (uint16_t)(frame+2)
#define PRINT_PARAMS uint16_t frame
#define PRINT_ARGS frame
#define ERASE_TEXT 0x142
#define opeu_text_x SWORD(0x15a)
#define opeu_text_y SWORD(0x15c)
#define opeu_text_left SWORD(0x15e)
#define opeu_text_paper ds[0x160]
#define opeu_text_ink ds[0x161]
#define opeu_text_background ds[0x162]
#define opeu_text_large SWORD(0x164)
#define opeu_text_lead ds[0x172]
#define opeu_text_state SWORD(0x174)
#define opeu_format_output WORD(0x176)
uint8_t opeu_key_read(STATE0);
void opeu_input_flush(STATE0);
int16_t opeu_format_decimal(STATE uint16_t *p);
int16_t opeu_put_unsigned(STATE uint16_t *p, uint16_t v, uint16_t base);
int16_t opeu_put_long(STATE uint16_t *p, int32_t v, int16_t base);
void opeu_format(STATE uint16_t args, TextOut out);
uint16_t opeu_open_sjis_to_jis(STATE uint16_t code);
void opeu_text_clamp(STATE0);
void opeu_army_position(STATE int16_t x, int16_t y);
void opeu_text_set_x(STATE int16_t x);
void opeu_text_set_y(STATE int16_t y);
void opeu_text_color(STATE uint16_t color);
void opeu_text_paper_set(STATE uint8_t paper);
void opeu_text_glyph(STATE int16_t x, int16_t y, int16_t attribute, uint16_t code);
void opeu_text_newline(STATE0);
void opeu_text_control(STATE uint8_t c);
uint8_t opeu_text_put(STATE uint8_t c);
void opeu_format_store(STATE char c);
void opeu_ui_text(STATE uint16_t s);
void opeu_cursor_print(STATE uint16_t frame);
void opeu_request_format(STATE uint16_t frame);
void opeu_text_put_range(STATE uint16_t s, uint8_t start, uint8_t count);
void opeu_text_erase_char(STATE0);

/* The far callbacks the original passes to opeu_format (02a0:04bc and
 * 02a0:0712) receive the character in AX. */
static void host_put_out(uint8_t *ds, TextHost *host, uint16_t c)
{
    opeu_text_put(ds, host, (uint8_t)c);
}

static void host_store_out(uint8_t *ds, TextHost *host, uint16_t c)
{
    opeu_format_store(ds, host, (char)c);
}

/* 0002: wait for a key (INT 16h) and return its character. */
uint8_t opeu_key_read(STATE0)
{
    KEY_REGS;
    KEY_READ(r);
    return KEY_CHAR(r);
}

/* 0022 */
void opeu_input_flush(STATE0)
{
    while (KEY_WAITING())
        opeu_key_read(ARGS0);
}

/* 0032: unsigned decimal at *p; *p is left at the first non-digit. */
I16 opeu_format_decimal(STATE PPCHAR p)
{
    uint16_t a, b;
    a = 0;
    while ((b = (uint8_t)AT(*p) - '0') < 10) {
        a = b + a*10;
        ++*p;
    }
    return a;
}

/* 006a/00d0: digits of v in base, uppercase; return the digit count. */
I16 CDECL opeu_put_unsigned(STATE PPCHAR p, uint16_t v, uint16_t base)
{
    I16 a;
    if (v < base) {
        AT(*p) = v < 10 ? v+'0' : v+'7';
        ++*p;
        return 1;
    }
    a = opeu_put_unsigned(ARGS p, v/base, base);
    opeu_put_unsigned(ARGS p, v%base, base);
    return a+1;
}

I16 CDECL opeu_put_long(STATE PPCHAR p, I32 v, I16 base)
{
    I16 a;
    if (v < base) {
        AT(*p) = v < 10 ? (char)v+'0' : (char)v+'7';
        ++*p;
        return 1;
    }
    a = opeu_put_long(ARGS p, v/base, base);
    opeu_put_long(ARGS p, v%base, base);
    return a+1;
}

/* 0168: args addresses the format pointer followed by its arguments; every
 * formatted character is passed to out. */
void CDECL opeu_format(STATE PARGS args, OUT_FN(out))
{
    uint8_t a;              /* pad character */
    I16 b;                  /* left-justify */
    uint8_t c;              /* current character */
    I16 d;                  /* precision */
    I16 e;                  /* remaining width */
    I16 f;                  /* long argument */
    PCHAR h;                /* conversion text */
    I16 i;                  /* base */
    PCHAR j;                /* format */
    I16 k;                  /* explicit plus */
    BUFFER(char, l, 20);    /* conversion buffer */
    I16 m;                  /* precision given */
    PARGS p;       /* next argument */
    ENTER(l, 20)
    j = ARG_PTR(args);
    p = ARG_REST(args);
    while ((c = AT(j++)) != 0) {
        if (c == '%') {
            h = l;
            d = 6;
            k = 0;
            f = 0;
            m = 0;
            b = 0;
            if (AT(j) == '-') { j++; b = 1; }
            if (AT(j) == '+') { j++; k = 1; }
            c = AT(j);
            a = c == '0' ? '0' : ' ';
            if (c >= '0' && c <= '9') e = opeu_format_decimal(ARGS &j);
            else e = 0;
            c = AT(j++);
            if (c == '.') {
                d = opeu_format_decimal(ARGS &j);
                m = 1;
                c = AT(j++);
            }
            if (opeu_ascii_upper(c) == 'L') {
                f = 1;
                c = AT(j++);
            }
            switch (opeu_ascii_upper(c)) {
            case 'X':
                i = 16;
                goto number;
            case 'D':
                if (f) {
                    if (ARG_LONG(p) < 0) { ARG_LONG(p) = NEGATE_LONG(ARG_LONG(p)); AT(h) = '-'; h++; e--; }
                    else if (k) { AT(h) = '+'; h++; e--; }
                } else {
                    if (ARG_INT(p) < 0) { ARG_INT(p) = -ARG_INT(p); AT(h) = '-'; h++; e--; }
                    else if (k) { AT(h) = '+'; h++; e--; }
                }
            case 'U':
                i = 10;
                goto number;
            case 'O':
                i = 8;
            number:
                if (!f)
                    e -= opeu_put_unsigned(ARGS &h, ARG_INT_NEXT(p), i);
                goto term;
            case 'S':
                h = ARG_PTR_NEXT(p);
                e -= LENGTH(h);
                goto pad;
            case 'C':
                AT(h) = (char)ARG_INT_NEXT(p);
                h++;
                e--;
            term:
                AT(h) = 0;
                h = l;
                m = 0;
            pad:
                if (!b)
                    while (e-- > 0) OUT(out, a);
                while (AT(h)) {
                    if (!m || d > 0) {
                        OUT(out, (uint8_t)AT(h));
                        d--;
                    }
                    h++;
                }
                if (b)
                    while (e-- > 0) OUT(out, ' ');
                continue;
            case 0:
                goto done;
            }
        }
        OUT(out, c);
    }
done:
    ;
    LEAVE(20)
}

/* 038e: Shift-JIS to JIS X 0208; 2228h for a code without a lead byte. */
uint16_t opeu_open_sjis_to_jis(STATE uint16_t code)
{
    uint8_t a, b;
    a = code >> 8;
    if (a > 0x80 && a < 0xf0) {
        a = (a - (a >= 0xa0 ? 0xb0 : 0x70)) << 1;
        b = (uint8_t)code;
        if (b >= 0x80)
            b--;
        if (b >= 0x9e)
            b -= 0x5e;
        else
            a--;
        b -= 0x1f;
        return b | a << 8;
    }
    return 0x2228;
}

void opeu_text_clamp(STATE0)
{
    opeu_text_x = opeu_text_x < 0 ? 0 : opeu_text_x > 79 ? 79 : opeu_text_x;
    opeu_text_y = opeu_text_y < 0 ? 0 : opeu_text_y > 399 ? 399 : opeu_text_y;
}

/* 0426: a negative coordinate keeps the current one. */
void opeu_army_position(STATE I16 x, I16 y)
{
    if (x >= 0) opeu_text_x = x;
    if (y >= 0) opeu_text_y = y;
    opeu_text_clamp(ARGS0);
}

void opeu_text_set_x(STATE I16 x)
{
    opeu_text_x = x;
    opeu_text_clamp(ARGS0);
}

void opeu_text_set_y(STATE I16 y)
{
    opeu_text_y = y;
    opeu_text_clamp(ARGS0);
}

void opeu_text_color(STATE uint16_t color)
{
    if (color <= 7) opeu_text_ink = color;
}

void opeu_text_paper_set(STATE uint8_t paper)
{
    opeu_text_paper = paper;
}

/* 045a: the glyph output of this build draws nothing. */
void opeu_text_glyph(STATE I16 x, I16 y, I16 attribute, uint16_t code)
{
    UNUSED(x) UNUSED(y) UNUSED(attribute) UNUSED(code)
}

/* 045e: lines are 16 pixels, 32 in large mode. */
void opeu_text_newline(STATE0)
{
    I16 a;
    opeu_text_x = opeu_text_left;
    a = opeu_text_large ? opeu_text_y + 32 : opeu_text_y + 16;
    if (a <= 399)
        opeu_text_y = a;
}

void opeu_text_control(STATE uint8_t c)
{
    switch (c) {
    case 10:
        opeu_text_newline(ARGS0);
        break;
    case 13:
        opeu_text_x = opeu_text_left;
        break;
    case 8:
        if (opeu_text_x >= 1) opeu_text_x--;
        break;
    case 7:
        SOUND_RESET(6);
        break;
    default:
        break;
    }
}

/* 04bc: console output of one byte; returns it. */
uint8_t opeu_text_put(STATE uint8_t c)
{
    uint16_t b;     /* character code */
    I16 d;          /* attribute */
    switch (opeu_text_state) {
    case 1:
        switch (c) {
        case '=':
            opeu_text_state = 2;
            break;
        case 'C':
            opeu_text_state = 4;
            break;
        case 'R':
            opeu_text_state = 5;
            break;
        case 'M':
            opeu_text_state = 6;
            break;
        case 'B':
            opeu_text_state = 7;
            break;
        case 'Y':
            opeu_text_state = 8;
            break;
        case 'E':
            SOUND_RESET(6);
        case 'W':
            opeu_text_state = 9;
            break;
        case 'T':
            BOX(opeu_text_x, opeu_text_y, 80 - opeu_text_x, opeu_text_large ? 32 : 16, opeu_text_background);
            goto reset;
        case '*':
            CLEAR();
            goto reset;
        case 'S':
            CODE_0541_011E(1);
            goto reset;
        default:
            break;
        }
        break;
    case 2:
        opeu_text_set_y(ARGS (c - 0x20) << 5);
        opeu_text_state = 3;
        break;
    case 3:
        opeu_text_set_x(ARGS c - 0x20);
    reset:
        opeu_text_state = 0;
        break;
    case 4:
        opeu_text_ink = c - '0';
        opeu_text_state = 0;
        break;
    case 5:
        opeu_text_left = c - 0x20;
        opeu_text_state = 0;
        break;
    case 6:
        opeu_text_paper = c - '0';
        opeu_text_state = 0;
        break;
    case 7:
        opeu_text_background = c - '0';
        opeu_text_state = 0;
        break;
    case 8:
        opeu_text_large = c == '1' ? 1 : 0;
        opeu_text_state = 0;
        break;
    case 9:
        PENDING_FINISH((uint8_t)(c - '0'));
        opeu_text_state = 0;
        break;
    default:
        if (opeu_text_lead) {
            b = opeu_open_sjis_to_jis(ARGS c | opeu_text_lead << 8);
            opeu_text_lead = 0;
        } else if ((c >= 0x80 && c <= 0x9f) || (c >= 0xe0 && c <= 0xfc)) {
            opeu_text_lead = c;
            break;
        } else if (c == 0x1b) {
            opeu_text_state = 1;
            break;
        } else if (c < 0x20) {
            opeu_text_control(ARGS c);
            break;
        } else
            b = c;
        if ((b >= 0x200 ? 78 : 79) < opeu_text_x)
            opeu_text_newline(ARGS0);
        d = opeu_text_background;
        if (!d) d = opeu_text_paper;
        d = d << 8 | opeu_text_ink;
        if (opeu_text_large)
            d |= 0x8000;
        opeu_text_glyph(ARGS opeu_text_x, opeu_text_y, d, b);
        opeu_text_x += b >= 0x200 ? 2 : 1;
        break;
    }
    return c;
}

/* 0712: opeu_format output for opeu_request_format. */
void opeu_format_store(STATE char c)
{
    AT(opeu_format_output++) = c;
}

/* 071e */
void opeu_ui_text(STATE PBYTE s)
{
    while (AT(s))
        opeu_text_put(ARGS AT(s++));
}

/* 073e: printf to the console. */
void CDECL opeu_cursor_print(STATE PRINT_PARAMS)
{
    opeu_format(ARGS PRINT_ARGS, PUT_OUT);
}

/* 0756: sprintf. */
void CDECL opeu_request_format(STATE REQUEST_PARAMS)
{
    opeu_format_output = REQUEST_DESTINATION;
    opeu_format(ARGS REQUEST_ARGS, STORE_OUT);
    AT(opeu_format_output) = 0;
}

void opeu_text_put_range(STATE PCHAR s, uint8_t start, uint8_t count)
{
    PCHAR a;
    a = s + start;
    while (count--)
        opeu_text_put(ARGS AT(a++));
}

/* 07a4: erase the previous character cell, keeping the paper. */
void opeu_text_erase_char(STATE0)
{
    uint8_t a;
    a = opeu_text_paper;
    opeu_ui_text(ARGS ERASE_TEXT);
    opeu_text_paper = a;
}
