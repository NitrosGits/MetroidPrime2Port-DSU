# Adds all five loader default/skip arms and the absent mold-token ctor arm to
# the lifecycle run. Input bytes and skipped nested scalars restore at return.
# One top-level load deliberately keeps the default actor-parameter record.
import os
import struct
import gdb

helper = os.path.join(os.path.dirname(os.environ['COV_SPEC']), 'darkcommando_dc4_lifecycle.py')
exec(compile(open(helper).read(), helper, 'exec'), globals())
LOADERS = {
    'LoadDarkCommando': (None, 0x7e397fed, None),
    'LoadTypedefDarkCommandoData': ('SLdrDarkCommandoData', 0xa4858f7d, 'lurkChance'),
    'LoadTypedefDarkCommandoEMPData': ('SLdrDarkCommandoEMPData', 0xb33a0cbc, 'preFireIdleTime'),
    'LoadTypedefDarkCommandoChargeBeamData': ('SLdrDarkCommandoChargeBeamData', 0xd258ec09,
                                          'postFireIdleTime'),
    'LoadTypedefDarkCommandoShadowDashData': ('SLdrDarkCommandoShadowDashData', 0xa0d037ee,
                                          'unknown_0xa0d037ee'),
}


class Loader(gdb.Breakpoint):
    def __init__(self, spec, name):
        super().__init__(spec, internal=True)
        self.name = name

    def stop(self):
        if count(self.name) != 0:
            return False
        typ, wanted, field = LOADERS[self.name]
        dest = '((%s*)%d)->%s' % (typ, int(ev('$rdi')), field) if field else None
        stream = '((CInputStream*)%d)->' % int(ev('$rsi'))
        start = int(ev(stream + 'mPtr'))
        end = int(ev(stream + 'mBuffer')) + int(ev(stream + 'mBufferSize'))
        data = bytes(gdb.selected_inferior().read_memory(start, end - start))
        size = int.from_bytes(data[:2], 'big')
        pos, replacement = 2, None
        for _ in range(size):
            pid, length = struct.unpack_from('>IH', data, pos)
            if pid == wanted:
                if field:
                    assert length == 4
                replacement = (pos, struct.unpack_from('>f', data, pos + 6)[0] if field else None)
            pos += 6 + length
            assert pos <= len(data)
        assert replacement is not None, self.name
        off, value = replacement
        gdb.selected_inferior().write_memory(start + off, b'\xfe\xed\xbe\xef')

        def finish():
            reached = int(ev(stream + 'mPtr'))
            log(self.name + ' unknown property', '%s consumed=%d expected=%d' %
                ('PASS' if reached == start + pos else 'FAIL', reached - start, pos))
            gdb.selected_inferior().write_memory(start + off, data[off:off + 4])
            if dest:
                put(dest, repr(value))
        Done(finish)
        return False


class CommandoCtor(gdb.Breakpoint):
    def stop(self):
        if count('ctor') != 0:
            return False
        actor = '((CDarkCommando*)%d)->' % int(ev('$rdi'))
        # SysV: this/uid/name/info/xf/model in GPRs; actorParams/patterned/data on stack.
        data = '((SLdrDarkCommandoData*)*(unsigned long*)($rsp+24))->chargeBeamAttackInfo.moldEffect'
        address = int(ev('&' + data))
        field = '*(unsigned int*)%d' % address
        before = int(ev(field))
        put(field, '0xffffffff')

        def finish():
            log('6FA0 absent mold', 'PASS' if not bool(ev(actor + 'mMoldEffect.m_valid')) else 'FAIL')
            put(field, before)
            # The owned optional deliberately remains absent; the actor is a valid
            # no-mold variant. Restore the source data record, which belongs to the loader.
        Done(finish)
        return False


for line in open(os.environ['COV_NM']):
    raw, _, demangled = line.rstrip().partition('\t')
    parts = raw.split()
    if len(parts) != 3 or '.cold' in demangled or 'clone' in demangled or parts[0] in seen:
        continue
    prefix = demangled.split('(')[0]
    if prefix in LOADERS:
        Loader('*' + parts[2], prefix)
    elif prefix == 'CDarkCommando::CDarkCommando':
        CommandoCtor('*' + parts[2], internal=True)
    else:
        continue
    seen.add(parts[0])
