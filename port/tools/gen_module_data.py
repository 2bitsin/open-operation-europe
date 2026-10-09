#!/usr/bin/env python3
"""Generate port/gen/module_data.c: the initialized DGROUP data that the
recovered CODE modules own in their DOS builds.

Several game modules keep string literals and static tables in their own
_DATA segment (and C 6.0a emits `$T` segment words for far tables into their
CONST segment). The host branches read those bytes through ds[] at the
original offsets, so the port must place them in the data image too. Every
byte here is derived from the recovered source text:

  C modules (src/main/{core,map,units}/<module>.c)
    The DOS branch is preprocessed natively (cc -E with _MSC_VER=600, the
    DOS compiler's version) and the _DATA layout of Microsoft C 6.0a /AM is
    rebuilt from the token stream, per function in definition order:
      1. string literals of block-scope `static` initializers, as parsed;
      2. string literals of the body, in code-generation order (call
         arguments right to left, everything else left to right);
      3. the block-scope `static` initialized objects, in declaration order.
    File-scope initialized objects are emitted where they are defined (their
    own initializer literals first). Literals keep their NUL and are never
    merged. Objects are byte-packed except that an object containing words
    is even-aligned (the sources supply explicit padding bytes where the
    original left such a gap). Near data pointers are the DS offsets of
    their targets; far code pointers are offset:segment, the offset taken
    from the host branch's `case 0x....: function(` dispatch and the segment
    from the module's code placement.
    CONST: one word per far extern table the DOS code indexes, in order of
    first use, holding the table's paragraph (port/gen/symbols.json).

  Assembly modules (src/main/asm/<module>.asm)
    The `db`/`dw`/`times` lines of the _DATA segment; a `dw label` is the
    label's frame-relative offset (label position from a native nasm
    listing of the same file plus the segment's placement).

Placements come from src/main/dos/layout.json (modules -> segments). The
layout is checked against the source text: each module must fit before the
next DGROUP placement, and every DS offset the host branch states for a
literal or table (TXT("text", 0x....), paired DOS/host macros) must hold.

Usage: port/tools/gen_module_data.py [cc] [nasm]
"""
import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
LAYOUT = ROOT/'src/main/dos/layout.json'
SYMBOLS = ROOT/'port/gen/symbols.json'
OUTPUT = ROOT/'port/gen/module_data.c'
CODE_DIRS = ('core', 'map', 'units')
DATA_INDEX, CONST_INDEX = '2', '3'   # SEGDEF order of C 6.0a objects: code, _DATA, CONST


class SourceError(Exception):
    """The recovered source no longer matches what this generator expects."""


def fail(module, message):
    raise SourceError(f'{module}: {message}')


# ---------------------------------------------------------------- C lexing --

TOKEN = re.compile(r'''
    (?P<ws>\s+)
  | (?P<str>"(?:\\.|[^"\\\n])*")
  | (?P<chr>'(?:\\.|[^'\\\n])+')
  | (?P<num>(?:0[xX][0-9a-fA-F]+|\d+)[uUlL]*)
  | (?P<id>[A-Za-z_]\w*)
  | (?P<punct>\.\.\.|<<=|>>=|->|\+\+|--|<<|>>|<=|>=|==|!=|&&|\|\||[-+*/%&|^]=
              |[][(){};,.?:~!<>=+\-*/%&|^])
''', re.X)


class Tok:
    __slots__ = ('kind', 'text', 'line')

    def __init__(self, kind, text, line):
        self.kind, self.text, self.line = kind, text, line

    def __repr__(self):
        return f'{self.text}@{self.line}'


def unescape(literal):
    """Bytes of one C string or character literal body (DOS code page bytes)."""
    body = literal[1:-1]
    out = bytearray()
    i = 0
    simple = {'n': 10, 't': 9, 'r': 13, 'a': 7, 'b': 8, 'f': 12, 'v': 11,
              '\\': 92, "'": 39, '"': 34, '?': 63}
    while i < len(body):
        c = body[i]
        if c != '\\':
            out += c.encode('latin-1')
            i += 1
            continue
        c = body[i+1]
        if c in simple:
            out.append(simple[c])
            i += 2
        elif c in '01234567':
            m = re.match(r'[0-7]{1,3}', body[i+1:])
            out.append(int(m.group(0), 8) & 0xff)
            i += 1 + len(m.group(0))
        elif c == 'x':
            m = re.match(r'[0-9a-fA-F]+', body[i+2:])
            out.append(int(m.group(0), 16) & 0xff)
            i += 2 + len(m.group(0))
        else:
            raise SourceError(f'unknown escape \\{c} in {literal}')
    return bytes(out)


