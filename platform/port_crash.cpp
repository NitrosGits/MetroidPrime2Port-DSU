#include "port_crash.h"
#include "port_env.h"

#include "port_build_info.h"
#include "port_breadcrumb.h"
#include "port_logfile.h"

#include <atomic>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <exception>
#include <string>
#include <typeinfo>

#include <cxxabi.h>
#include <dlfcn.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <pthread.h>
#include <sys/ucontext.h> // <ucontext.h> refuses to compile there without _XOPEN_SOURCE
#else
#include <sys/syscall.h>
#include <ucontext.h>
#endif
#include <unwind.h>

namespace PortCrash {
namespace {
// Set by the first crash; a second one (another thread, or the report itself
// crashing) skips the report and goes straight on to the system.
std::atomic< bool > sCrashing{false};

// Set by main on Android: waits for the stderr pump to empty and writes the crash buffer line.
void (*sAfterReport)(int, const void*) = nullptr;

constexpr int kMaxFrames = 64;

// One line of the report, made without the heap or stdio, which the crash may
// have left broken.
struct Line {
  char text[1024];
  size_t used = 0;

  Line& Add(const char* s) {
    while (s != nullptr && *s != '\0' && used + 1 < sizeof(text)) {
      text[used++] = *s++;
    }
    return *this;
  }
  Line& Hex(uintptr_t value) {
    char digits[2 * sizeof(value)];
    int n = 0;
    do {
      digits[n++] = "0123456789abcdef"[value & 15];
      value >>= 4;
    } while (value != 0);
    Add("0x");
    while (n > 0 && used + 1 < sizeof(text)) {
      text[used++] = digits[--n];
    }
    return *this;
  }
  Line& Dec(unsigned long value) {
    char digits[24];
    int n = 0;
    do {
      digits[n++] = static_cast< char >('0' + value % 10);
      value /= 10;
    } while (value != 0);
    while (n > 0 && used + 1 < sizeof(text)) {
      text[used++] = digits[--n];
    }
    return *this;
  }
};

const char* BaseName(const char* path) {
  const char* name = path;
  for (const char* c = path; *c != '\0'; ++c) {
    if (*c == '/' || *c == '\\') {
      name = c + 1;
    }
  }
  return name;
}

void CrashTest() {
  const char* test = port::EnvString("MP2_CRASH_TEST");
  if (test == nullptr) {
    return;
  }
  if (std::strcmp(test, "segv") == 0) {
    volatile int* volatile nowhere = nullptr;
    *nowhere = 1;
  } else if (std::strcmp(test, "abort") == 0) {
    std::abort();
  }
}

// The handlers that were there before, which the signal goes on to: the default
// one (a core dump), or on Android the one that writes the tombstone.
constexpr int kSignals[] = {SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGABRT, SIGTRAP};
constexpr int kSignalCount = sizeof(kSignals) / sizeof(kSignals[0]);
// What RequestStack sends; not a crash, so it has no previous handler to chain to.
constexpr int kStackSignal = SIGUSR2;
struct sigaction sPrevious[kSignalCount];

void Emit(Line& line) {
  line.text[line.used++] = '\n';
  PortLogFile::WriteRaw(line.text, line.used);
}

const char* SignalName(int signal) {
  switch (signal) {
  case SIGSEGV:
    return "SIGSEGV (bad memory access)";
  case SIGBUS:
    return "SIGBUS (bad memory access)";
  case SIGFPE:
    return "SIGFPE (arithmetic error)";
  case SIGILL:
    return "SIGILL (illegal instruction)";
  case SIGABRT:
    return "SIGABRT (abort() was called)";
  case SIGTRAP:
    return "SIGTRAP (trap)";
  default:
    return "signal";
  }
}

// "module+0xoffset (symbol+0xoffset)". The offset is from the module's load
// address, which is what addr2line -e <module> takes. Symbols show only for
// exported functions (Android's system libraries, mostly).
void Describe(Line& line, uintptr_t pc) {
  Dl_info info{};
  if (dladdr(reinterpret_cast< void* >(pc), &info) != 0 && info.dli_fname != nullptr) {
    line.Add(BaseName(info.dli_fname)).Add("+").Hex(pc - reinterpret_cast< uintptr_t >(info.dli_fbase));
    if (info.dli_sname != nullptr) {
      line.Add(" (").Add(info.dli_sname).Add("+").Hex(pc - reinterpret_cast< uintptr_t >(info.dli_saddr)).Add(")");
    }
  } else {
    line.Hex(pc);
  }
}

uintptr_t FaultAddress(void* context) {
  const auto* user = static_cast< const ucontext_t* >(context);
#if defined(__APPLE__) && defined(__aarch64__)
  return static_cast< uintptr_t >(user->uc_mcontext->__ss.__pc);
#elif defined(__APPLE__) && defined(__x86_64__)
  return static_cast< uintptr_t >(user->uc_mcontext->__ss.__rip);
#elif defined(__x86_64__)
  return static_cast< uintptr_t >(user->uc_mcontext.gregs[REG_RIP]);
#elif defined(__aarch64__)
  return static_cast< uintptr_t >(user->uc_mcontext.pc);
#else
  (void)user;
  return 0;
#endif
}

struct Walk {
  const char* prefix;
  int frame;
};

_Unwind_Reason_Code OnFrame(_Unwind_Context* context, void* argument) {
  Walk& walk = *static_cast< Walk* >(argument);
  const uintptr_t pc = _Unwind_GetIP(context);
  if (pc == 0) {
    return _URC_END_OF_STACK;
  }
  Line line;
  line.Add(walk.prefix).Dec(static_cast< unsigned long >(walk.frame)).Add(" ");
  Describe(line, pc);
  Emit(line);
  return ++walk.frame < kMaxFrames ? _URC_NO_REASON : _URC_END_OF_STACK;
}

// The watchdog's request (RequestStack): runs on the thread that was asked, writes its
// stack and returns, so the game goes on. Same rules as the crash report's handler.
void OnStackSignal(int, siginfo_t*, void*) {
  const int savedErrno = errno;
  Line title;
  Emit(title.Add("watchdog: main thread stack:"));
  Walk walk{"watchdog:   #", 0};
  _Unwind_Backtrace(OnFrame, &walk);
  errno = savedErrno;
}

void OnSignal(int signal, siginfo_t* info, void* context) {
  if (!sCrashing.exchange(true)) {
    Line header;
    header.Add("port: crashed: ").Add(SignalName(signal));
    if (signal == SIGSEGV || signal == SIGBUS) {
      header.Add(" at address ").Hex(reinterpret_cast< uintptr_t >(info->si_addr));
    }
    Emit(header);
    if (const uintptr_t pc = FaultAddress(context); pc != 0) {
      Line at;
      at.Add("port: at ");
      Describe(at, pc);
      Emit(at);
    }
    Line build;
    Emit(build.Add("port: build ").Add(MP_BUILD_REVISION));
    // Starts in this handler; the frames after the signal frame are the crashed code.
    Line title;
    Emit(title.Add("port: stack:"));
    Walk walk{"port:   #", 0};
    _Unwind_Backtrace(OnFrame, &walk);
    // What the game was doing: the last breadcrumbs (room loads, state changes).
    PortBreadcrumb::Dump();
    if (sAfterReport != nullptr) {
      sAfterReport(signal, info->si_addr);
    }
  }
  for (int i = 0; i < kSignalCount; ++i) {
    if (kSignals[i] == signal) {
      sigaction(signal, &sPrevious[i], nullptr);
    }
  }
  // A fault happens again on return, now to the previous handler; a signal that
  // was sent (abort, kill) has to be sent again.
  if (info->si_code <= 0) {
    raise(signal);
  }
}

_Unwind_Reason_Code CountFrame(_Unwind_Context*, void* argument) {
  ++*static_cast< int* >(argument);
  return _URC_NO_REASON;
}
} // namespace

void Install() {
  // The main thread's stack overflowing leaves no stack for the handler.
  static char alternate[64 * 1024];
  stack_t stack{};
  stack.ss_sp = alternate;
  stack.ss_size = sizeof(alternate);
  sigaltstack(&stack, nullptr);
  // The unwinder and dladdr load what they need now, not during a crash.
  int frames = 0;
  _Unwind_Backtrace(CountFrame, &frames);
  Dl_info info{};
  dladdr(reinterpret_cast< void* >(&Install), &info);
  struct sigaction stackAction {};
  stackAction.sa_sigaction = OnStackSignal;
  stackAction.sa_flags = SA_SIGINFO | SA_RESTART;
  sigemptyset(&stackAction.sa_mask);
  sigaction(kStackSignal, &stackAction, nullptr);
  struct sigaction action {};
  action.sa_sigaction = OnSignal;
  action.sa_flags = SA_SIGINFO | SA_ONSTACK;
  sigemptyset(&action.sa_mask);
  for (int i = 0; i < kSignalCount; ++i) {
    sigaction(kSignals[i], &action, &sPrevious[i]);
  }
  CrashTest();
}

void SetAfterReport(void (*afterReport)(int signal, const void* address)) { sAfterReport = afterReport; }

bool RequestStack(long threadId) {
  // A library loaded later (a GPU driver) may have taken the signal; don't run its handler.
  struct sigaction current {};
  if (threadId == 0 || sigaction(kStackSignal, nullptr, &current) != 0 || current.sa_sigaction != OnStackSignal) {
    return false;
  }
#if defined(__APPLE__)
  // macOS has no tgkill; the id the watchdog stores there is the thread's pthread_t.
  return pthread_kill(reinterpret_cast< pthread_t >(threadId), kStackSignal) == 0;
#else
  return syscall(SYS_tgkill, getpid(), static_cast< pid_t >(threadId), kStackSignal) == 0;
#endif
}

void AbortOnException(const char* where) {
  std::string what = "not a std::exception";
  try {
    throw;
  } catch (const std::exception& e) {
    what = e.what();
  } catch (...) {
  }
  const char* type = "unknown type";
  char* demangled = nullptr;
  if (const std::type_info* info = abi::__cxa_current_exception_type(); info != nullptr) {
    int status = 0;
    demangled = abi::__cxa_demangle(info->name(), nullptr, nullptr, &status);
    type = demangled != nullptr ? demangled : info->name();
  }
  Line line;
  line.Add("port: uncaught C++ exception in ").Add(where).Add(": ").Add(type).Add(": ").Add(what.c_str());
  Emit(line);
  std::abort();
}

} // namespace PortCrash
