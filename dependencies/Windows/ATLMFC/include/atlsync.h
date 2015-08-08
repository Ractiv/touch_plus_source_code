// This is a part of the Active Template Library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.


#ifndef __ATLSYNC_H__
#define __ATLSYNC_H__

#pragma once

#ifndef _ATL_NO_PRAGMA_WARNINGS
#pragma warning(push)
#pragma warning(disable: 4512)  // assignment operator could not be generated
#endif  // !_ATL_NO_PRAGMA_WARNINGS

#include <atlbase.h>

#pragma pack(push,_ATL_PACKING)

namespace ATL
{

class CCriticalSection :
	public CRITICAL_SECTION
{
public:
	CCriticalSection();
	explicit CCriticalSection(_In_ ULONG nSpinCount);

	~CCriticalSection() throw();

	// Acquire the critical section
	_Acquires_lock_(*this)
	void Enter();
	// Release the critical section
	_Releases_lock_(*this)
	void Leave() throw();

#ifdef _ATL_USE_WINAPI_FAMILY_DESKTOP_APP
	// Set the spin count for the critical section
	ULONG SetSpinCount(_In_ ULONG nSpinCount) throw();
#endif
	// Attempt to acquire the critical section
	_When_(return != 0, _Acquires_lock_(*this))
	BOOL TryEnter() throw();
};

class CEvent :
	public CHandle
{
public:
	CEvent() throw();
	CEvent(_Inout_ CEvent& h) throw();
	CEvent(
		_In_ BOOL bManualReset,
		_In_ BOOL bInitialState);
	CEvent(
		_In_opt_ LPSECURITY_ATTRIBUTES pSecurity,
		_In_ BOOL bManualReset,
		_In_ BOOL bInitialState,
		_In_opt_z_ LPCTSTR pszName);
	explicit CEvent(_In_ HANDLE h) throw();

	// Create a new event
	BOOL Create(
		_In_opt_ LPSECURITY_ATTRIBUTES pSecurity,
		_In_ BOOL bManualReset,
		_In_ BOOL bInitialState,
		_In_opt_z_ LPCTSTR pszName) throw();
	// Open an existing named event
	BOOL Open(
		_In_ DWORD dwAccess,
		_In_ BOOL bInheritHandle,
		_In_z_ LPCTSTR pszName) throw();
#ifdef _ATL_USE_WINAPI_FAMILY_DESKTOP_APP
	// Pulse the event (signals waiting objects, then resets)
	BOOL Pulse() throw();
#endif // _ATL_USE_WINAPI_FAMILY_DESKTOP_APP
	// Set the event to the non-signaled state
	BOOL Reset() throw();
	// Set the event to the signaled state
	BOOL Set() throw();
};

class CMutex :
	public CHandle
{
public:
	CMutex() throw();
	CMutex(_Inout_ CMutex& h) throw();
	explicit CMutex(_In_ BOOL bInitialOwner);
	CMutex(
		_In_opt_ LPSECURITY_ATTRIBUTES pSecurity,
		_In_ BOOL bInitialOwner,
		_In_opt_z_ LPCTSTR pszName);
	explicit CMutex(_In_ HANDLE h) throw();

	// Create a new mutex
	BOOL Create(
		_In_opt_ LPSECURITY_ATTRIBUTES pSecurity,
		_In_ BOOL bInitialOwner,
		_In_opt_z_ LPCTSTR pszName) throw();
	// Open an existing named mutex
	BOOL Open(
		_In_ DWORD dwAccess,
		_In_ BOOL bInheritHandle,
		_In_z_ LPCTSTR pszName) throw();
	// Release ownership of the mutex
	_Releases_lock_(this->m_h)
	BOOL Release() throw();
};

class CSemaphore :
	public CHandle
{
public:
	CSemaphore() throw();
	CSemaphore(_Inout_ CSemaphore& h) throw();
	CSemaphore(
		_In_ LONG nInitialCount,
		_In_ LONG nMaxCount);
	CSemaphore(
		_In_opt_ LPSECURITY_ATTRIBUTES pSecurity,
		_In_ LONG nInitialCount,
		_In_ LONG nMaxCount,
		_In_opt_z_ LPCTSTR pszName );
	explicit CSemaphore(_In_ HANDLE h) throw();

	// Create a new semaphore
	BOOL Create(
		_In_opt_ LPSECURITY_ATTRIBUTES pSecurity,
		_In_ LONG nInitialCount,
		_In_ LONG nMaxCount,
		_In_opt_z_ LPCTSTR pszName) throw();
	// Open an existing named semaphore
	BOOL Open(
		_In_ DWORD dwAccess,
		_In_ BOOL bInheritHandle,
		_In_z_ LPCTSTR pszName) throw();
	// Increase the count of the semaphore
	BOOL Release(
		_In_ LONG nReleaseCount = 1,
		_Out_opt_ LONG* pnOldCount = NULL) throw();
};

class CMutexLock
{
public:
	_Post_same_lock_(mtx, this->m_mtx)
	_When_(bInitialLock != 0, _Acquires_lock_(this->m_mtx) _Post_satisfies_(this->m_bLocked != 0))
	_When_(bInitialLock == 0, _Post_satisfies_(this->m_bLocked == 0))
	CMutexLock(
		_Inout_ CMutex& mtx,
		_In_ bool bInitialLock = true);
	_When_(this->m_bLocked != 0, _Requires_lock_held_(this->m_mtx) _Releases_lock_(this->m_mtx) _Post_satisfies_(this->m_bLocked == 0))
	~CMutexLock() throw();

	_Acquires_lock_(this->m_mtx) _Post_satisfies_(this->m_bLocked != 0)
	void Lock();

	_Releases_lock_(this->m_mtx) _Post_satisfies_(this->m_bLocked == 0)
	void Unlock() throw();

// Implementation
private:
	CMutex& m_mtx;
	bool m_bLocked;

// Private to prevent accidental use
	CMutexLock(_In_ const CMutexLock&) throw();
	CMutexLock& operator=(_In_ const CMutexLock&) throw();
};

};  // namespace ATL


#include <atlsync.inl>

#pragma pack(pop)
#ifndef _ATL_NO_PRAGMA_WARNINGS
#pragma warning(pop)
#endif  // !_ATL_NO_PRAGMA_WARNINGS

#endif  // __ATLSYNC_H__