def preprocess(cc, path, dos):
    """Token stream of `path` as the DOS (or host) compiler sees it.
    Directive lines of the module's own file (#define, #pragma) are kept
    as single 'dir' tokens in place."""
    args = [cc, '-E', '-dD', '-I', str(ROOT/'src/common'), '-I', str(ROOT/'src/main/include')]
    if dos:
        args.append('-D_MSC_VER=600')
    out = subprocess.run(args + [str(path)], capture_output=True, text=True, check=True).stdout
    tokens = []
    current, line = None, 0
    for text in out.splitlines():
        marker = re.match(r'#\s*(\d+)\s+"([^"]*)"', text)
        if marker:
            line, current = int(marker.group(1)), marker.group(2)
            continue
        stripped = text.strip()
        if stripped.startswith('#'):
            if current == str(path) or Path(current).resolve() == path.resolve():
                tokens.append(Tok('dir', stripped, line))
            line += 1
            continue
        pos = 0
        while pos < len(text):
            m = TOKEN.match(text, pos)
            if not m:
                raise SourceError(f'{path.name}:{line}: cannot tokenize {text[pos:pos+20]!r}')
            pos = m.end()
            if m.lastgroup != 'ws':
                tokens.append(Tok(m.lastgroup, m.group(0), line))
        line += 1
    return tokens


def match_close(toks, i):
    """Index of the bracket closing toks[i]."""
    pairs = {'(': ')', '[': ']', '{': '}'}
    opener, closer, depth = toks[i].text, pairs[toks[i].text], 0
    for j in range(i, len(toks)):
        if toks[j].text == opener and toks[j].kind == 'punct':
            depth += 1
        elif toks[j].text == closer and toks[j].kind == 'punct':
            depth -= 1
            if depth == 0:
                return j
    raise SourceError(f'unbalanced {opener} at line {toks[i].line}')


def split_top(toks, sep=','):
    """Split a token list at top-level separators."""
    parts, cur, depth = [], [], 0
    for t in toks:
        if t.kind == 'punct' and t.text in '([{':
            depth += 1
        elif t.kind == 'punct' and t.text in ')]}':
            depth -= 1
        if depth == 0 and t.kind == 'punct' and t.text == sep:
            parts.append(cur)
            cur = []
        else:
            cur.append(t)
    if cur or parts:
        parts.append(cur)
    return parts


# ---------------------------------------------------------------- C types --

class Scalar:
    def __init__(self, size, name):
        self.size, self.name = size, name
        self.align = size


class Pointer:
    def __init__(self, target, far=False):
        self.target, self.far = target, far
        self.code = isinstance(target, Func)
        # /AM: data pointers near unless declared far; code pointers far.
        self.size = 4 if (far or self.code) else 2
        self.align = 2


class Array:
    def __init__(self, elem, count):
        self.elem, self.count = elem, count

    @property
    def size(self):
        return self.elem.size * self.count

    @property
    def align(self):
        return self.elem.align


class Struct:
    def __init__(self, fields, size, align):
        self.fields, self.size, self.align = fields, size, align


class Func:
    size = 0
    align = 1

    def __init__(self, result):
        self.result = result


SCALARS = {'char': 1, 'short': 2, 'int': 2, 'long': 4, 'float': 4, 'double': 8}
STORAGE = {'typedef', 'extern', 'static', 'auto', 'register'}
QUALIFIERS = {'const', 'volatile', '_cdecl', 'cdecl', '_pascal', 'pascal', '_fastcall',
              '_loadds', '_saveregs', '_export', '_interrupt', 'interrupt'}
FARNESS = {'far': True, '_far': True, 'near': False, '_near': False, 'huge': True, '_huge': True}
KEYWORDS = (set(SCALARS) | STORAGE | QUALIFIERS | set(FARNESS) |
            {'unsigned', 'signed', 'void', 'struct', 'union', 'enum', 'if', 'else', 'while',
             'for', 'do', 'switch', 'case', 'default', 'return', 'break', 'continue', 'goto',
             'sizeof', '_segment'})


class Declaration:
    def __init__(self, name, ctype, storage, far, init, line):
        self.name, self.type, self.storage, self.far, self.init, self.line = \
            name, ctype, storage, far, init, line


