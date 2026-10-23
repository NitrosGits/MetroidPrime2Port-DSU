#include "port_logfile.h"
#include "port_settings.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>

#if defined(__ANDROID__)
#include <SDL3/SDL.h>

#include <mutex>
#endif

#if !defined(_WIN32)
#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#endif
#if !defined(_WIN32) && !defined(__ANDROID__)
#include <poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#endif

namespace PortLogFile {

namespace {

constexpr const char* kFileName = "metroid_prime2_port.log";
// A run stops at 64 MiB: a spinning log must not fill the disk, and a truncated one still says
// why. The next run rotates it aside and starts a fresh file.
constexpr long long kMaxLogBytes = 64LL * 1024 * 1024;

bool sEnabled = true; // `MP2_LOG_FILE=0` turns it off; on unless set
bool sStarted = false;
std::string sPath;

#if !defined(_WIN32)
void WriteAll(int fd, const char* data, ssize_t size) {
  while (size > 0) {
    const ssize_t n = write(fd, data, static_cast< size_t >(size));
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      return;
    }
    data += n;
    size -= n;
  }
}
#endif

#if defined(__ANDROID__)
// The open log file. Every line is one write() of its own, so whatever was logged before a
// crash or a kill by Android is already in the file.
int sFile = -1;
std::mutex sMutex;
long long sWritten = 0;
bool sTruncated = false;
#endif

#if !defined(_WIN32) && !defined(__ANDROID__)
// Writes to the terminal, dropping what it will not take. The terminal copy is
// opened non-blocking (or is a socket, sent to with MSG_DONTWAIT), so a terminal
// that stops reading - Ctrl+S, a pager left waiting - loses its copy of the
// output instead of filling the pipe and stalling every write the game makes. A
// reader that is only slow gets kTerminalWaitMs to make room; one that has not
// is skipped without waiting until it takes something again. (From MP1.)
constexpr int kTerminalWaitMs = 100;
bool sTerminalStalled = false;

void WriteTerminal(int fd, const char* data, ssize_t size, bool socket) {
  int waits = 0; // without progress, in case "ready" is not followed by room
  while (size > 0) {
    const ssize_t n = socket ? send(fd, data, static_cast< size_t >(size), MSG_DONTWAIT)
                             : write(fd, data, static_cast< size_t >(size));
    if (n >= 0) {
      sTerminalStalled = false;
      waits = 0;
      data += n;
      size -= n;
      continue;
    }
    if (errno == EINTR) {
      continue;
    }
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      return;
    }
    pollfd descriptor{};
    descriptor.fd = fd;
    descriptor.events = POLLOUT;
    if (++waits > 8 || poll(&descriptor, 1, sTerminalStalled ? 0 : kTerminalWaitMs) <= 0) {
      sTerminalStalled = true;
      return; // this much never reaches the terminal; the file has it
    }
  }
}

