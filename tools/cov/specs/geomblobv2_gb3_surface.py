# The shipped game never creates CSurfaceParticleEffect. This spec schedules
# calls to its real factory/methods in place of selected tracker Think calls.
# x86-64 SysV only. Each redirected call gets a separate aligned stack; its
# return breakpoint restores the caller's stack. The scratch actor is never
# inserted into the world and is destroyed before the spec disables itself.
# force.sh + cov_force.py; build-port-cov; Ing area; --frames 2400 --fast.
import os
import gdb

CLASS = 'CCollisionTracker'
SPEC = []
FORCE = []

if not hasattr(gdb, '_gb3_surface_installed'):
    gdb._gb3_surface_installed = True
    symbols = []
    for line in open(os.environ['COV_NM']):
        parts = line.rstrip().split('\t')
        if len(parts) == 2:
            fields = parts[0].split()
            if len(fields) == 3:
                symbols.append((fields[2], parts[1]))

    def symbol(prefix):
        return next(m for m, d in symbols if d.startswith(prefix) and 'clone' not in d)

    def address(prefix):
        return int(gdb.parse_and_eval("&'%s'" % symbol(prefix)))

    def cmd(text):
        gdb.execute(text, to_string=True)

    def ev(text):
        return int(gdb.parse_and_eval(text))

    def log(text):
        with open(os.path.join(os.environ['COV_OUT'], 'surface.log'), 'a') as f:
            f.write(text + '\n')

    class Return(gdb.Breakpoint):
        def __init__(self, pc, sp, callback):
            super().__init__('*%d' % pc, internal=True, temporary=True)
            self.saved_sp, self.callback = sp, callback

        def stop(self):
            self.enabled = False
            cmd('set $rsp = %d' % (self.saved_sp + 8))
            if self.callback:
                self.callback()
            return False

    def invoke(prefix, args, callback=None):
        sp = ev('$rsp')
        ret = ev('*(unsigned long*)$rsp')
        # Keep entry SP == 8 mod 16, including the seventh/eighth arguments.
        new_sp = sp - 65536
        cmd('set *(unsigned long*)%d = %d' % (new_sp, ret))
        values = [ev(str(arg)) for arg in args]
        for reg, value in zip(('rdi', 'rsi', 'rdx', 'rcx', 'r8', 'r9'), values):
            cmd('set $%s = %d' % (reg, value))
        for i, value in enumerate(values[6:]):
            cmd('set *(unsigned long*)%d = %d' % (new_sp + 8 * (i + 1), value))
        Return(ret, sp, callback)
        cmd('set $rsp = %d' % new_sp)
        cmd('set $rip = %d' % address(prefix))

    class Surface(gdb.Breakpoint):
        stage = 0
        effect = 0

        def created(self):
            self.effect = ev('$rax')
            cmd('set $gb3_spe = (CSurfaceParticleEffect*)%d' % self.effect)
            cmd('set $gb3_spe->mGeneratorRate = 0.0')
            cmd('set $gb3_spe->mParticleGen.mPtr->mEnableADV = 1')
            log('6750 factory constructed private SPE; ADV enabled for its surface records')

        def stop(self):
            try:
                tracker = ev('$rdi')
                mgr = ev('$rsi')
                cmd('set $gb3_ct = (CCollisionTracker*)%d' % tracker)
                if self.stage == 0:
                    cmd('set $gb3_name = (rstl::string*)($rsp - 2048)')
                    cmd('set $gb3_name->mPtr = (char*)%d' % address(
                        'rstl::basic_string<char, rstl::char_traits<char>, rstl::rmemory_allocator>::mNull'))
                    cmd('set $gb3_name->mCow = 0')
                    cmd('set $gb3_name->mSize = 0')
                    invoke('CreateSurfaceParticleEffect(', [
                        '&$gb3_ct->mParticleSystem.mPtr->mGenDesc', 65535,
                        '$gb3_ct->mAreaId.value', 1, '$gb3_name',
                        '&$gb3_ct->mTransform', 65535, 0], self.created)
                elif self.stage == 1:
                    invoke('CSurfaceParticleEffect::PreRenderAllViewports(', [self.effect, mgr])
                    log('6518 empty particle bounds')
                elif self.stage in (2, 3):
                    # Exact symbol suffix distinguishes these identical accessors.
                    prefix = 'CSurfaceParticleEffect::GetParticleGen()' + (' const' if self.stage == 3 else '')
                    invoke(prefix, [self.effect])
                    m = next(m for m, d in symbols if d == prefix)
                    cmd("set $rip = (unsigned long)&'%s'" % m)
                    log('49A4/49AC accessor stage %d' % self.stage)
                elif self.stage == 4:
                    cmd('set $xmm0.v4_float[0] = 0.016666667')
                    invoke('CSurfaceParticleEffect::Think(', [self.effect, mgr])
                    log('4BEC zero-rate early return')
                elif self.stage in (5, 302, 303, 304):
                    cmd('set $gb3_msg = (CScriptMsg*)($rsp - 2048)')
                    for member in ('mSenderId', 'm_originator', 'm_id'):
                        cmd('set $gb3_msg->%s.value = 65535' % member)
                    cmd('set $gb3_msg->m_state = kSS_InvalidState')
                    msg = 'kSM_Create' if self.stage in (5, 304) else 'kSM_Delete'
                    cmd('set $gb3_msg->m_msg = ' + msg)
                    if self.stage == 5:
                        cmd('set $gb3_spe->mGeneratorRate = 1.0')
                        cmd('set $gb3_spe->mParticleGen.mPtr->mLightType = CElementGen::kLT_Custom')
                    elif self.stage == 304:
                        cmd('set $gb3_spe->mParticleGen.mPtr->mLightType = CElementGen::kLT_None')
                    invoke('CSurfaceParticleEffect::AcceptScriptMsg(', [self.effect, mgr, '$gb3_msg'])
                    log('49E4 ' + msg)
                elif self.stage == 300:
                    invoke('CSurfaceParticleEffect::PreRenderAllViewports(', [self.effect, mgr])
                    log('6518 populated particle bounds')
                elif self.stage == 301:
                    cmd('set $gb3_msg = (CScriptMsg*)($rsp - 2048)')
                    for member in ('mSenderId', 'm_originator', 'm_id'):
                        cmd('set $gb3_msg->%s.value = 65535' % member)
                    cmd('set $gb3_msg->m_state = kSS_InvalidState')
                    cmd('set $gb3_msg->m_msg = kSM_Deactivate')
                    invoke('CSurfaceParticleEffect::AcceptScriptMsg(', [self.effect, mgr, '$gb3_msg'])
                    log('49E4 default Deactivate')
                elif self.stage == 305:
                    # Deleting destructor (D0) releases the factory's allocation.
                    m = next(m for m, d in symbols if d == 'CSurfaceParticleEffect::~CSurfaceParticleEffect()' and 'D0' in m)
                    invoke('CSurfaceParticleEffect::~CSurfaceParticleEffect()', [self.effect])
                    cmd("set $rip = (unsigned long)&'%s'" % m)
                    self.enabled = False
                    log('66B0 destroying private SPE; scheduled calls complete')
                else:
                    cmd('set $xmm0.v4_float[0] = 0.016666667')
                    invoke('CSurfaceParticleEffect::Think(', [self.effect, mgr])
                    if self.stage == 6:
                        log('4BEC advancing emitted surface particles for 294 calls')
                self.stage += 1
            except Exception as exc:
                log('FAILED stage %d: %s' % (self.stage, exc))
                self.enabled = False
            return False

    class Normal(gdb.Breakpoint):
        n = 0

        def stop(self):
            self.n += 1
            cmd('set $gb3_normal = (CVector3f*)($rsp - 16384)')
            # +X reaches the fallback tangent. Non-unit +2X tests the first
            # explicit clamp. The second clamp is dead: see the module proof.
            cmd('set $gb3_normal->mX = %s' % ('1.0' if self.n == 1 else '2.0'))
            cmd('set $gb3_normal->mY = 0.0')
            cmd('set $gb3_normal->mZ = 0.0')
            cmd('set $rdi = $gb3_normal')
            log('61C4 forced normal case %d (+X, +2X)' % self.n)
            if self.n == 2:
                self.enabled = False
            return False

    if not globals().get('GB3_SURFACE_LIBRARY', False):
        Normal('*' + symbol('BuildOrientationFromNormal('), internal=True)
        Surface('*' + symbol('CCollisionTracker::Think('), internal=True)
