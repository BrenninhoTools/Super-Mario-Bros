import re
import sys
import zlib
from pathlib import Path

OPCODES = {
    "adc": {"imm": 0x69, "zp": 0x65, "zpx": 0x75, "abs": 0x6D, "absx": 0x7D, "absy": 0x79, "indx": 0x61, "indy": 0x71},
    "and": {"imm": 0x29, "zp": 0x25, "zpx": 0x35, "abs": 0x2D, "absx": 0x3D, "absy": 0x39, "indx": 0x21, "indy": 0x31},
    "asl": {"acc": 0x0A, "zp": 0x06, "zpx": 0x16, "abs": 0x0E, "absx": 0x1E},
    "bcc": {"rel": 0x90},
    "bcs": {"rel": 0xB0},
    "beq": {"rel": 0xF0},
    "bit": {"zp": 0x24, "abs": 0x2C},
    "bmi": {"rel": 0x30},
    "bne": {"rel": 0xD0},
    "bpl": {"rel": 0x10},
    "brk": {"imp": 0x00},
    "bvc": {"rel": 0x50},
    "bvs": {"rel": 0x70},
    "clc": {"imp": 0x18},
    "cld": {"imp": 0xD8},
    "cli": {"imp": 0x58},
    "clv": {"imp": 0xB8},
    "cmp": {"imm": 0xC9, "zp": 0xC5, "zpx": 0xD5, "abs": 0xCD, "absx": 0xDD, "absy": 0xD9, "indx": 0xC1, "indy": 0xD1},
    "cpx": {"imm": 0xE0, "zp": 0xE4, "abs": 0xEC},
    "cpy": {"imm": 0xC0, "zp": 0xC4, "abs": 0xCC},
    "dec": {"zp": 0xC6, "zpx": 0xD6, "abs": 0xCE, "absx": 0xDE},
    "dex": {"imp": 0xCA},
    "dey": {"imp": 0x88},
    "eor": {"imm": 0x49, "zp": 0x45, "zpx": 0x55, "abs": 0x4D, "absx": 0x5D, "absy": 0x59, "indx": 0x41, "indy": 0x51},
    "inc": {"zp": 0xE6, "zpx": 0xF6, "abs": 0xEE, "absx": 0xFE},
    "inx": {"imp": 0xE8},
    "iny": {"imp": 0xC8},
    "jmp": {"abs": 0x4C, "ind": 0x6C},
    "jsr": {"abs": 0x20},
    "lda": {"imm": 0xA9, "zp": 0xA5, "zpx": 0xB5, "abs": 0xAD, "absx": 0xBD, "absy": 0xB9, "indx": 0xA1, "indy": 0xB1},
    "ldx": {"imm": 0xA2, "zp": 0xA6, "zpy": 0xB6, "abs": 0xAE, "absy": 0xBE},
    "ldy": {"imm": 0xA0, "zp": 0xA4, "zpx": 0xB4, "abs": 0xAC, "absx": 0xBC},
    "lsr": {"acc": 0x4A, "zp": 0x46, "zpx": 0x56, "abs": 0x4E, "absx": 0x5E},
    "nop": {"imp": 0xEA},
    "ora": {"imm": 0x09, "zp": 0x05, "zpx": 0x15, "abs": 0x0D, "absx": 0x1D, "absy": 0x19, "indx": 0x01, "indy": 0x11},
    "pha": {"imp": 0x48},
    "php": {"imp": 0x08},
    "pla": {"imp": 0x68},
    "plp": {"imp": 0x28},
    "rol": {"acc": 0x2A, "zp": 0x26, "zpx": 0x36, "abs": 0x2E, "absx": 0x3E},
    "ror": {"acc": 0x6A, "zp": 0x66, "zpx": 0x76, "abs": 0x6E, "absx": 0x7E},
    "rti": {"imp": 0x40},
    "rts": {"imp": 0x60},
    "sbc": {"imm": 0xE9, "zp": 0xE5, "zpx": 0xF5, "abs": 0xED, "absx": 0xFD, "absy": 0xF9, "indx": 0xE1, "indy": 0xF1},
    "sec": {"imp": 0x38},
    "sed": {"imp": 0xF8},
    "sei": {"imp": 0x78},
    "sta": {"zp": 0x85, "zpx": 0x95, "abs": 0x8D, "absx": 0x9D, "absy": 0x99, "indx": 0x81, "indy": 0x91},
    "stx": {"zp": 0x86, "zpy": 0x96, "abs": 0x8E},
    "sty": {"zp": 0x84, "zpx": 0x94, "abs": 0x8C},
    "tax": {"imp": 0xAA},
    "tay": {"imp": 0xA8},
    "tsx": {"imp": 0xBA},
    "txa": {"imp": 0x8A},
    "txs": {"imp": 0x9A},
    "tya": {"imp": 0x98},
}

