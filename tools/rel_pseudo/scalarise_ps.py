# Rewrites Gekko paired-single instructions in an ELF's .text into the scalar FP form Ghidra's
# (Kept from the Ing-family work; used by tools/rel_pseudo/link_rel.sh.)
# generic PowerPC decodes (ps0 lane only). For reading control flow; vector math is approximate.
import sys,struct
src,dst,text_off,text_size=sys.argv[1],sys.argv[2],int(sys.argv[3],16),int(sys.argv[4],16)
b=bytearray(open(src,'rb').read()); n=0; hits=[]
A59={18,20,21,24,25,28,29,30,31}; A63={23,26}
for o in range(text_off,text_off+text_size,4):
  w=struct.unpack('>I',b[o:o+4])[0]; op=w>>26; nw=None
  if op in (56,57,60,61):
    d=w&0xfff; d = d-0x1000 if d&0x800 else d
    nw=({56:48,57:49,60:52,61:53}[op]<<26)|(w&0x03ff0000)|(d&0xffff)
  elif op==4:
    x6=(w>>1)&0x3f; x5=(w>>1)&0x1f; x10=(w>>1)&0x3ff
    if x6 in (6,7,38,39):
      nw=(31<<26)|(w&0x03fff800)|({6:535,7:663,38:567,39:695}[x6]<<1)
    elif x5 in A59: nw=(59<<26)|(w&0x03ffffff)
    elif x5 in A63: nw=(63<<26)|(w&0x03ffffff)
    elif x5 in (10,11): nw=(59<<26)|(w&0x03fff83f&~0x3e)|(21<<1)|(w&1)  # sum -> fadds
    elif x5 in (12,13): nw=(59<<26)|(w&0x03ffffc1&~0x3e)|(25<<1)|(w&1)
    elif x5 in (14,15): nw=(59<<26)|(w&0x03ffffc1&~0x3e)|(29<<1)|(w&1)
    elif x10 in (0,32,64,96): nw=(63<<26)|(w&0x03fff800)|({0:0,32:32,64:0,96:32}[x10]<<1)
    elif x10 in (40,72,136,264): nw=(63<<26)|(w&0x03e0f801)|(x10<<1)
    elif x10 in (528,560,592,624):
      fa=(w>>16)&0x1f; nw=(63<<26)|(w&0x03e00000)|(fa<<11)|(72<<1)|(w&1)
    elif x10==1014: nw=0x60000000
  if nw is not None and nw!=w:
    b[o:o+4]=struct.pack('>I',nw); n+=1; hits.append(o-text_off)
open(dst,'wb').write(b); open(dst+'.pshits','w').write('\n'.join('%x'%h for h in hits)); print('patched',n)