class CParser:
    """Just enough of C to lay out declarations and initializers."""

    def __init__(self, module):
        self.module = module
        self.typedefs = {}
        self.tags = {}
        self.pack = None          # None: the compiler default

    def is_type_start(self, tok):
        return tok.kind == 'id' and (tok.text in SCALARS or tok.text in STORAGE or
                                     tok.text in QUALIFIERS or tok.text in FARNESS or
                                     tok.text in ('unsigned', 'signed', 'void', 'struct',
                                                  'union', 'enum', '_segment') or
                                     tok.text in self.typedefs)

    def specifiers(self, toks, i):
        storage, far, base, sign, words = None, None, None, None, []
        while i < len(toks):
            t = toks[i]
            if t.kind != 'id':
                break
            if t.text in STORAGE:
                storage = t.text
            elif t.text in QUALIFIERS:
                pass
            elif t.text in FARNESS:
                far = FARNESS[t.text]
            elif t.text in ('unsigned', 'signed'):
                sign = t.text
            elif t.text in SCALARS:
                words.append(t.text)
            elif t.text == '_segment':
                base = Scalar(2, '_segment')
            elif t.text == 'void':
                base = Scalar(0, 'void')
            elif t.text in ('struct', 'union'):
                base, i = self.struct_spec(toks, i)
                continue
            elif t.text == 'enum':
                fail(self.module, f'enum declarations are not modelled (line {t.line})')
            elif t.text in self.typedefs and base is None and not words and sign is None:
                base = self.typedefs[t.text]
            else:
                break
            i += 1
        if base is None:
            if not words and sign is None:
                fail(self.module, f'no type at line {toks[i].line if i < len(toks) else "?"}')
            size = SCALARS[[w for w in words if w != 'int'][0]] if [w for w in words if w != 'int'] \
                else 2
            base = Scalar(size, ' '.join(([sign] if sign else []) + words))
        return storage, far, base, i

    def struct_spec(self, toks, i):
        kind = toks[i].text
        i += 1
        tag = None
        if toks[i].kind == 'id':
            tag = toks[i].text
            i += 1
        if toks[i].text != '{':
            if tag not in self.tags:
                self.tags[tag] = None      # incomplete; only pointers to it are used
            return self.tags.get(tag) or Struct([], 0, 1), i
        close = match_close(toks, i)
        if kind == 'union':
            fail(self.module, f'unions are not modelled (line {toks[i].line})')
        if tag:
            self.tags[tag] = Struct([], 0, 1)   # allow self-referencing pointers
        fields, offset, align = [], 0, 1
        for member in split_top(toks[i+1:close], ';'):
            if not member:
                continue
            _, far, base, j = self.specifiers(member, 0)
            for part in split_top(member[j:]):
                name, ctype = self.declarator(part, 0, base, far)[:2]
                a = self.field_align(ctype)
                pad = -offset % a
                if pad and self.pack is None:
                    fail(self.module, f'struct member {name} needs padding under the default '
                                      f'packing (line {part[0].line}); not modelled')
                offset += pad
                fields.append((name, ctype, offset))
                offset += ctype.size
                align = max(align, a)
        size = offset + (-offset % align)
        struct = Struct(fields, size, align)
        if tag:
            self.tags[tag] = struct
        return struct, close + 1

    def field_align(self, ctype):
        natural = min(ctype.align, 2)
        return min(natural, self.pack) if self.pack else natural

    def declarator(self, toks, i, base, far):
        """Parse one declarator; returns (name, type, next index)."""
        pointers = []
        while i < len(toks) and (toks[i].text == '*' or toks[i].text in QUALIFIERS or
                                 toks[i].text in FARNESS):
            if toks[i].text == '*':
                pointers.append(far is True)
            elif toks[i].text in FARNESS and pointers:
                pointers[-1] = FARNESS[toks[i].text]
            i += 1
        name, inner = None, None
        if i < len(toks) and toks[i].text == '(' and i + 1 < len(toks) and \
                (toks[i+1].text == '*' or toks[i+1].text in QUALIFIERS or toks[i+1].text in FARNESS):
            close = match_close(toks, i)
            inner = toks[i+1:close]
            i = close + 1
        elif i < len(toks) and toks[i].kind == 'id':
            name = toks[i].text
            i += 1
        suffixes = []
        while i < len(toks) and toks[i].text in ('[', '('):
            close = match_close(toks, i)
            if toks[i].text == '[':
                suffixes.append(('array', toks[i+1:close]))
            else:
                suffixes.append(('func', None))
            i = close + 1
        ctype = base
        for is_far in pointers:
            ctype = Pointer(ctype, is_far)
        for kind, expr in reversed(suffixes):
            if kind == 'func':
                ctype = Func(ctype)
            else:
                ctype = Array(ctype, const_eval(self.module, expr) if expr else None)
        if inner is not None:
            name, ctype, _ = self.declarator(inner, 0, ctype, None)
        return name, ctype, i


def const_eval(module, toks):
    """Value of an integer constant expression (casts to scalar types dropped)."""
    out = []
    i = 0
    while i < len(toks):
        t = toks[i]
        if t.text == '(' and i + 1 < len(toks) and toks[i+1].kind == 'id' and \
                (toks[i+1].text in SCALARS or toks[i+1].text in ('unsigned', 'signed') or
                 re.fullmatch(r'u?int(8|16|32)_t', toks[i+1].text)):
            i = match_close(toks, i) + 1
            continue
        if t.kind == 'num':
            out.append(str(int(t.text.rstrip('uUlL'), 0)))
        elif t.kind == 'chr':
            value = unescape(t.text)
            if len(value) != 1:
                fail(module, f'character constant {t.text}')
            out.append(str(value[0]))
        elif t.kind == 'punct' and t.text in ('+', '-', '*', '%', '<<', '>>', '&', '|', '^', '~', '(', ')'):
            out.append(t.text)
        elif t.kind == 'punct' and t.text == '/':
            out.append('//')
        else:
            fail(module, f'unsupported constant expression token {t.text!r} at line {t.line}')
        i += 1
    if not out:
        fail(module, 'empty constant expression')
    return eval(' '.join(out), {'__builtins__': {}})


# ---------------------------------------------------------- module image --

class Item:
    """One emitted piece of a module's _DATA: a literal, an object or a gap."""

    def __init__(self, kind, name, size, align=1, line=0):
        self.kind, self.name, self.size, self.align, self.line = kind, name, size, align, line
        self.data = bytearray(size)
        self.refs = []            # (offset in item, target Item, addend)
        self.code_refs = []       # (offset in item, function name)
        self.address = None