SIZES = {"imp": 1, "acc": 1, "imm": 2, "zp": 2, "zpx": 2, "zpy": 2, "rel": 2, "indx": 2, "indy": 2, "abs": 3, "absx": 3, "absy": 3, "ind": 3}

LINE_RE = re.compile(r"^\s*(?:([A-Za-z_][\w]*)\s*:)?\s*(.*?)\s*$")
CONST_RE = re.compile(r"^\s*([A-Za-z_][\w]*)\s*=\s*(.+?)\s*$")


class AsmError(Exception):
    pass


class Statement:
    def __init__(self, kind, line, **kw):
        self.kind = kind
        self.line = line
        self.__dict__.update(kw)


class Assembler:
    def __init__(self, text):
        self.consts = {}
        self.labels = {}
        self.statements = []
        self.parse(text)

    def parse(self, text):
        for number, raw in enumerate(text.splitlines(), 1):
            line = raw.split(";", 1)[0].rstrip()
            if not line.strip():
                continue
            m = CONST_RE.match(line)
            if m and not line.lstrip().startswith("."):
                self.consts[m.group(1)] = m.group(2)
                continue
            m = LINE_RE.match(line)
            label, body = m.group(1), m.group(2)
            if label:
                self.statements.append(Statement("label", number, name=label))
            if not body:
                continue
            parts = body.split(None, 1)
            head = parts[0].lower()
            rest = parts[1].strip() if len(parts) > 1 else ""
            if head in (".db", ".dw"):
                values = [v.strip() for v in rest.split(",") if v.strip()]
                self.statements.append(Statement(head[1:], number, values=values))
            elif head == ".org":
                self.statements.append(Statement("org", number, expr=rest))
            elif head in (".mem", ".index"):
                continue
            elif head in OPCODES:
                self.statements.append(Statement("ins", number, mnem=head, operand=rest))
            else:
                raise AsmError(f"line {number}: unknown statement '{body}'")

    def number(self, tok):
        if tok.startswith("$"):
            return int(tok[1:], 16)
        if tok.startswith("%"):
            return int(tok[1:], 2)
        return int(tok, 10)

    def evaluate(self, expr, resolve_labels=True, depth=0):
        expr = expr.strip()
        if depth > 32:
            raise AsmError(f"recursive expression '{expr}'")
        selector = None
        if expr[:1] in "<>":
            selector = expr[0]
            expr = expr[1:].strip()
        tokens = re.findall(r"\$[0-9A-Fa-f]+|%[01]+|\d+|[A-Za-z_]\w*|[+\-]", expr)
        if "".join(tokens) != expr.replace(" ", ""):
            raise AsmError(f"bad expression '{expr}'")
        total = 0
        sign = 1
        unresolved = False
        for tok in tokens:
            if tok == "+":
                sign = 1
                continue
            if tok == "-":
                sign = -1
                continue
            if tok[0] in "$%" or tok[0].isdigit():
                value = self.number(tok)
            elif tok in self.consts:
                value, sub_unresolved = self.evaluate(self.consts[tok], resolve_labels, depth + 1)
                unresolved |= sub_unresolved
            elif tok in self.labels:
                value = self.labels[tok]
            elif not resolve_labels:
                value = 0x8000
                unresolved = True
            else:
                raise AsmError(f"undefined symbol '{tok}'")
            total += sign * value
            sign = 1
        if selector == "<":
            total &= 0xFF
        elif selector == ">":
            total = (total >> 8) & 0xFF
        return total, unresolved and selector is None

    def uses_label(self, expr):
        for tok in re.findall(r"\$[0-9A-Fa-f]+|%[01]+|[A-Za-z_]\w*", expr):
            if tok[0] in "$%":
                continue
            if tok in self.consts:
                if self.uses_label(self.consts[tok]):
                    return True
            else:
                return True
        return False

    def mode_of(self, mnem, operand, final):
        table = OPCODES[mnem]
        op = operand.replace(" ", "")
        if not op or op.lower() == "a":
            return ("acc" if "acc" in table else "imp"), None
        if "rel" in table:
            return "rel", op
        if op.startswith("#"):
            return "imm", op[1:]
        m = re.fullmatch(r"\((.+)\),[yY]", op)
        if m:
            return "indy", m.group(1)
        m = re.fullmatch(r"\((.+),[xX]\)", op)
        if m:
            return "indx", m.group(1)
        m = re.fullmatch(r"\((.+)\)", op)
        if m:
            return "ind", m.group(1)
        index = None
        m = re.fullmatch(r"(.+),([xXyY])", op)
        if m:
            op, index = m.group(1), m.group(2).lower()
        forced_abs = bool(re.fullmatch(r"\$[0-9A-Fa-f]{4}", op))
        if self.uses_label(op):
            value, unresolved = self.evaluate(op, final)
            small = (not unresolved) and value < 0x100
        else:
            value, _ = self.evaluate(op, False)
            small = value < 0x100
        small = small and not forced_abs
        base = "zp" if small else "abs"
        mode = base + (index or "")
        if mode not in table and mode.startswith("zp"):
            mode = "abs" + (index or "")
        if mode not in table:
            raise AsmError(f"{mnem} has no mode {mode}")
        return mode, op

    def layout(self, final):
        pc = 0
        self.items = []
        for st in self.statements:
            if st.kind == "org":
                pc = self.evaluate(st.expr, final)[0]
            elif st.kind == "label":
                if not final:
                    self.labels[st.name] = pc
                elif self.labels.get(st.name) != pc:
                    raise AsmError(f"label {st.name} moved between passes")
            elif st.kind == "db":
                st.addr = pc
                pc += len(st.values)
            elif st.kind == "dw":
                st.addr = pc
                pc += 2 * len(st.values)
            elif st.kind == "ins":
                mode, expr = self.mode_of(st.mnem, st.operand, final)
                st.addr = pc
                st.mode = mode
                st.expr = expr
                pc += SIZES[mode]
            self.items.append(st)
        return pc

    def assemble(self):
        self.layout(False)
        end = self.layout(True)
        if end != 0x10000:
            raise AsmError(f"program ends at ${end:04X}, expected $10000")
        prg = bytearray(0x8000)
        for st in self.items:
            if st.kind == "db":
                for i, v in enumerate(st.values):
                    prg[st.addr - 0x8000 + i] = self.evaluate(v)[0] & 0xFF
            elif st.kind == "dw":
                for i, v in enumerate(st.values):
                    word = self.evaluate(v)[0] & 0xFFFF
                    prg[st.addr - 0x8000 + 2 * i] = word & 0xFF
                    prg[st.addr - 0x8000 + 2 * i + 1] = word >> 8
                st.words = [self.evaluate(v)[0] & 0xFFFF for v in st.values]
            elif st.kind == "ins":
                o = st.addr - 0x8000
                prg[o] = OPCODES[st.mnem][st.mode]
                if st.mode == "rel":
                    target = self.evaluate(st.expr)[0]
                    delta = target - (st.addr + 2)
                    if not -128 <= delta <= 127:
                        raise AsmError(f"line {st.line}: branch out of range")
                    st.value = target
                    prg[o + 1] = delta & 0xFF
                elif SIZES[st.mode] == 2:
                    st.value = self.evaluate(st.expr)[0] & 0xFF
                    prg[o + 1] = st.value
                elif SIZES[st.mode] == 3:
                    st.value = self.evaluate(st.expr)[0] & 0xFFFF
                    prg[o + 1] = st.value & 0xFF
                    prg[o + 2] = st.value >> 8
                else:
                    st.value = None
        self.prg = bytes(prg)
        return self.prg


