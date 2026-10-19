#!/usr/bin/env bash
# Links a REL's .plf into an ET_EXEC Ghidra (via REA) can decompile, with DOL calls named.
#   tools/rel_pseudo/link_rel.sh <Module> <outdir>
# -> <outdir>/<Module>.ps.elf (paired singles scalarised, see scalarise_ps.py) and
#    <outdir>/<Module>.addrs (function start addresses for REA batch_decompile).
# .text is placed at 0x80800000 (within REL24 reach of the DOL), .rodata at 0x80900000, .data after
# it; cross-REL references are parked at 0x80A00000+. Then, in REA: open_binary(<Module>.ps.elf),
# batch_decompile(addresses) in chunks of 40, export_evidence_bundle(path), and
# split.py <bundle> <dir> <Module>.ps.elf writes one .c per function.
set -euo pipefail
m=$1; out=$2; here=$(cd "$(dirname "$0")" && pwd)
P=${MP2_ARCHIVE:-/run/media/odran/Leo/projects/Restored-projects/Chatgpt/MetroidPrime2Port}
B=$P/build/binutils; plf=$P/build/G2ME01/$m/$m.plf; mkdir -p "$out"
und=$($B/powerpc-eabi-ld -o /dev/null --just-symbols=$P/build/G2ME01/main.elf -T "$here/rel.ld" \
  --unresolved-symbols=report-all "$plf" 2>&1 | grep -oP "undefined reference to \`\K[^']+" | sort -u || true)
ds=(); i=0; for s in $und; do ds+=("--defsym=$s=$(printf 0x%x $((0x80A00000 + i*0x100)))"); i=$((i+1)); done
$B/powerpc-eabi-ld -o "$out/$m.x.elf" --just-symbols=$P/build/G2ME01/main.elf -T "$here/rel.ld" "${ds[@]}" "$plf"
python3 -I - "$out/$m.x.elf" <<'PY'
import sys; p=sys.argv[1]; b=bytearray(open(p,'rb').read()); b[0x24:0x28]=b'\0'*4; open(p,'wb').write(b)
PY
set -- $($B/powerpc-eabi-objdump -h "$out/$m.x.elf" | awk '$2==".text"{print $3" "$6}')
python3 -I "$here/scalarise_ps.py" "$out/$m.x.elf" "$out/$m.ps.elf" "$2" "$1"
$B/powerpc-eabi-nm -n "$out/$m.x.elf" | awk '$1>="80800000" && $1<"80900000" && ($2=="T"||$2=="t"){print "0x"$1}' > "$out/$m.addrs"