class CModule:
    def __init__(self, name, path, cc, placement, code_frame):
        self.name, self.path, self.cc = name, path, cc
        self.placement, self.code_frame = placement, code_frame
        self.parser = CParser(name)
        self.items = []
        self.globals = {}             # file-scope initialized objects by name
        self.far_externs = {}         # far extern tables by name
        self.functions = set()
        self.const_uses = []          # far tables in order of first use
        self.dos_macros = []          # (name, params, value tokens, (local, file) scopes)
        raw = path.read_text(errors='replace')
        self.raw = raw
        self.code_offsets = {}
        for m in re.finditer(r'case\s+(0x[0-9a-fA-F]+)\s*:\s*(?:return\s+)?(\w+)\s*\(', raw):
            self.code_offsets.setdefault(m.group(2), set()).add(int(m.group(1), 16))

    # -- top level
    def build(self):
        toks = preprocess(self.cc, self.path, dos=True)
        i = 0
        while i < len(toks):
            t = toks[i]
            if t.kind == 'dir':
                self.directive(t, {})
                i += 1
                continue
            if t.text == ';':
                i += 1
                continue
            j = i
            while j < len(toks) and toks[j].kind != 'dir' and toks[j].text not in (';', '{', '='):
                if toks[j].text in ('(', '['):
                    j = match_close(toks, j)
                j += 1
            if j < len(toks) and toks[j].text == '{' and toks[j-1].text == ')':
                close = match_close(toks, j)
                self.function(toks[i:j], toks[j+1:close])
                i = close + 1
                continue
            end = self.statement_end(toks, i)
            for decl in self.declaration(toks[i:end], local=None):
                obj = self.object_item(decl)
                self.globals[decl.name] = obj
                self.items.extend(obj.literals)
                self.items.append(obj)
            i = end + 1

    def statement_end(self, toks, i):
        j = i
        while toks[j].text != ';' or toks[j].kind != 'punct':
            if toks[j].kind == 'punct' and toks[j].text in '([{':
                j = match_close(toks, j)
            j += 1
        return j

    def directive(self, tok, scope):
        text = tok.text
        m = re.match(r'#\s*pragma\s+pack\s*\(\s*(\d*)\s*\)', text)
        if m:
            self.parser.pack = int(m.group(1)) if m.group(1) else None
            return
        m = re.match(r'#\s*define\s+(\w+)(\([^)]*\))?\s*(.*)$', text)
        if m:
            params = set(re.findall(r'\w+', m.group(2) or ''))
            value = [Tok(k.lastgroup, k.group(0), tok.line)
                     for k in TOKEN.finditer(m.group(3)) if k.lastgroup != 'ws']
            # names resolve when checked: the function's statics, then file scope
            self.dos_macros.append((m.group(1), params, value, (scope, self.globals)))

    def declaration(self, toks, local):
        """A declaration statement; returns parse-time literals and objects."""
        if not toks:
            return []
        storage, far, base, i = self.parser.specifiers(toks, 0)
        if storage == 'typedef':
            for part in split_top(toks[i:]):
                name, ctype, _ = self.parser.declarator(part, 0, base, far)
                self.parser.typedefs[name] = ctype
            return []
        made = []
        for part in split_top(toks[i:]):
            if not part:
                continue
            name, ctype, j = self.parser.declarator(part, 0, base, far)
            init = part[j+1:] if j < len(part) and part[j].text == '=' else None
            if storage == 'extern' or isinstance(ctype, Func):
                if far and not isinstance(ctype, (Func, Pointer)):
                    self.far_externs[name] = ctype
                continue
            if init is None:
                continue          # uninitialized: _BSS / communal, not _DATA
            if local is not None and storage != 'static':
                local['auto'].append(init)   # auto initializer: code-generation literals
                continue
            made.append(Declaration(name, ctype, storage, far, init, part[0].line))
        return made

    # -- functions
    def function(self, header, body):
        _, _, base, i = self.parser.specifiers(header, 0)
        name, ctype, _ = self.parser.declarator(header, i, base, None)
        self.functions.add(name)
        scope = {}
        statics = []
        code = []
        local = {'auto': []}
        k = 0
        while k < len(body):
            t = body[k]
            if t.kind == 'dir':
                self.directive(t, scope)
                k += 1
                continue
            at_statement = k == 0 or body[k-1].text in ('{', ';', '}') or body[k-1].kind == 'dir'
            if at_statement and t.kind == 'id' and self.parser.is_type_start(t) and \
                    not (k + 1 < len(body) and body[k+1].text == ':'):
                end = self.statement_end(body, k)
                for decl in self.declaration(body[k:end], local):
                    obj = self.object_item(decl)
                    statics.append(obj)
                    scope[decl.name] = obj
                    self.items.extend(obj.literals)
                # an auto initializer is code: keep its tokens in order
                for init in local['auto']:
                    code.extend(init)
                    code.append(Tok('punct', ';', t.line))
                local['auto'] = []
                k = end + 1
                continue
            code.append(t)
            k += 1
        for value, line in literal_order(code):
            self.items.append(literal_item(value, line))
        for t in code:
            if t.kind == 'id' and t.text in self.far_externs and t.text not in self.const_uses:
                self.const_uses.append(t.text)
        for obj in statics:
            self.items.append(obj)

    # -- objects
    def object_item(self, decl):
        ctype = decl.type
        tree = parse_initializer(decl.init)
        if isinstance(ctype, Array) and ctype.count is None:
            if tree[0] == 'str' and isinstance(ctype.elem, Scalar) and ctype.elem.size == 1:
                count = len(tree[1]) + 1
            elif tree[0] == 'brace':
                count = len(tree[1])
            else:
                fail(self.name, f'cannot size {decl.name}')
            ctype = Array(ctype.elem, count)
        obj = Item('object', decl.name, ctype.size, min(ctype.align, 2), decl.line)
        obj.literals = []
        obj.ctype = ctype
        self.fill(obj, ctype, tree, 0, decl.name)
        return obj

    def fill(self, obj, ctype, node, offset, what):
        if isinstance(ctype, (Array, Struct)):
            if node[0] == 'str' and isinstance(ctype, Array) and ctype.elem.size == 1:
                value = node[1]
                if len(value) > ctype.count:
                    fail(self.name, f'string too long for {what}')
                obj.data[offset:offset+len(value)] = value
                return
            if node[0] != 'brace':
                fail(self.name, f'aggregate {what} needs braces')
            rest = self.fill_aggregate(obj, ctype, node[1], 0, offset, what)
            if rest != len(node[1]):
                fail(self.name, f'too many initializers for {what}')
            return
        if node[0] == 'brace':
            if len(node[1]) != 1:
                fail(self.name, f'scalar {what} with {len(node[1])} initializers')
            node = node[1][0]
        self.scalar(obj, ctype, node, offset, what)

    def fill_aggregate(self, obj, ctype, nodes, i, offset, what):
        """Fill ctype from nodes[i:], with brace elision; returns next index."""
        if isinstance(ctype, Array):
            members = [(f'{what}[{n}]', ctype.elem, offset + n*ctype.elem.size) for n in range(ctype.count)]
        else:
            members = [(f'{what}.{n}', t, offset + o) for n, t, o in ctype.fields]
        for name, mtype, moffset in members:
            if i >= len(nodes):
                break
            node = nodes[i]
            if isinstance(mtype, (Array, Struct)) and node[0] != 'brace' and \
                    not (node[0] == 'str' and isinstance(mtype, Array) and mtype.elem.size == 1):
                i = self.fill_aggregate(obj, mtype, nodes, i, moffset, name)
                continue
            self.fill(obj, mtype, node, moffset, name)
            i += 1
        return i

    def scalar(self, obj, ctype, node, offset, what):
        if node[0] == 'str':
            if not isinstance(ctype, Pointer) or ctype.far:
                fail(self.name, f'string literal for non-pointer {what}')
            lit = literal_item(node[1], node[2])
            obj.literals.append(lit)
            obj.refs.append((offset, lit, 0))
            return
        toks = node[1]
        if isinstance(ctype, Pointer):
            while toks and toks[0].text == '(' and self.parser.is_type_start(toks[1]):
                toks = toks[match_close(toks, 0)+1:]
            if len(toks) == 1 and toks[0].kind == 'id' or \
                    len(toks) == 2 and toks[0].text == '&' and toks[1].kind == 'id':
                target = toks[-1].text
                if ctype.code:
                    self.code_pointer(obj, offset, target, what)
                    return
                if target in self.globals:
                    obj.refs.append((offset, self.globals[target], 0))
                    return
                fail(self.name, f'{what}: pointer to {target}, not an object of this module')
        value = const_eval(self.name, toks)
        if isinstance(ctype, Pointer) and value != 0:
            fail(self.name, f'{what}: non-null integer pointer')
        size = ctype.size
        obj.data[offset:offset+size] = (value & ((1 << 8*size) - 1)).to_bytes(size, 'little')

    def code_pointer(self, obj, offset, function, what):
        if function not in self.functions:
            fail(self.name, f'{what}: far code pointer to {function}, not defined in this module')
        offsets = self.code_offsets.get(function)
        if not offsets or len(offsets) != 1:
            fail(self.name, f'{what}: no unique host dispatch offset for {function}')
        value = next(iter(offsets))
        obj.data[offset:offset+4] = bytes([value & 0xff, value >> 8,
                                           self.code_frame & 0xff, self.code_frame >> 8])
        obj.code_refs.append((offset, function))


