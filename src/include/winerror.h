#ifndef _WINERROR_
#define _WINERROR_

extern DWORD lastWIN32Error;

#define GetLastError() lastWIN32Error
#define SetLastError(dwErr) lastWIN32Error=dwErr

//
// MessageId: ERROR_SUCCESS
//
// MessageText:
//
//  The operation completed successfully.
//
#define ERROR_SUCCESS                    0L

//
// MessageId: ERROR_OUTOFMEMORY
//
// MessageText:
//
//  Not enough storage is available to complete this operation.
//
#define ERROR_OUTOFMEMORY                14L

//
// MessageId: ERROR_INVALID_PARAMETER
//
// MessageText:
//
//  The parameter is incorrect.
//
#define ERROR_INVALID_PARAMETER          87L    // dderror


// MessageId: ERROR_IO_PENDING
//
// MessageText:
//
//  Overlapped I/O operation is in progress.
//
#define ERROR_IO_PENDING                 997L    // dderror

//
// MessageId: ERROR_INSUFFICIENT_BUFFER
//
// MessageText:
//
//  The data area passed to a system call is too
//  small.
//
#define ERROR_INSUFFICIENT_BUFFER        122L    // dderror

//
// MessageId: ERROR_IO_PENDING
//
// MessageText:
//
//  Overlapped I/O operation is in progress.
//
#define ERROR_IO_PENDING                 997L    // dderror


#endif
