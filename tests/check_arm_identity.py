#!/usr/bin/env python3
"""Read the actual factory GUID word from an ELF32 ARM relocatable object."""
import json
from pathlib import Path
import struct
import sys


def inspect(path):
    data = Path(path).read_bytes()
    assert data[:7] == b'\x7fELF\x01\x01\x01'
    header = struct.unpack_from('<HHIIIIIHHHHHH', data, 16)
    assert header[0] == 1 and header[1] == 40, 'Expected ARM REL'
    offset, stride, count = header[5], header[10], header[11]
    sections = [struct.unpack_from('<IIIIIIIIII', data, offset+i*stride) for i in range(count)]
    found = []
    for s in sections:
        if s[1] != 2:
            continue
        strings = sections[s[6]]
        names = data[strings[4]:strings[4]+strings[5]]
        for pos in range(s[4], s[4]+s[5], s[9]):
            name, value, size, info, other, index = struct.unpack_from('<IIIBBH', data, pos)
            symbol = names[name:].split(b'\0', 1)[0].decode()
            if symbol.endswith('7factoryE'):
                section = sections[index]
                assert value+4 <= section[5] and size >= 4
                guid, = struct.unpack_from('<I', data, section[4]+value)
                found.append(dict(symbol=symbol, section=index, sectionOffset=value,
                                  fileOffset=section[4]+value, guid=f'0x{guid:08x}'))
                assert guid == 0x70506854, 'Expected ThPp factory identity'
    assert len(found) == 1, f'Expected one Pop factory, got {found}'
    return dict(object=str(path), factory=found[0], expectedBytes='54 68 50 70', status='PASS')


if __name__ == '__main__':
    print(json.dumps(inspect(sys.argv[1]), indent=2))
