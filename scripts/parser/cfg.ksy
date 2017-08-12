meta:
  id: dragon_cfg
  file-extension: cfg
  endian: le

seq:
  - id: grammar_type
    type: u4
  - id: grammar_flags
    type: u4
  - id: chunks
    type: chunk
    repeat: eos

types:
  chunk:
    seq:
      - id: type
        type: u4
        enum: chunk_type
      - id: size
        type: u4
      - id: defs
        size: size
        type:
          switch-on: type
          cases:
            chunk_type::rules: rule_chunk
            chunk_type::words: id_chunk
            chunk_type::lists: id_chunk
            chunk_type::imports: id_chunk
            chunk_type::exports: id_chunk

  rule_chunk:
    seq:
      - id: rules
        type: rule
        repeat: eos
  rule:
    seq:
      - id: size
        type: u4
      - id: rule_num
        type: u4
      - id: rdefs
        type: rule_def
        repeat: expr
        repeat-expr: (size - 8) / 8
  rule_def:
    seq:
      - id: type
        type: u2
      - id: prob
        type: u2
      - id: value
        type: u4

  id_chunk:
    seq:
      - id: ids
        type: id_def
        repeat: eos
  id_def:
    seq:
      - id: size
        type: u4
      - id: id
        type: u4
      - id: name
        type: strz
        size: size - 8
        encoding: ASCII

enums:
  chunk_type:
    2: words
    3: rules
    4: exports
    5: imports
    6: lists
  cfg_elem:
    1: start
    2: end
    3: word
    4: rule
    5: list
  cfg_range:
    1: seq
    2: alt
    3: rep
    4: opt
