from dragon_cfg import *
import sys
import pprint

ChunkType = DragonCfg.ChunkType

class CfgElem:
    start = 1
    end = 2
    word = 3
    rule = 4
    list = 5

class CfgRange:
    seq = 1
    alt = 2
    rep = 3
    opt = 4

def finalize(cur, els):
    joiner = ' '
    if cur == CfgRange.alt:
        joiner = ' | '

    borders = {
        CfgRange.seq: ('', ''),
        CfgRange.alt: ('(', ')'),
        CfgRange.rep: ('(', ')+'),
        CfgRange.opt: ('[', ']'),
    }
    a, b = borders[cur]
    return a + (joiner.join(els)) + b

def parse(path):
    with open(path, 'rb') as f:
        stream = KaitaiStream(BytesIO(f.read()))
    cfg = DragonCfg(stream)
    imports = {}
    exports = {}
    words = {}
    rules = {}
    lists = {}
    lookup = {
        ChunkType.imports: imports,
        ChunkType.exports: exports,
        ChunkType.words: words,
        ChunkType.rules: rules,
        ChunkType.lists: lists,
    }
    cfglookup = {
        CfgElem.start: {
            CfgRange.seq: '<seq>',
            CfgRange.alt: '<alt>',
            CfgRange.rep: '<rep>',
            CfgRange.opt: '<opt>',
        },
        CfgElem.end: {
            CfgRange.seq: '</seq>',
            CfgRange.alt: '</alt>',
            CfgRange.rep: '</rep>',
            CfgRange.opt: '</opt>',
        },
        CfgElem.word: words,
        CfgElem.rule: rules,
        CfgElem.list: lists,
    }

    for chunk in cfg.chunks:
        if chunk.type != ChunkType.rules:
            d = lookup[chunk.type]
            for ent in chunk.defs.ids:
                d[ent.id] = ent.name

    for val, name in imports.items():
        rules[val] = name

    ruledefs = {}
    for chunk in cfg.chunks:
        if chunk.type == ChunkType.rules:
            for cfg_rule in chunk.defs.rules:
                rawrule = []
                stack = []
                cur = CfgRange.seq
                curseq = []
                for ent in cfg_rule.rdefs:
                    if ent.type == CfgElem.start:
                        stack.append((cur, curseq))
                        cur = ent.value
                        curseq = []
                    elif ent.type == CfgElem.end:
                        if ent.value != cur:
                            print 'warning: invalid cfg: mismatched start/end'
                        tmp = finalize(cur, curseq)
                        cur, curseq = stack.pop(-1)
                        curseq.append(tmp)

                    if ent.type == CfgElem.rule:
                        name = imports.get(ent.value, exports.get(ent.value, '%d' % ent.value))
                        val = '<%s>' % name
                    else:
                        val = cfglookup.get(ent.type, {}).get(ent.value, (ent.type, ent.value))

                    val = str(val)
                    if ent.type == CfgElem.list:
                        val = '{' + val + '}'

                    if ent.type not in (CfgElem.start, CfgElem.end):
                        curseq.append(val)
                    rawrule.append(val)

                if stack:
                    print 'warning: unclosed sequence. raw rule:', rawrule
                rule = finalize(cur, curseq)
                ruledefs[cfg_rule.rule_num] = rule

    print 'type:', cfg.grammar_type
    print 'flags:', cfg.grammar_flags
    print

    if imports:
        print 'imports:'
        for k, v in imports.items():
            print '%3d: %s' % (k, v)
        print

    if exports:
        print 'exports:'
        for k, v in exports.items():
            print '%3d: %s' % (k, v)
        print

    # print 'words:'
    # pprint.pprint(words)

    if lists:
        print 'lists:'
        for k, v in lists.items():
            print '%3d: %s' % (k, v)
        print

    print 'private rules:'
    for k, v in sorted(ruledefs.items()):
        if not k in exports:
            print '%3d: %s' % (k, v)
    print

    print 'public rules:'
    pad = max(map(len, exports.values()))
    for k, v in sorted(ruledefs.items()):
        if k in exports:
            name = ('<' + exports[k] + '>').rjust(pad + 2)
            print '%3d: %s %s' % (k, name, v)

parse(sys.argv[1])