class Translator:
    def __init__(self, asm):
        self.asm = asm
        self.code = {}
        self.bit_skips = set()
        items = asm.items
        flat = [st for st in items if st.kind in ("ins", "db", "dw")]
        for i, st in enumerate(flat):
            if st.kind == "ins":
                self.code[st.addr] = st
        for i, st in enumerate(flat):
            if st.kind == "db" and len(st.values) == 1 and asm.evaluate(st.values[0])[0] == 0x2C:
                prev = flat[i - 1] if i > 0 else None
                nxt = flat[i + 1] if i + 1 < len(flat) else None
                if prev is not None and nxt is not None and prev.kind == "ins" and nxt.kind == "ins" and SIZES[nxt.mode] == 2:
                    if prev.mnem not in ("rts", "rti", "jmp"):
                        self.bit_skips.add(st.addr)
                        self.code[st.addr] = st
        self.returns = sorted(
            st.addr + 3 for st in self.code.values()
            if st.kind == "ins" and st.mnem == "jsr" and (st.addr + 3) in self.code
        )
        self.indirect_targets = set()
        for st in items:
            if st.kind == "dw":
                for w in st.words:
                    if w in self.code:
                        self.indirect_targets.add(w)
        self.indirect_targets = sorted(self.indirect_targets)
        self.reset = asm.labels["Start"]
        self.nmi = asm.labels["NonMaskableInterrupt"]

    @staticmethod
    def h(v, width=4):
        return f"0x{v:0{width}X}"

    def read_expr(self, st):
        m, v = st.mode, st.value
        h = self.h
        if m == "imm":
            return h(v, 2)
        if m == "zp":
            return f"ram[{h(v, 2)}]"
        if m == "zpx":
            return f"ram[(uint8_t)({h(v, 2)} + X)]"
        if m == "zpy":
            return f"ram[(uint8_t)({h(v, 2)} + Y)]"
        if m == "abs":
            if v < 0x2000:
                return f"ram[{h(v & 0x7FF)}]"
            return f"bus.read({h(v)})"
        if m in ("absx", "absy"):
            r = "X" if m == "absx" else "Y"
            if v + 0xFF < 0x800:
                return f"ram[{h(v)} + {r}]"
            return f"bus.read((uint16_t)({h(v)} + {r}))"
        if m == "indy":
            return f"bus.read((uint16_t)((ram[{h(v, 2)}] | (ram[(uint8_t)({h(v, 2)} + 1)] << 8)) + Y))"
        if m == "indx":
            return f"bus.read((uint16_t)(ram[(uint8_t)({h(v, 2)} + X)] | (ram[(uint8_t)({h(v, 2)} + X + 1)] << 8)))"
        raise AsmError(f"no read for mode {m}")

    def write_stmt(self, st, value):
        m, v = st.mode, st.value
        h = self.h
        if m == "zp":
            return f"ram[{h(v, 2)}] = {value};"
        if m == "zpx":
            return f"ram[(uint8_t)({h(v, 2)} + X)] = {value};"
        if m == "zpy":
            return f"ram[(uint8_t)({h(v, 2)} + Y)] = {value};"
        if m == "abs":
            if v < 0x2000:
                return f"ram[{h(v & 0x7FF)}] = {value};"
            return f"bus.write({h(v)}, {value});"
        if m in ("absx", "absy"):
            r = "X" if m == "absx" else "Y"
            if v + 0xFF < 0x800:
                return f"ram[{h(v)} + {r}] = {value};"
            return f"bus.write((uint16_t)({h(v)} + {r}), {value});"
        if m == "indy":
            return f"bus.write((uint16_t)((ram[{h(v, 2)}] | (ram[(uint8_t)({h(v, 2)} + 1)] << 8)) + Y), {value});"
        if m == "indx":
            return f"bus.write((uint16_t)(ram[(uint8_t)({h(v, 2)} + X)] | (ram[(uint8_t)({h(v, 2)} + X + 1)] << 8)), {value});"
        raise AsmError(f"no write for mode {m}")

    def rmw(self, st, op):
        if st.mode == "acc":
            return op.replace("@", "A").replace("$", "A = ")
        return None

    def emit_instruction(self, st):
        h = self.h
        if st.kind == "db":
            prg = self.asm.prg
            target = prg[st.addr - 0x7FFF] | (prg[st.addr - 0x7FFE] << 8)
            return f"t = bus.read({h(target)}); Z = !(A & t); N = t & 0x80; V = t & 0x40; goto L{st.addr + 3:04X};"
        mn = st.mnem
        if mn in ("lda", "ldx", "ldy"):
            reg = mn[2].upper()
            return f"{reg} = {self.read_expr(st)}; NZ({reg});"
        if mn in ("sta", "stx", "sty"):
            return self.write_stmt(st, mn[2].upper())
        if mn in ("adc", "sbc"):
            inv = " ^ 0xFF" if mn == "sbc" else ""
            return f"t = {self.read_expr(st)}{inv}; ADC(t);"
        if mn in ("and", "ora", "eor"):
            op = {"and": "&", "ora": "|", "eor": "^"}[mn]
            return f"A {op}= {self.read_expr(st)}; NZ(A);"
        if mn in ("cmp", "cpx", "cpy"):
            reg = {"cmp": "A", "cpx": "X", "cpy": "Y"}[mn]
            return f"t = {self.read_expr(st)}; CMP({reg}, t);"
        if mn == "bit":
            return f"t = {self.read_expr(st)}; Z = !(A & t); N = t & 0x80; V = t & 0x40;"
        if mn in ("asl", "lsr", "rol", "ror"):
            body = {
                "asl": "C = t & 0x80; t = (uint8_t)(t << 1);",
                "lsr": "C = t & 0x01; t = t >> 1;",
                "rol": "u = (t << 1) | (C ? 1 : 0); C = t & 0x80; t = (uint8_t)u;",
                "ror": "u = (t >> 1) | (C ? 0x80 : 0); C = t & 0x01; t = (uint8_t)u;",
            }[mn]
            if st.mode == "acc":
                return f"t = A; {body} A = t; NZ(A);"
            return f"t = {self.read_expr(st)}; {body} {self.write_stmt(st, 't')} NZ(t);"
        if mn in ("inc", "dec"):
            d = "+" if mn == "inc" else "-"
            return f"t = (uint8_t)({self.read_expr(st)} {d} 1); {self.write_stmt(st, 't')} NZ(t);"
        simple = {
            "inx": "X++; NZ(X);",
            "iny": "Y++; NZ(Y);",
            "dex": "X--; NZ(X);",
            "dey": "Y--; NZ(Y);",
            "tax": "X = A; NZ(X);",
            "tay": "Y = A; NZ(Y);",
            "txa": "A = X; NZ(A);",
            "tya": "A = Y; NZ(A);",
            "tsx": "X = S; NZ(X);",
            "txs": "S = X;",
            "clc": "C = false;",
            "sec": "C = true;",
            "cli": "I = false;",
            "sei": "I = true;",
            "cld": "D = false;",
            "sed": "D = true;",
            "clv": "V = false;",
            "nop": ";",
            "pha": "PUSH(A);",
            "pla": "A = PULL(); NZ(A);",
            "php": "PUSH(PACK());",
            "plp": "t = PULL(); UNPACK(t);",
            "rti": "t = PULL(); UNPACK(t); S += 2; return;",
            "brk": "fault(0x00, " + h(st.addr) + "); return;",
        }
        if mn in simple:
            return simple[mn]
        if mn in ("bcc", "bcs", "beq", "bne", "bmi", "bpl", "bvc", "bvs"):
            cond = {"bcc": "!C", "bcs": "C", "beq": "Z", "bne": "!Z", "bmi": "N", "bpl": "!N", "bvc": "!V", "bvs": "V"}[mn]
            return f"if ({cond}) goto L{st.value:04X};"
        if mn == "jmp":
            if st.mode == "ind":
                return f"u = bus.read({h(st.value)}) | (bus.read({h(st.value + 1)}) << 8); goto indirect;"
            if st.value == st.addr:
                return "return;"
            return f"goto L{st.value:04X};"
        if mn == "jsr":
            ret = st.addr + 2
            return f"PUSH({h(ret >> 8, 2)}); PUSH({h(ret & 0xFF, 2)}); goto L{st.value:04X};"
        if mn == "rts":
            return "u = PULL(); u |= PULL() << 8; u = (u + 1) & 0xFFFF; goto ret;"
        raise AsmError(f"cannot translate {mn}")

    def generate(self, crc):
        out = []
        w = out.append
        w('#include "Cpu.hpp"')
        w("")
        w("#define NZ(v) (Z = !(v), N = ((v) & 0x80) != 0)")
        w("#define ADC(v) do { u = A + (v) + (C ? 1 : 0); V = (~(A ^ (v)) & (A ^ u) & 0x80) != 0; C = u > 0xFF; A = (uint8_t)u; NZ(A); } while (0)")
        w("#define CMP(r, v) do { C = (r) >= (v); t = (uint8_t)((r) - (v)); NZ(t); } while (0)")
        w("#define PUSH(v) (ram[0x100 + S--] = (uint8_t)(v))")
        w("#define PULL() (ram[0x100 + ++S])")
        w("#define PACK() (uint8_t)((N ? 0x80 : 0) | (V ? 0x40 : 0) | 0x30 | (D ? 0x08 : 0) | (I ? 0x04 : 0) | (Z ? 0x02 : 0) | (C ? 0x01 : 0))")
        w("#define UNPACK(p) (N = (p) & 0x80, V = (p) & 0x40, D = (p) & 0x08, I = (p) & 0x04, Z = (p) & 0x02, C = (p) & 0x01)")
        w("")
        w("namespace smb")
        w("{")
        w(f"const uint32_t Cpu::kPrgCrc = 0x{crc:08X}u;")
        w(f"const uint16_t Cpu::kResetVector = {self.h(self.reset)};")
        w(f"const uint16_t Cpu::kNmiVector = {self.h(self.nmi)};")
        w("")
        w("void Cpu::execute(uint16_t entry)")
        w("{")
        w("    uint8_t* ram = bus.ram;")
        w("    uint8_t t = 0;")
        w("    unsigned u = 0;")
        w("    switch (entry)")
        w("    {")
        for e in sorted({self.reset, self.nmi}):
            w(f"    case {self.h(e)}: goto L{e:04X};")
        w("    default: fault(0x01, entry); return;")
        w("    }")
        for addr in sorted(self.code):
            st = self.code[addr]
            w(f"L{addr:04X}: {self.emit_instruction(st)}")
        w("ret:")
        w("    switch (u)")
        w("    {")
        for r in self.returns:
            w(f"    case {self.h(r)}: goto L{r:04X};")
        w("    default: fault(0x02, (uint16_t)u); return;")
        w("    }")
        w("indirect:")
        w("    switch (u)")
        w("    {")
        for r in self.indirect_targets:
            w(f"    case {self.h(r)}: goto L{r:04X};")
        w("    default: fault(0x03, (uint16_t)u); return;")
        w("    }")
        w("}")
        w("}")
        w("")
        return "\n".join(out)


def main():
    if len(sys.argv) < 3:
        print("usage: smb2cpp.py <smbdis.asm> <out.cpp> [out.prg]")
        return 2
    text = Path(sys.argv[1]).read_text(encoding="latin-1")
    asm = Assembler(text)
    prg = asm.assemble()
    vectors = (prg[0x7FFA] | prg[0x7FFB] << 8, prg[0x7FFC] | prg[0x7FFD] << 8)
    if vectors != (asm.labels["NonMaskableInterrupt"], asm.labels["Start"]):
        raise AsmError("interrupt vectors do not match labels")
    crc = zlib.crc32(prg) & 0xFFFFFFFF
    tr = Translator(asm)
    source = tr.generate(crc)
    out = Path(sys.argv[2])
    out.parent.mkdir(parents=True, exist_ok=True)
    if not out.exists() or out.read_text() != source:
        out.write_text(source)
    if len(sys.argv) > 3:
        Path(sys.argv[3]).write_bytes(prg)
    return 0


if __name__ == "__main__":
    sys.exit(main())
