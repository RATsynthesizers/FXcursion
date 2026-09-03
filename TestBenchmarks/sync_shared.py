#!/usr/bin/env python
"""
Keep Shared/ byte-identical between the two firmwares.

    python sync_shared.py            report drift, change nothing
    python sync_shared.py --apply    copy audio -> interface

WHY A SCRIPT

fx_protocol.h says "DUPLICATED IN THE INTERFACE CONTROLLER - keep in sync" at
the top, in a box. That is a comment, and comments do not fail builds.

Some drift IS caught automatically: every wire struct is pinned to an exact size
with FXC_STATIC_ASSERT, and both projects now compile fx_defs.c, so a padding or
layout difference is a build error on whichever side is stale. What that does NOT
catch is the interesting case - one side gaining a new command, a new enum value,
or a changed PROTO_VERSION while every struct keeps its size. Those link and run,
and then the two boards disagree about what a byte means.

THE AUDIO PROJECT IS THE SOURCE OF TRUTH

Not arbitrary: it is the side with the host test suite. Test/test_protocol.c and
Test/test_ctrl_link.c assert the struct sizes, the command ids and the framing on
every run there, so a change made in the audio project is exercised before it can
reach the interface. A change made the other way round is not.
"""
import filecmp
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, 'FXcursion_Audio_SHIELD', 'Shared')
DST = os.path.join(HERE, 'FXcursion_Interface_SHIELD', 'Shared')

# Only the wire contract. fx_defs.c and fx_crc.c come too, because the interface
# needs the tables and the CRC, not just their declarations.
FILES = [
    'fx_defs.h',
    'fx_defs.c',
    'fx_protocol.h',
    'fx_link.h',
    'fx_link.c',
    'fx_interleave.h',
    'fx_interleave.c',
    'fx_crc.h',
    'fx_crc.c',
    # The loop transport session. BOTH boards run this same state machine, so
    # drift here is worse than drift in a struct: the sizes would still match
    # and the two sides would simply sequence a transfer differently.
    'fx_loop.h',
    'fx_loop.c',
]


def main():
    apply_it = '--apply' in sys.argv

    if not os.path.isdir(SRC):
        print('source not found: %s' % SRC)
        return 2

    os.makedirs(DST, exist_ok=True)

    drift = []
    for name in FILES:
        a = os.path.join(SRC, name)
        b = os.path.join(DST, name)

        if not os.path.isfile(a):
            print('  MISSING in audio     %s' % name)
            drift.append(name)
            continue

        if not os.path.isfile(b):
            print('  MISSING in interface %s' % name)
            drift.append(name)
        elif filecmp.cmp(a, b, shallow=False):
            print('  identical            %-16s %d bytes' % (name, os.path.getsize(a)))
            continue
        else:
            print('  DIFFERS              %-16s audio %d B, interface %d B'
                  % (name, os.path.getsize(a), os.path.getsize(b)))
            drift.append(name)

        if apply_it:
            shutil.copy2(a, b)
            print('     -> copied')

    if not drift:
        print('\nin sync')
        return 0

    if apply_it:
        print('\n%d file(s) synced. Rebuild BOTH projects: the struct-size'
              ' assertions only fire at compile time.' % len(drift))
        return 0

    print('\n%d file(s) out of sync. Re-run with --apply to copy'
          ' audio -> interface.' % len(drift))
    return 1


if __name__ == '__main__':
    sys.exit(main())