// A descriptor for the terminal copy of the log, of its own so that making it
// non-blocking leaves the shell's descriptor (which a tty's stdin, stdout and
// stderr usually all share) alone. `socket` is set when it is a socket, which
// is written per call without blocking instead. Falls back to a plain,
// blocking dup when no such descriptor can be had.
int OpenTerminal(bool& socket) {
  socket = false;
  struct stat info {};
  if (fstat(STDERR_FILENO, &info) == 0) {
    const char* reopen = nullptr;
    if (isatty(STDERR_FILENO)) {
      reopen = ttyname(STDERR_FILENO);
#if defined(__linux__)
    } else if (S_ISFIFO(info.st_mode)) {
      reopen = "/proc/self/fd/2"; // a new open of the same pipe
#endif
    } else if (S_ISSOCK(info.st_mode)) {
      socket = true;
    }
    if (reopen != nullptr) {
      const int fd = open(reopen, O_WRONLY | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
      if (fd >= 0) {
        return fd;
      }
    }
  }
  return dup(STDERR_FILENO);
}

// The copying process: pipe -> terminal + file until every writer has gone. Only
// async-signal-safe calls, since the game may already have threads when it forks, and its stdin
// is left alone. The terminal gets the output as it is; the file gets the same bytes.
[[noreturn]] void Copy(int in, int terminal, bool terminalSocket, int file) {
  // Ctrl+C in the terminal reaches this process too; it must keep draining until the game has
  // printed its last line.
  signal(SIGINT, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
  long long written = 0;
  bool truncated = false;
  char buffer[8192];
  for (;;) {
    const ssize_t n = read(in, buffer, sizeof(buffer));
    if (n < 0 && errno == EINTR) {
      continue;
    }
    if (n <= 0) {
      break;
    }
    if (terminal >= 0) {
      WriteTerminal(terminal, buffer, n, terminalSocket);
    }
    // Past the cap the pipe is still drained - the game must never block on it - but the file
    // stops growing.
    if (written + n <= kMaxLogBytes) {
      WriteAll(file, buffer, n);
      written += n;
    } else if (!truncated) {
      truncated = true;
      static const char kNote[] =
          "port: the log reached 64 MiB and was truncated; the next run starts a fresh log\n";
      WriteAll(file, kNote, sizeof(kNote) - 1);
    }
  }
  _exit(0);
}
#endif

#if !defined(_WIN32)
// Rotates the previous run's log aside, and makes the directory it lives in.
void PreparePath(const std::string& path) {
  if (path.empty()) {
    return;
  }
  std::error_code ec;
  const std::filesystem::path file(path);
  // On a first start the user folder may not exist yet, and that run is the one most likely to
  // need a log.
  std::filesystem::create_directories(file.parent_path(), ec);
  if (std::filesystem::exists(file, ec)) {
    std::filesystem::path old = file;
    old.replace_extension(".old.log");
    std::filesystem::rename(file, old, ec);
  }
}
#endif

} // namespace

void Start(const std::string& userDirectory) {
  if (sStarted) {
    return; // once per run
  }
  // MP2_LOG_FILE, else `log_file` in settings.cfg (PortSettings::Initialize has run by now).
  sEnabled = PortSettings::LogFileEnabled();
  if (!sEnabled) {
    return;
  }
#if defined(_WIN32)
  // This tree builds the port on Linux and Android only; the file stays compilable here.
  std::fprintf(stderr, "port: the log file is not written on Windows in this build\n");
  sEnabled = false;
#elif defined(__ANDROID__)
  const char* external = SDL_GetAndroidExternalStoragePath();
  sPath = (external != nullptr && external[0] != '\0')
              ? std::string(external) + "/" + kFileName
              : (userDirectory.empty() ? std::string() : userDirectory + "/" + kFileName);
  if (sPath.empty()) {
    return; // nowhere to write it
  }
  PreparePath(sPath);
  const int out =
      open(sPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_APPEND | O_CLOEXEC, 0644);
  if (out < 0) {
    std::fprintf(stderr, "port: cannot write the log file %s\n", sPath.c_str());
    sPath.clear();
    sEnabled = false;
    return;
  }
  sFile = out;
  sStarted = true;
  std::fprintf(stderr, "port: writing the log to %s\n", sPath.c_str());
#else
  sPath = userDirectory.empty() ? std::string() : userDirectory + "/" + kFileName;
  if (sPath.empty()) {
    return; // no user directory to write into
  }
  PreparePath(sPath);
  const int out = open(sPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
  if (out < 0) {
    std::fprintf(stderr, "port: cannot write the log file %s; the streams are unchanged\n",
                 sPath.c_str());
    sPath.clear();
    sEnabled = false;
    return;
  }
  int fds[2];
  // Where stderr went before the redirect (the terminal, or a file a script redirected it to),
  // so the output still shows up there as well as in the file. Duplicated before the fork: the
  // parent redirects fd 2 immediately, and the child must not race it.
  bool terminalSocket = false;
  const int terminal = OpenTerminal(terminalSocket);
  std::fflush(stdout);
  std::fflush(stderr);
  if (pipe(fds) != 0) {
    std::fprintf(stderr, "port: cannot make the log pipe; the streams are unchanged\n");
    close(out);
    if (terminal >= 0) {
      close(terminal);
    }
    sPath.clear();
    sEnabled = false;
    return;
  }
  const pid_t child = fork();
  if (child == 0) {
    close(fds[1]);
    Copy(fds[0], terminal, terminalSocket, out);
  }
  close(fds[0]);
  close(out);
  if (terminal >= 0) {
    close(terminal);
  }
  if (child < 0) {
    close(fds[1]);
    std::fprintf(stderr, "port: cannot fork the log copier; the streams are unchanged\n");
    sPath.clear();
    sEnabled = false;
    return;
  }
  dup2(fds[1], STDOUT_FILENO);
  dup2(fds[1], STDERR_FILENO);
  close(fds[1]);
  // stdout to a pipe is fully buffered and abort() flushes nothing: a line at a time, and
  // stderr straight out, keeps the last messages before a crash.
  std::setvbuf(stdout, nullptr, _IOLBF, 0);
  std::setvbuf(stderr, nullptr, _IONBF, 0);
  sStarted = true;
  // Through the pipe, so this line is in the file as well as on the terminal.
  std::fprintf(stderr, "port: writing the log to %s\n", sPath.c_str());
#endif
}

bool Enabled() {
  return sEnabled;
}

std::string Path() {
  return sEnabled ? sPath : std::string();
}

void Write(const std::string& line) {
#if defined(__ANDROID__)
  if (!sStarted || sFile < 0) {
    return;
  }
  std::lock_guard< std::mutex > guard(sMutex);
  if (sWritten >= kMaxLogBytes) {
    if (!sTruncated) {
      sTruncated = true;
      static const char kNote[] =
          "port: the log reached 64 MiB and was truncated; the next run starts a fresh log\n";
      WriteAll(sFile, kNote, sizeof(kNote) - 1);
    }
    return;
  }
  const std::string message = line + "\n";
  WriteAll(sFile, message.data(), static_cast< ssize_t >(message.size()));
  sWritten += static_cast< long long >(message.size());
#else
  (void)line; // the copier process writes the bytes itself
#endif
}

void WriteRaw(const char* data, size_t size) {
#if !defined(_WIN32)
  // stderr: on the desktop the copier's pipe (or the terminal when the file is off), on Android
  // the pump that feeds logcat and the file. One write(2), no lock and no heap: signal handlers
  // call this.
  WriteAll(STDERR_FILENO, data, static_cast< ssize_t >(size));
#else
  std::fwrite(data, 1, size, stderr);
#endif
}

} // namespace PortLogFile