def parse_initializer(toks):
    """('brace', [nodes]) | ('str', bytes, line) | ('expr', tokens)."""
    if toks and toks[0].text == '{':
        close = match_close(toks, 0)
        if close != len(toks) - 1:
            raise SourceError(f'trailing tokens after initializer at line {toks[0].line}')
        return ('brace', [parse_initializer(p) for p in split_top(toks[1:close]) if p])
    if toks and all(t.kind == 'str' for t in toks):
        return ('str', b''.join(unescape(t.text) for t in toks), toks[0].line)
    return ('expr', toks)


def literal_item(value, line):
    item = Item('literal', None, len(value) + 1, 1, line)
    item.data[:len(value)] = value
    item.value = value
    return item


def literal_order(toks):
    """String literals of statement tokens in C 6.0a code-generation order:
    the arguments of a call right to left, everything else left to right."""
    out = []

    def walk(seq):
        i = 0
        while i < len(seq):
            t = seq[i]
            if t.kind == 'str':
                j, value = i, b''
                while j < len(seq) and seq[j].kind == 'str':
                    value += unescape(seq[j].text)
                    j += 1
                out.append((value, t.line))
                i = j
                continue
            if t.kind == 'punct' and t.text in '([{':
                close = match_close(seq, i)
                inner = seq[i+1:close]
                prev = seq[i-1] if i else None
                call = t.text == '(' and prev is not None and (
                    (prev.kind == 'id' and prev.text not in KEYWORDS) or prev.text in (')', ']'))
                if call:
                    for arg in reversed(split_top(inner)):
                        walk(arg)
                else:
                    walk(inner)
                i = close + 1
                continue
            i += 1
    walk(toks)
    return out


