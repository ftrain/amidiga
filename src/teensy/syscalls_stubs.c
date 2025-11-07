// Syscall stubs for Teensy 4.1 (bare metal embedded system)
// These provide minimal implementations for syscalls that Lua's I/O library expects
// but which aren't available on bare-metal Teensy.

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/times.h>
#include <errno.h>

// File operations - return errors since we don't have a filesystem yet
int _open(const char *name, int flags, int mode) {
    (void)name; (void)flags; (void)mode;
    errno = ENOSYS;  // Function not implemented
    return -1;
}

int _close(int file) {
    (void)file;
    errno = EBADF;
    return -1;
}

int _unlink(const char *name) {
    (void)name;
    errno = ENOSYS;
    return -1;
}

int _link(const char *old, const char *new) {
    (void)old; (void)new;
    errno = ENOSYS;
    return -1;
}

int _stat(const char *file, struct stat *st) {
    (void)file;
    st->st_mode = S_IFCHR;  // Character device
    return 0;
}

int _fstat(int file, struct stat *st) {
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file) {
    (void)file;
    return 1;  // Pretend everything is a tty
}

// Process operations - return dummy values
int _getpid(void) {
    return 1;
}

int _kill(int pid, int sig) {
    (void)pid; (void)sig;
    errno = EINVAL;
    return -1;
}

// Timing - return dummy values
clock_t _times(struct tms *buf) {
    (void)buf;
    return -1;
}
