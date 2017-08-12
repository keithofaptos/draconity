# This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

import array
import struct
import zlib
from enum import Enum
from pkg_resources import parse_version

from kaitaistruct import __version__ as ks_version, KaitaiStruct, KaitaiStream, BytesIO


if parse_version(ks_version) < parse_version('0.7'):
    raise Exception("Incompatible Kaitai Struct Python API: 0.7 or later is required, but you have %s" % (ks_version))

class DragonCfg(KaitaiStruct):

    class ChunkType(Enum):
        words = 2
        rules = 3
        exports = 4
        imports = 5
        lists = 6

    class CfgElem(Enum):
        start = 1
        end = 2
        word = 3
        rule = 4
        list = 5

    class CfgRange(Enum):
        seq = 1
        alt = 2
        rep = 3
        opt = 4
    def __init__(self, _io, _parent=None, _root=None):
        self._io = _io
        self._parent = _parent
        self._root = _root if _root else self
        self.grammar_type = self._io.read_u4le()
        self.grammar_flags = self._io.read_u4le()
        self.chunks = []
        while not self._io.is_eof():
            self.chunks.append(self._root.Chunk(self._io, self, self._root))


    class Chunk(KaitaiStruct):
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self.type = self._root.ChunkType(self._io.read_u4le())
            self.size = self._io.read_u4le()
            _on = self.type
            if _on == self._root.ChunkType.imports:
                self._raw_defs = self._io.read_bytes(self.size)
                io = KaitaiStream(BytesIO(self._raw_defs))
                self.defs = self._root.IdChunk(io, self, self._root)
            elif _on == self._root.ChunkType.words:
                self._raw_defs = self._io.read_bytes(self.size)
                io = KaitaiStream(BytesIO(self._raw_defs))
                self.defs = self._root.IdChunk(io, self, self._root)
            elif _on == self._root.ChunkType.lists:
                self._raw_defs = self._io.read_bytes(self.size)
                io = KaitaiStream(BytesIO(self._raw_defs))
                self.defs = self._root.IdChunk(io, self, self._root)
            elif _on == self._root.ChunkType.exports:
                self._raw_defs = self._io.read_bytes(self.size)
                io = KaitaiStream(BytesIO(self._raw_defs))
                self.defs = self._root.IdChunk(io, self, self._root)
            elif _on == self._root.ChunkType.rules:
                self._raw_defs = self._io.read_bytes(self.size)
                io = KaitaiStream(BytesIO(self._raw_defs))
                self.defs = self._root.RuleChunk(io, self, self._root)
            else:
                self.defs = self._io.read_bytes(self.size)


    class RuleChunk(KaitaiStruct):
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self.rules = []
            while not self._io.is_eof():
                self.rules.append(self._root.Rule(self._io, self, self._root))



    class Rule(KaitaiStruct):
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self.size = self._io.read_u4le()
            self.rule_num = self._io.read_u4le()
            self.rdefs = [None] * ((self.size - 8) // 8)
            for i in range((self.size - 8) // 8):
                self.rdefs[i] = self._root.RuleDef(self._io, self, self._root)



    class IdDef(KaitaiStruct):
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self.size = self._io.read_u4le()
            self.id = self._io.read_u4le()
            self.name = (KaitaiStream.bytes_terminate(self._io.read_bytes((self.size - 8)), 0, False)).decode(u"ASCII")


    class RuleDef(KaitaiStruct):
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self.type = self._io.read_u2le()
            self.prob = self._io.read_u2le()
            self.value = self._io.read_u4le()


    class IdChunk(KaitaiStruct):
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self.ids = []
            while not self._io.is_eof():
                self.ids.append(self._root.IdDef(self._io, self, self._root))




