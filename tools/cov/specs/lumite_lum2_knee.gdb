# lum2 knee: GetKneeIndex with fake rstl::string objects (mPtr, mCow, mSize) for the empty name, every knee name and an
# unknown 'L' / 'R' name.
set $t = (CLumite*)$rdi
python
import struct
inf = gdb.selected_inferior()
def mk(s):
    b = s.encode() + b'\0'
    p = int(gdb.parse_and_eval('(long)malloc(%d)' % (len(b) + 8)))
    inf.write_memory(p, b)
    o = int(gdb.parse_and_eval('(long)malloc(64)'))
    inf.write_memory(o, struct.pack('<QQI', p, 0, len(s)))
    return o
for n in ['', 'L_front_knee', 'L_back_knee', 'L_middle_knee', 'R_front_knee', 'R_back_knee', 'R_middle_knee', 'Lx', 'Rx']:
    o = mk(n)
    print('knee %r -> %s' % (n, gdb.parse_and_eval('$t->GetKneeIndex(*(rstl::string*)%d)' % o)))
end
printf "knee done\n"