# ------------------------------------------------------------- assembly --

ASM_LINE = re.compile(r'^\s*(?:(\w+):)?\s*(.*?)\s*(?:;.*)?$')


def asm_values(module, text):
    """Comma-separated db/dw operands (numbers, quoted strings, labels)."""
    values = []
    for part in re.findall(r"'[^']*'|\"[^\"]*\"|[^,]+", text):
        part = part.strip()
        if not part or part == ',':
            continue
        values.append(part)
    return values


def asm_number(module, text):
    text = text.strip()
    if re.fullmatch(r'0x[0-9a-fA-F]+|\d+', text):
        return int(text, 0)
    if re.fullmatch(r'[0-9][0-9a-fA-F]*h', text, re.I):
        return int(text[:-1], 16)
    return None


def asm_labels(nasm, path, module):
    """{label: (segment name, offset)} from a native nasm listing of the file."""
    with tempfile.TemporaryDirectory() as tmp:
        listing = Path(tmp)/'list.lst'
        subprocess.run([nasm, '-f', 'obj', '-O0', '-l', str(listing), '-o', str(Path(tmp)/'out.obj'),
                        str(path)], check=True, capture_output=True)
        lines = listing.read_text(errors='replace').splitlines()
    labels, pending, segment = {}, [], None
    for line in lines:
        m = re.match(r'^\s*\d+\s+(?:<\d+>\s+)?(?:([0-9A-F]{8})\s+\S+\s+)?(.*)$', line)
        if not m:
            continue
        address, source = m.group(1), m.group(2).split(';')[0].strip()
        seg = re.match(r'segment\s+(\w+)', source)
        if seg:
            segment, pending = seg.group(1), []
            continue
        lab = re.match(r'^(\w+):', source)
        if lab:
            pending.append(lab.group(1))
        if address is not None:
            for name in pending:
                labels[name] = (segment, int(address, 16))
            pending = []
    return labels


def build_asm(name, path, nasm, segments):
    """Items of the _DATA segment of an assembly module."""
    text = path.read_text(errors='replace')
    order = []
    for m in re.finditer(r'^segment\s+(\w+)', text, re.M):
        if m.group(1) not in order:
            order.append(m.group(1))
    if '_DATA' not in order:
        fail(name, 'no _DATA segment')
    index = {seg: str(n + 1) for n, seg in enumerate(order)}
    body = text[text.index('segment _DATA'):]
    body = body.split('\n', 1)[1]
    labels = None
    data = bytearray()
    notes = []
    for raw in body.splitlines():
        if re.match(r'^segment\s', raw):
            break
        m = ASM_LINE.match(raw)
        label, stmt = m.group(1), m.group(2)
        if label:
            notes.append((len(data), label))
        if not stmt or stmt.startswith('group '):
            continue
        times = re.match(r'times\s+(\S+)\s+(db|dw)\s+(.*)$', stmt)
        count, op, operands = (asm_number(name, times.group(1)), times.group(2), times.group(3)) \
            if times else (1, None, None)
        if not times:
            d = re.match(r'(db|dw)\s+(.*)$', stmt)
            if not d:
                fail(name, f'unexpected _DATA line {raw.strip()!r}')
            op, operands = d.group(1), d.group(2)
        if count is None:
            fail(name, f'bad repeat count in {raw.strip()!r}')
        chunk = bytearray()
        for value in asm_values(name, operands):
            if value[0] in '\'"':
                if op != 'db':
                    fail(name, f'string operand of dw in {raw.strip()!r}')
                chunk += value[1:-1].encode('latin-1')
                continue
            number = asm_number(name, value)
            if number is None:
                if op != 'dw' or not re.fullmatch(r'\w+', value):
                    fail(name, f'unsupported operand {value!r}')
                if labels is None:
                    labels = asm_labels(nasm, path, name)
                if value not in labels:
                    fail(name, f'label {value} not found in the listing')
                seg, off = labels[value]
                if index.get(seg) not in segments:
                    fail(name, f'label {value} in unplaced segment {seg}')
                number = segments[index[seg]][1] + off
                notes.append((len(data) + len(chunk), f'dw {value} = {seg}+{off:#x}'))
            size = 1 if op == 'db' else 2
            if number >= 1 << 8*size:
                fail(name, f'operand {value} too large for {op}')
            chunk += number.to_bytes(size, 'little')
        data += chunk * count
    item = Item('object', '_DATA', len(data), 1)
    item.data[:] = data
    item.notes = notes
    return [item]


# --------------------------------------------------------------- driver --

