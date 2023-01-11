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

#ifndef __ngRTOS_ERRNO_H__
#define __ngRTOS_ERRNO_H__

#ifdef __cplusplus
extern "C" {
#endif

/* FreeRTOS error definitions. */
#define errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY    ( -1 )
#define errQUEUE_BLOCKED                         ( -4 )
#define errQUEUE_YIELD                           ( -5 )

/* Macros used for basic data corruption checks. */
#ifndef configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES
    #define configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES    0
#endif

#if ( configUSE_16_BIT_TICKS == 1 )
    #define pdINTEGRITY_CHECK_VALUE    0x5a5a
#else
    #define pdINTEGRITY_CHECK_VALUE    0x5a5a5a5aUL
#endif

/* The following errno values are used by ngRTOS. */
#define ng_err_NONE              0  /* OK */
#define ng_err_EPERM             1  /* Operation not permitted */
#define ng_err_ENOENT            2  /* No such file or directory */
#define ng_err_ESRCH             3  /* No such process */
#define ng_err_EINTR             4  /* Interrupted system call */
#define ng_err_EIO               5  /* I/O error */
#define ng_err_ENXIO             6  /* No such device or address */
#define ng_err_E2BIG             7  /* Argument list too long */
#define ng_err_ENOEXEC           8  /* Exec format error */
#define ng_err_EBADF             9  /* Bad file number */
#define ng_err_ECHILD           10  /* No child processes */
#define ng_err_EAGAIN           11  /* Try again */
#define ng_err_ENOMEM           12  /* Out of memory */
#define ng_err_EACCES           13  /* Permission denied */
#define ng_err_EFAULT           14  /* Bad address */
#define ng_err_ENOTBLK          15  /* Block device required */
#define ng_err_EBUSY            16  /* Device or resource busy */
#define ng_err_EEXIST           17  /* File exists */
#define ng_err_EXDEV            18  /* Cross-device link */
#define ng_err_ENODEV           19  /* No such device */
#define ng_err_ENOTDIR          20  /* Not a directory */
#define ng_err_EISDIR           21  /* Is a directory */
#define ng_err_EINVAL           22  /* Invalid argument */
#define ng_err_ENFILE           23  /* File table overflow */
#define ng_err_EMFILE           24  /* Too many open files */
#define ng_err_ENOTTY           25  /* Not a typewriter */
#define ng_err_ETXTBSY          26  /* Text file busy */
#define ng_err_EFBIG            27  /* File too large */
#define ng_err_ENOSPC           28  /* No space left on device */
#define ng_err_ESPIPE           29  /* Illegal seek */
#define ng_err_EROFS            30  /* Read-only file system */
#define ng_err_EMLINK           31  /* Too many links */
#define ng_err_EPIPE            32  /* Broken pipe */
#define ng_err_EDOM             33  /* Math argument out of domain of func */
#define ng_err_ERANGE           34  /* Math result not representable */
  
#define ng_err_EDEADLK          35  /* Resource deadlock would occur */
#define ng_err_ENAMETOOLONG     36  /* File name too long */
#define ng_err_ENOLCK           37  /* No record locks available */

/*
 * This error code is special: arch syscall entry code will return
 * -ENOSYS if users try to call a syscall that doesn't exist.  To keep
 * failures of syscalls that really do exist distinguishable from
 * failures due to attempts to use a nonexistent syscall, syscall
 * implementations should refrain from returning -ENOSYS.
 */
#define ng_err_ENOSYS           38  /* Invalid system call number */

#define ng_err_ENOTEMPTY        39  /* Directory not empty */
#define ng_err_ELOOP            40  /* Too many symbolic links encountered */
#define ng_err_EWOULDBLOCK      EAGAIN  /* Operation would block */
#define ng_err_ENOMSG           42  /* No message of desired type */
#define ng_err_EIDRM            43  /* Identifier removed */
#define ng_err_ECHRNG           44  /* Channel number out of range */
#define ng_err_EL2NSYNC         45  /* Level 2 not synchronized */
#define ng_err_EL3HLT           46  /* Level 3 halted */
#define ng_err_EL3RST           47  /* Level 3 reset */
#define ng_err_ELNRNG           48  /* Link number out of range */
#define ng_err_EUNATCH          49  /* Protocol driver not attached */
#define ng_err_ENOCSI           50  /* No CSI structure available */
#define ng_err_EL2HLT           51  /* Level 2 halted */
#define ng_err_EBADE            52  /* Invalid exchange */
#define ng_err_EBADR            53  /* Invalid request descriptor */
#define ng_err_EXFULL           54  /* Exchange full */
#define ng_err_ENOANO           55  /* No anode */
#define ng_err_EBADRQC          56  /* Invalid request code */
#define ng_err_EBADSLT          57  /* Invalid slot */

#define ng_err_EDEADLOCK        EDEADLK

#define ng_err_EBFONT           59  /* Bad font file format */
#define ng_err_ENOSTR           60  /* Device not a stream */
#define ng_err_ENODATA          61  /* No data available */
#define ng_err_ETIME            62  /* Timer expired */
#define ng_err_ENOSR            63  /* Out of streams resources */
#define ng_err_ENONET           64  /* Machine is not on the network */
#define ng_err_ENOPKG           65  /* Package not installed */
#define ng_err_EREMOTE          66  /* Object is remote */
#define ng_err_ENOLINK          67  /* Link has been severed */
#define ng_err_EADV             68  /* Advertise error */
#define ng_err_ESRMNT           69  /* Srmount error */
#define ng_err_ECOMM            70  /* Communication error on send */
#define ng_err_EPROTO           71  /* Protocol error */
#define ng_err_EMULTIHOP        72  /* Multihop attempted */
#define ng_err_EDOTDOT          73  /* RFS specific error */
#define ng_err_EBADMSG          74  /* Not a data message */
#define ng_err_EOVERFLOW        75  /* Value too large for defined data type */
#define ng_err_ENOTUNIQ         76  /* Name not unique on network */
#define ng_err_EBADFD           77  /* File descriptor in bad state */
#define ng_err_EREMCHG          78  /* Remote address changed */
#define ng_err_ELIBACC          79  /* Can not access a needed shared library */
#define ng_err_ELIBBAD          80  /* Accessing a corrupted shared library */
#define ng_err_ELIBSCN          81  /* .lib section in a.out corrupted */
#define ng_err_ELIBMAX          82  /* Attempting to link in too many shared libraries */
#define ng_err_ELIBEXEC         83  /* Cannot exec a shared library directly */
#define ng_err_EILSEQ           84  /* Illegal byte sequence */
#define ng_err_ERESTART         85  /* Interrupted system call should be restarted */
#define ng_err_ESTRPIPE         86  /* Streams pipe error */
#define ng_err_EUSERS           87  /* Too many users */
#define ng_err_ENOTSOCK         88  /* Socket operation on non-socket */
#define ng_err_EDESTADDRREQ     89  /* Destination address required */
#define ng_err_EMSGSIZE         90  /* Message too long */
#define ng_err_EPROTOTYPE       91  /* Protocol wrong type for socket */
#define ng_err_ENOPROTOOPT      92  /* Protocol not available */
#define ng_err_EPROTONOSUPPORT  93  /* Protocol not supported */
#define ng_err_ESOCKTNOSUPPORT  94  /* Socket type not supported */
#define ng_err_EOPNOTSUPP       95  /* Operation not supported on transport endpoint */
#define ng_err_EPFNOSUPPORT     96  /* Protocol family not supported */
#define ng_err_EAFNOSUPPORT     97  /* Address family not supported by protocol */
#define ng_err_EADDRINUSE       98  /* Address already in use */
#define ng_err_EADDRNOTAVAIL    99  /* Cannot assign requested address */
#define ng_err_ENETDOWN         100  /* Network is down */
#define ng_err_ENETUNREACH      101  /* Network is unreachable */
#define ng_err_ENETRESET        102  /* Network dropped connection because of reset */
#define ng_err_ECONNABORTED     103  /* Software caused connection abort */
#define ng_err_ECONNRESET       104  /* Connection reset by peer */
#define ng_err_ENOBUFS          105  /* No buffer space available */
#define ng_err_EISCONN          106  /* Transport endpoint is already connected */
#define ng_err_ENOTCONN         107  /* Transport endpoint is not connected */
#define ng_err_ESHUTDOWN        108  /* Cannot send after transport endpoint shutdown */
#define ng_err_ETOOMANYREFS     109  /* Too many references: cannot splice */
#define ng_err_ETIMEDOUT        110  /* Connection timed out */
#define ng_err_ECONNREFUSED     111  /* Connection refused */
#define ng_err_EHOSTDOWN        112  /* Host is down */
#define ng_err_EHOSTUNREACH     113  /* No route to host */
#define ng_err_EALREADY         114  /* Operation already in progress */
#define ng_err_EINPROGRESS      115  /* Operation now in progress */
#define ng_err_ESTALE           116  /* Stale file handle */
#define ng_err_EUCLEAN          117  /* Structure needs cleaning */
#define ng_err_ENOTNAM          118  /* Not a XENIX named type file */
#define ng_err_ENAVAIL          119  /* No XENIX semaphores available */
#define ng_err_EISNAM           120  /* Is a named type file */
#define ng_err_EREMOTEIO        121  /* Remote I/O error */
#define ng_err_EDQUOT           122  /* Quota exceeded */

#define ng_err_ENOMEDIUM        123  /* No medium found */
#define ng_err_EMEDIUMTYPE      124  /* Wrong medium type */
#define ng_err_ECANCELED        125  /* Operation Canceled */
#define ng_err_ENOKEY           126  /* Required key not available */
#define ng_err_EKEYEXPIRED      127  /* Key has expired */
#define ng_err_EKEYREVOKED      128  /* Key has been revoked */
#define ng_err_EKEYREJECTED     129  /* Key was rejected by service */

/* for robust mutexes */
#define ng_err_EOWNERDEAD       130  /* Owner died */
#define ng_err_ENOTRECOVERABLE  131  /* State not recoverable */

#define ng_err_ERFKILL          132  /* Operation not possible due to RF-kill */
#define ng_err_EHWPOISON        133  /* Memory page has hardware error */
#define ng_err_MAX              134  

const char *ng_n2err(int n);

#define IS_ERR_VALUE(x) ((unsigned long)(x) >= (unsigned long)ng_err_MAX)

static inline long IS_ERR(const void *ptr)
{
	return IS_ERR_VALUE((unsigned long)ptr);
}

#ifdef __cplusplus
}
#endif

#endif
