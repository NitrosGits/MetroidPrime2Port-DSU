import json,sys,os
b=json.load(open(sys.argv[1])); out=sys.argv[2]; subj=sys.argv[3]; os.makedirs(out,exist_ok=True)
ps=set(int(x,16) for x in open(sys.argv[4]).read().split()) if len(sys.argv)>4 else set()
n=0; fails=[]
for r in b['records']:
  if r.get('subject',{}).get('name')!=subj or r.get('operation')!='batch_decompile': continue
  nr=r.get('normalized_result')
  if not isinstance(nr,dict): continue
  for it in nr.get('items',[]):
    p=it.get('pseudocode'); name=(it.get('procedure') or {}).get('name') or it['address']
    if not p: fails.append(name); continue
    p=p.replace('0x8080','0x8080')
    open(os.path.join(out,name+'.c'),'w').write(p); n+=1
print(subj, n, 'fails', fails[:10], 'baddata', sum('halt_baddata' in open(os.path.join(out,f)).read() for f in os.listdir(out)))
