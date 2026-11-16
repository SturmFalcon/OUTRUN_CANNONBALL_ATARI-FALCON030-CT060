#!/usr/bin/env python3
"""Function map and v0.39 comparison of a Cannonball Falcon executable (a.out-mintprg with symbols).

  funcmap.py map  PRG [out.tsv]        one row per text symbol: address, size, instructions, and the
                                        instruction classes that matter on a 68060 (div, mul, move16,
                                        branches, absolute ST/hardware accesses, movem, ...)
  funcmap.py diff OLD.PRG NEW.PRG       per function: same / modified / new / removed

Disassembly by m68k-atari-mint-objdump. Code is compared after normalisation: addresses, branch
targets and absolute operands are replaced by symbol names or a placeholder, so moving a function
does not count as a change. A changed body is a code difference, not a measured speed difference.
"""
import re, subprocess, sys, hashlib, collections

HDR = re.compile(r'^([0-9a-f]+) <([^>]+)>:$')
INS = re.compile(r'^\s*([0-9a-f]+):\t([0-9a-f ]+)\t(\S+)\s*(.*)$')

def disasm(prg):
    out = subprocess.run(['m68k-atari-mint-objdump', '-d', '--no-show-raw-insn', prg],
                         capture_output=True, text=True).stdout
    funcs = collections.OrderedDict(); cur = None
    for l in out.splitlines():
        m = HDR.match(l)
        if m:
            name = m.group(2)
            # Only names with the C/C++/global leading '_' start a function; .Lnn blocks and the
            # assembler's own labels (tv_loop, span_fr...) belong to the function above them.
            if name.startswith('_') or cur is None:
                if name in funcs: name = '%s@%s' % (name, m.group(1))
                cur = name; funcs[cur] = {'addr': int(m.group(1), 16), 'ins': [], 'labels': []}
            else:
                funcs[cur]['labels'].append(name)
            continue
        if cur is None: continue
        m = re.match(r'^\s*([0-9a-f]+):\t(\S+)\s*(.*)$', l)
        if m: funcs[cur]['ins'].append((int(m.group(1), 16), m.group(2), m.group(3)))
    return funcs

def norm(op):
    # objdump also names plain constants after a nearby symbol: every "value <sym+off>" becomes T
    op = re.sub(r'[0-9a-f]+ <[^>]*>', 'T', op)
    op = re.sub(r'#0x[0-9a-f]{5,}', '#ADDR', op)
    op = re.sub(r'#-?\d{5,}', '#ADDR', op)            # relocated addresses printed in decimal (also hides
                                                      # genuine constants >= 10000: documented limit)
    op = re.sub(r'(?<![#\w])[0-9a-f]{5,}(?![\w])', 'ADDR', op)
    return op

CLASSES = {
    'div': lambda m, o: m.startswith('div'),
    'mul': lambda m, o: m.startswith('mul'),
    'move16': lambda m, o: m.startswith('move16'),
    'movem': lambda m, o: m.startswith('movem'),
    'branch': lambda m, o: m[:1] == 'b' and m not in ('btst', 'bset', 'bclr', 'bchg', 'bfextu', 'bfins', 'bfexts', 'bfffo') or m.startswith('db'),
    'call': lambda m, o: m in ('jsr', 'bsrw', 'bsrl', 'bsrs', 'bsr'),
    'hw': lambda m, o: bool(re.search(r'\bff[f8][0-9a-f]{3}\b|#0xff8|0xffff8', o)),
    'trap': lambda m, o: m.startswith('trap'),
}

def summary(funcs):
    rows = []
    names = list(funcs)
    for i, n in enumerate(names):
        f = funcs[n]
        end = (f['ins'][-1][0] + 2) if f['ins'] else f['addr']   # approximate: last instruction start + 2
        c = collections.Counter()
        for a, m, o in f['ins']:
            for k, t in CLASSES.items():
                if t(m, o): c[k] += 1
        h = hashlib.md5('\n'.join(m + ' ' + norm(o) for a, m, o in f['ins']).encode()).hexdigest()[:12]
        hm = hashlib.md5('\n'.join(m for a, m, o in f['ins']).encode()).hexdigest()[:12]
        rows.append((n, f['addr'], end - f['addr'], len(f['ins']), c, h, hm))
    return rows

def cmd_map(prg, out=None):
    rows = summary(disasm(prg))
    lines = ['name\taddr\tsize\tinsns\t' + '\t'.join(CLASSES) + '\tcode_hash']
    for n, a, s, k, c, h, hm in rows:
        lines.append('%s\t0x%06x\t%d\t%d\t%s\t%s' % (n, a, s, k, '\t'.join(str(c[x]) for x in CLASSES), h))
    txt = '\n'.join(lines) + '\n'
    if out: open(out, 'w').write(txt)
    else: sys.stdout.write(txt)

def cmd_diff(old, new):
    o = {r[0]: r for r in summary(disasm(old))}; n = {r[0]: r for r in summary(disasm(new))}
    same = [k for k in n if k in o and o[k][5] == n[k][5]]
    opnd = [k for k in n if k in o and o[k][5] != n[k][5] and o[k][6] == n[k][6]]
    mod = [k for k in n if k in o and o[k][6] != n[k][6]]
    add = [k for k in n if k not in o]; rem = [k for k in o if k not in n]
    print('same %d  operands only %d  modified %d  new %d  removed %d' % (len(same), len(opnd), len(mod), len(add), len(rem)))
    print('(operands only = same instruction sequence, some operand differs: usually a moved address)')
    print('\n## modified: instruction sequence differs (name, size old -> new, instructions, div/mul)')
    for k in sorted(mod, key=lambda k: -abs(n[k][2] - o[k][2])):
        a, b = o[k], n[k]
        print('%-50s %6d -> %6d  %5d -> %5d  div %d->%d mul %d->%d' % (k, a[2], b[2], a[3], b[3], a[4]['div'], b[4]['div'], a[4]['mul'], b[4]['mul']))
    print('\n## new'); [print('%-50s %6d bytes %5d insns' % (k, n[k][2], n[k][3])) for k in add]
    print('\n## removed'); [print('%-50s %6d bytes' % (k, o[k][2])) for k in rem]

if __name__ == '__main__':
    if sys.argv[1] == 'map': cmd_map(*sys.argv[2:])
    elif sys.argv[1] == 'diff': cmd_diff(*sys.argv[2:4])