def layout_modules(layout):
    """(module, kind, path) for every module with a DGROUP placement."""
    dgroup = {tuple(v['groups']['1']) for v in layout['modules'].values() if '1' in v.get('groups', {})}
    if len(dgroup) != 1:
        raise SourceError(f'expected one DGROUP frame, found {dgroup}')
    frame = next(iter(dgroup))[0]
    found = []
    for name, entry in layout['modules'].items():
        placed = {i: s for i, s in entry['segments'].items() if s[0] == frame}
        if not placed:
            continue
        stem = name.lower()
        if entry.get('class') == 'library':
            found.append((name, 'library', None, placed, entry))
            continue
        data = ROOT/'src/main/data'/f'{stem}.c'
        if data.exists():
            found.append((name, 'data', data, placed, entry))
            continue
        asm = ROOT/'src/main/asm'/f'{stem}.asm'
        if entry.get('class') == 'asm' and asm.exists():
            found.append((name, 'asm', asm, placed, entry))
            continue
        code = [ROOT/'src/main'/d/f'{stem}.c' for d in CODE_DIRS if (ROOT/'src/main'/d/f'{stem}.c').exists()]
        if len(code) != 1:
            raise SourceError(f'{name}: DGROUP placement but no unique source file')
        found.append((name, 'c', code[0], placed, entry))
    return frame, found


def host_macros(cc, path):
    out = subprocess.run([cc, '-E', '-dM', '-I', str(ROOT/'src/common'), '-I', str(ROOT/'src/main/include'),
                          str(path)], capture_output=True, text=True, check=True).stdout
    macros = {}
    for line in out.splitlines():
        m = re.match(r'#define\s+(\w+)(\([^)]*\))?\s*(.*)$', line)
        if m:
            macros[m.group(1)] = m.group(3)
    return macros


def evidence(module, cc, image, frame):
    """Checks of the generated bytes against offsets the source states."""
    checks = []
    for m in re.finditer(r'\bTXT\(\s*((?:"(?:\\.|[^"\\])*"\s*)+),\s*(0x[0-9a-fA-F]+)\s*\)', module.raw):
        value = b''.join(unescape(s) for s in re.findall(r'"(?:\\.|[^"\\])*"', m.group(1)))
        checks.append(('literal', int(m.group(2), 16), value, f'TXT {value[:20]!r}'))
    host = host_macros(cc, module.path)
    for name, params, value, visible in module.dos_macros:
        if name not in host:
            continue
        hexes = re.findall(r'0x[0-9a-fA-F]+', host[name])
        if len(hexes) != 1:
            continue
        offset = int(hexes[0], 16)
        if value and all(t.kind == 'str' for t in value):
            checks.append(('literal', offset, b''.join(unescape(t.text) for t in value), f'macro {name}'))
            continue
        ids = [t.text for t in value if t.kind == 'id' and t.text not in params]
        for names in visible:
            if ids and ids[0] in names:
                checks.append(('object', offset, names[ids[0]], f'macro {name} -> {ids[0]}'))
                break
    done = []
    for kind, offset, what, label in checks:
        if kind == 'literal':
            got = image.get_bytes(offset, len(what) + 1)
            if got != what + b'\0':
                fail(module.name, f'{label}: DS:{offset:04x} holds {got!r}, source says {what!r}')
        elif what.address != offset:
            fail(module.name, f'{label}: object at DS:{what.address:04x}, host branch says {offset:04x}')
        done.append(f'{label} @ {offset:04x}')
    return done


class Image:
    """The generated DGROUP bytes, for the evidence checks."""

    def __init__(self):
        self.bytes = {}

    def put(self, offset, data):
        for n, b in enumerate(data):
            self.bytes[offset + n] = b

    def get_bytes(self, offset, size):
        if not all(offset + n in self.bytes for n in range(size)):
            return None
        return bytes(self.bytes[offset + n] for n in range(size))


def place(items, start):
    address = start
    out = []
    for item in items:
        if item.align > 1 and address % item.align:
            gap = Item('gap', 'alignment', item.align - address % item.align)
            gap.address = address
            out.append(gap)
            address += gap.size
        item.address = address
        address += item.size
        out.append(item)
    for item in out:
        for offset, target, addend in item.refs:
            value = target.address + addend
            item.data[offset:offset+2] = value.to_bytes(2, 'little')
    return out, address


def describe(item):
    if item.kind == 'literal':
        text = item.value.decode('latin-1')
        text = ''.join(c if 32 <= ord(c) < 127 and c not in '*/\\' else f'\\x{ord(c):02x}' for c in text)
        return f'"{text[:56]}"' + ('...' if len(text) > 56 else '')
    return item.name


