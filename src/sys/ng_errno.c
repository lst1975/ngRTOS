/*************************************************************************************
 *                               ngRTOS Kernel V2.0.1
 * Copyright (C) 2022 Songtao Liu, 980680431@qq.com.  All Rights Reserved.
 **************************************************************************************
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * THE ABOVE COPYRIGHT NOTICE AND THIS PERMISSION NOTICE SHALL BE INCLUDED IN ALL
 * COPIES OR SUBSTANTIAL PORTIONS OF THE SOFTWARE. WHAT'S MORE, A DECLARATION OF 
 * NGRTOS MUST BE DISPLAYED IN THE FINAL SOFTWARE OR PRODUCT RELEASE. NGRTOS HAS 
 * NOT ANY LIMITATION OF CONTRIBUTIONS TO IT, WITHOUT ANY LIMITATION OF CODING STYLE, 
 * DRIVERS, CORE, APPLICATIONS, LIBRARIES, TOOLS, AND ETC. ANY LICENSE IS PERMITTED 
 * UNDER THE ABOVE LICENSE. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF 
 * ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO 
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES 
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
 * IN THE SOFTWARE.
 *
 *************************************************************************************
 *                              https://www.ngRTOS.org
 *                              https://github.com/ngRTOS
 **************************************************************************************
 */