def main(argv):
    cc = argv[1] if len(argv) > 1 else 'cc'
    nasm = argv[2] if len(argv) > 2 else 'nasm'
    layout = json.loads(LAYOUT.read_text())
    symbols = json.loads(SYMBOLS.read_text())
    frame, modules = layout_modules(layout)
    starts = sorted({s[1] for _, _, _, placed, _ in modules for s in placed.values()})
    blocks = []          # (offset, module, index, items, path)
    report = []
    library = []
    image = Image()
    for name, kind, path, placed, entry in modules:
        if kind == 'data':
            continue
        if kind == 'library':
            library.append(f'{name} ({entry.get("library")}): ' +
                           ', '.join(f'{o:04x}' for o in sorted({s[1] for s in placed.values()})))
            continue
        if kind == 'asm':
            if set(placed) != {'2'}:
                fail(name, f'unexpected DGROUP segments {sorted(placed)}')
            items = build_asm(name, path, nasm, entry['segments'])
            blocks.append((placed['2'][1], name, '_DATA', items, path))
            continue
        code = [s for i, s in entry['segments'].items() if s[0] != frame]
        if len(code) != 1:
            fail(name, 'expected exactly one code segment')
        module = CModule(name, path, cc, placed, code[0][0])
        module.build()
        # file-scope objects are registered as they are made (for pointers)
        if set(placed) - {DATA_INDEX, CONST_INDEX}:
            fail(name, f'unexpected DGROUP segments {sorted(placed)}')
        if bool(module.items) != (DATA_INDEX in placed):
            fail(name, f'{len(module.items)} _DATA items but layout _DATA placement '
                       f'{"present" if DATA_INDEX in placed else "absent"}')
        if bool(module.const_uses) != (CONST_INDEX in placed):
            fail(name, f'far tables used {module.const_uses} but layout CONST placement '
                       f'{"present" if CONST_INDEX in placed else "absent"}')
        if module.items:
            blocks.append((placed[DATA_INDEX][1], name, '_DATA', module.items, path, module))
        if module.const_uses:
            words = []
            for table in module.const_uses:
                key = '_' + table
                if key not in symbols:
                    fail(name, f'no placement for far table {table}')
                item = Item('object', f'SEG {table}', 2, 2)
                item.data[:] = symbols[key][0].to_bytes(2, 'little')
                words.append(item)
            blocks.append((placed[CONST_INDEX][1], name, 'CONST', words, path))
    placed_blocks = []
    for block in sorted(blocks, key=lambda b: b[0]):
        start, name = block[0], block[1]
        items, end = place(block[3], start)
        following = [s for s in starts if s > start]
        limit = following[0] if following else 0x10000
        if end > limit:
            fail(name, f'{block[2]} {start:04x}..{end:04x} overlaps the next placement at {limit:04x}')
        for item in items:
            image.put(item.address, item.data)
        placed_blocks.append((start, end, limit, name, block[2], items, block[4]))
    checks = []
    for block in blocks:
        if len(block) > 5:
            checks += [f'{block[1]}: {c}' for c in evidence(block[5], cc, image, frame)]
    write_output(frame, placed_blocks)
    total = 0
    for start, end, limit, name, seg, items, path in placed_blocks:
        total += end - start
        gap = f', {limit - end} unowned byte(s) before {limit:04x}' if limit != end else ''
        report.append(f'  {name:9} {seg:6} DS:{start:04x}..{end:04x} ({end-start} bytes){gap}')
    print(f'{len(placed_blocks)} blocks, {total} bytes')
    print('\n'.join(report))
    print(f'{len(checks)} source-stated offsets verified')
    for c in checks:
        print('  ' + c)
    print('library modules with DGROUP data (not provided):')
    for line in library:
        print('  ' + line)
    return 0


def write_output(frame, blocks):
    lines = ['/* Generated by port/tools/gen_module_data.py; do not edit.',
             ' * Initialized DGROUP data owned by the recovered code modules (string',
             ' * literals, static tables and CONST segment words), laid out from the',
             ' * recovered source text; see the generator for the rules. */',
             '#include "port.h"', '']
    entries = []
    for start, end, limit, name, seg, items, path in blocks:
        array = f'{name.lower()}_{seg.strip("_").lower()}'
        rel = path.relative_to(ROOT)
        lines.append(f'/* {name} {seg} ({rel}) at {frame:04x}:{start:04x}, {end-start} bytes')
        for item in items:
            if item.kind == 'gap':
                lines.append(f' *   {item.address:04x} alignment gap ({item.size})')
                continue
            for note_offset, note in getattr(item, 'notes', []):
                lines.append(f' *   {item.address + note_offset:04x} {note}')
            if not getattr(item, 'notes', None):
                lines.append(f' *   {item.address:04x} {describe(item)}')
        lines.append(' */')
        data = bytearray()
        for item in items:
            data += item.data
        lines.append(f'static const uint8_t {array}[{len(data)}] = {{')
        for n in range(0, len(data), 12):
            lines.append('    ' + ', '.join(f'0x{b:02x}' for b in data[n:n+12]) + ',')
        lines.append('};')
        lines.append('')
        entries.append(f'    {{ 0x{frame:04x}u, 0x{start:04x}u, {len(data)}u, {array}, "{name}" }},')
    lines.append('const PortImageBytes port_module_data[] = {')
    lines += entries
    lines.append('};')
    lines.append(f'const unsigned port_module_data_count = {len(entries)};')
    lines.append('')
    OUTPUT.write_text('\n'.join(lines))


if __name__ == '__main__':
    try:
        sys.exit(main(sys.argv))
    except (SourceError, subprocess.CalledProcessError) as error:
        print(f'gen_module_data: {error}', file=sys.stderr)
        sys.exit(1)