#if configENABLE_ERRNO
const char* __errtable[]={
  /* The following errno values are used by ngRTOS.*/
  [ng_err_NONE]         = "OK",
  [ng_err_EPERM]        = "Operation not permitted",
  [ng_err_ENOENT]       = "No such file or directory",
  [ng_err_ESRCH]        = "No such process",
  [ng_err_EINTR]        = "Interrupted system call",
  [ng_err_EIO]          = "I/O error",
  [ng_err_ENXIO]        = "No such device or address",
  [ng_err_E2BIG]        = "Argument list too long",
  [ng_err_ENOEXEC]      = "Exec format error",
  [ng_err_EBADF]        = "Bad file number",
  [ng_err_ECHILD]       = "No child processes",
  [ng_err_EAGAIN]       = "Try again",
  [ng_err_ENOMEM]       = "Out of memory",
  [ng_err_EACCES]       = "Permission denied",
  [ng_err_EFAULT]       = "Bad address",
  [ng_err_ENOTBLK]      = "Block device required",
  [ng_err_EBUSY]        = "Device or resource busy",
  [ng_err_EEXIST]       = "File exists",
  [ng_err_EXDEV]        = "Cross-device link",
  [ng_err_ENODEV]       = "No such device",
  [ng_err_ENOTDIR]      = "Not a directory",
  [ng_err_EISDIR]       = "Is a directory",
  [ng_err_EINVAL]       = "Invalid argument",
  [ng_err_ENFILE]       = "File table overflow",
  [ng_err_EMFILE]       = "Too many open files",
  [ng_err_ENOTTY]       = "Not a typewriter",
  [ng_err_ETXTBSY]      = "Text file busy",
  [ng_err_EFBIG]        = "File too large",
  [ng_err_ENOSPC]       = "No space left on device",
  [ng_err_ESPIPE]       = "Illegal seek",
  [ng_err_EROFS]        = "Read-only file system",
  [ng_err_EMLINK]       = "Too many links",
  [ng_err_EPIPE]        = "Broken pipe",
  [ng_err_EDOM]         = "Math argument out of domain of func",
  [ng_err_ERANGE]       = "Math result not representable",
  
  [ng_err_EDEADLK]      = "Resource deadlock would occur",
  [ng_err_ENAMETOOLONG] = "File name too long",
  [ng_err_ENOLCK]       = "No record locks available",

/*
 * This error code is special: arch syscall entry code will return
 * -ENOSYS if users try to call a syscall that doesn't exist.  To keep
 * failures of syscalls that really do exist distinguishable from
 * failures due to attempts to use a nonexistent syscall, syscall
 * implementations should refrain from returning -ENOSYS. 
 */
  [ng_err_ENOSYS]       = "Invalid system call number",

  [ng_err_ENOTEMPTY]    = "Directory not empty",
  [ng_err_ELOOP]        = "Too many symbolic links encountered",
  [ng_err_EWOULDBLOCK]  = "Operation would block",
  [ng_err_ENOMSG]       = "No message of desired type",
  [ng_err_EIDRM]        = "Identifier removed",
  [ng_err_ECHRNG]       = "Channel number out of range",
  [ng_err_EL2NSYNC]     = "Level 2 not synchronized",
  [ng_err_EL3HLT]       = "Level 3 halted",
  [ng_err_EL3RST]       = "Level 3 reset",
  [ng_err_ELNRNG]       = "Link number out of range",
  [ng_err_EUNATCH]      = "Protocol driver not attached",
  [ng_err_ENOCSI]       = "No CSI structure available",
  [ng_err_EL2HLT]       = "Level 2 halted",
  [ng_err_EBADE]        = "Invalid exchange",
  [ng_err_EBADR]        = "Invalid request descriptor",
  [ng_err_EXFULL]       = "Exchange full",
  [ng_err_ENOANO]       = "No anode",
  [ng_err_EBADRQC]      = "Invalid request code",
  [ng_err_EBADSLT]      = "Invalid slot",

  [ng_err_EBFONT]       = "Bad font file format",
  [ng_err_ENOSTR]       = "Device not a stream",
  [ng_err_ENODATA]      = "No data available",
  [ng_err_ETIME]        = "Timer expired",
  [ng_err_ENOSR]        = "Out of streams resources",
  [ng_err_ENONET]       = "Machine is not on the network",
  [ng_err_ENOPKG]       = "Package not installed",
  [ng_err_EREMOTE]      = "Object is remote",
  [ng_err_ENOLINK]      = "Link has been severed",
  [ng_err_EADV]         = "Advertise error",
  [ng_err_ESRMNT]       = "Srmount error",
  [ng_err_ECOMM]        = "Communication error on send",
  [ng_err_EPROTO]       = "Protocol error",
  [ng_err_EMULTIHOP]    = "Multihop attempted",
  [ng_err_EDOTDOT]      = "RFS specific error",
  [ng_err_EBADMSG]      = "Not a data message",
  [ng_err_EOVERFLOW]    = "Value too large for defined data type",
  [ng_err_ENOTUNIQ]     = "Name not unique on network",
  [ng_err_EBADFD]       = "File descriptor in bad state",
  [ng_err_EREMCHG]      = "Remote address changed",
  [ng_err_ELIBACC]      = "Can not access a needed shared library",
  [ng_err_ELIBBAD]      = "Accessing a corrupted shared library",
  [ng_err_ELIBSCN]      = ".lib section in a.out corrupted",
  [ng_err_ELIBMAX]      = "Attempting to link in too many shared libraries",
  [ng_err_ELIBEXEC]     = "Cannot exec a shared library directly",
  [ng_err_EILSEQ]       = "Illegal byte sequence",
  [ng_err_ERESTART]     = "Interrupted system call should be restarted",
  [ng_err_ESTRPIPE]     = "Streams pipe error",
  [ng_err_EUSERS]       = "Too many users",
  [ng_err_ENOTSOCK]     = "Socket operation on non-socket",
  [ng_err_EDESTADDRREQ] = "Destination address required",
  [ng_err_EMSGSIZE]     = "Message too long",
  [ng_err_EPROTOTYPE]   = "Protocol wrong type for socket",
  [ng_err_ENOPROTOOPT]  = "Protocol not available",
  [ng_err_EPROTONOSUPPORT] = "Protocol not supported",
  [ng_err_ESOCKTNOSUPPORT] = "Socket type not supported",
  [ng_err_EOPNOTSUPP]      = "Operation not supported on transport endpoint",
  [ng_err_EPFNOSUPPORT]    = "Protocol family not supported",
  [ng_err_EAFNOSUPPORT]    = "Address family not supported by protocol",
  [ng_err_EADDRINUSE]      = "Address already in use",
  [ng_err_EADDRNOTAVAIL]   = "Cannot assign requested address",
  [ng_err_ENETDOWN]        = "Network is down",
  [ng_err_ENETUNREACH]     = "Network is unreachable",
  [ng_err_ENETRESET]       = "Network dropped connection because of reset",
  [ng_err_ECONNABORTED]    = "Software caused connection abort",
  [ng_err_ECONNRESET]      = "Connection reset by peer",
  [ng_err_ENOBUFS]         = "No buffer space available",
  [ng_err_EISCONN]         = "Transport endpoint is already connected",
  [ng_err_ENOTCONN]        = "Transport endpoint is not connected",
  [ng_err_ESHUTDOWN]       = "Cannot send after transport endpoint shutdown",
  [ng_err_ETOOMANYREFS]    = "Too many references: cannot splice",
  [ng_err_ETIMEDOUT]       = "Connection timed out",
  [ng_err_ECONNREFUSED]    = "Connection refused",
  [ng_err_EHOSTDOWN]       = "Host is down",
  [ng_err_EHOSTUNREACH]    = "No route to host",
  [ng_err_EALREADY]        = "Operation already in progress",
  [ng_err_EINPROGRESS]     = "Operation now in progress",
  [ng_err_ESTALE]          = "Stale file handle",
  [ng_err_EUCLEAN]         = "Structure needs cleaning",
  [ng_err_ENOTNAM]         = "Not a XENIX named type file",
  [ng_err_ENAVAIL]         = "No XENIX semaphores available",
  [ng_err_EISNAM]          = "Is a named type file",
  [ng_err_EREMOTEIO]       = "Remote I/O error",
  [ng_err_EDQUOT]          = "Quota exceeded",

  [ng_err_ENOMEDIUM]       = "No medium found",
  [ng_err_EMEDIUMTYPE]     = "Wrong medium type",
  [ng_err_ECANCELED]       = "Operation Canceled",
  [ng_err_ENOKEY]          = "Required key not available",
  [ng_err_EKEYEXPIRED]     = "Key has expired",
  [ng_err_EKEYREVOKED]     = "Key has been revoked",
  [ng_err_EKEYREJECTED]    = "Key was rejected by service",

  /* for robust mutexes. */
  [ng_err_EOWNERDEAD]      = "Owner died",
  [ng_err_ENOTRECOVERABLE] = "State not recoverable",

  [ng_err_ERFKILL]         = "Operation not possible due to RF-kill",
  [ng_err_EHWPOISON]       = "Memory page has hardware error",
  [ng_err_MAX]             = "Unknow error",
};
#endif

const char *ng_n2err(int n)
{
#if configENABLE_ERRNO
  if (n < 0 || n > ng_err_MAX)
    n = ng_err_MAX;
  return __errtable[n];
#else
  return NULL;
#endif
}
