// This is a part of the Active Template Library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLSYNC_INL__
#define __ATLSYNC_INL__

#pragma once

#ifndef __ATLSYNC_H__
	#error atlsync.inl requires atlsync.h to be included first
#endif

namespace ATL
{

inline CCriticalSection::CCriticalSection()
{
#ifdef _ATL_USE_WINAPI_FAMILY_DESKTOP_APP
	if (!::InitializeCriticalSectionAndSpinCount( this, 0 ))
#else
	if (!::_AtlInitializeCriticalSectionEx( this, 0, 0 ))
#endif
	{
		AtlThrow(HRESULT_FROM_WIN32(GetLastError()));
	}
}

inline CCriticalSection::CCriticalSection(_In_ ULONG nSpinCount)
{
#if !defined(_ATL_USE_WINAPI_FAMILY_DESKTOP_APP) || defined(_ATL_STATIC_LIB_IMPL)
	if (!::_AtlInitializeCriticalSectionEx( this, nSpinCount, 0 ))
#else
	if (!::InitializeCriticalSectionAndSpinCount( this, nSpinCount ))
#endif
	{
		AtlThrow(HRESULT_FROM_WIN32(GetLastError()));
	}
}

inline CCriticalSection::~CCriticalSection() throw()
{
	::DeleteCriticalSection( this );
}

_Acquires_lock_(*this)
inline void CCriticalSection::Enter()
{
	::EnterCriticalSection( this );
}

_Releases_lock_(*this)
inline void CCriticalSection::Leave() throw()
{
	::LeaveCriticalSection( this );
}

#ifdef _ATL_USE_WINAPI_FAMILY_DESKTOP_APP

inline ULONG CCriticalSection::SetSpinCount(_In_ ULONG nSpinCount) throw()
{
	return( ::SetCriticalSectionSpinCount( this, nSpinCount ) );
}

#endif

_When_(return != 0, _Acquires_lock_(*this))
inline BOOL CCriticalSection::TryEnter() throw()
{
	return( ::TryEnterCriticalSection( this ) );
}

inline CEvent::CEvent() throw()
{
}

inline CEvent::CEvent(_Inout_ CEvent& hEvent) throw() :
	CHandle( hEvent )
{
}

inline CEvent::CEvent(
	_In_ BOOL bManualReset,
	_In_ BOOL bInitialState)
{
	BOOL bSuccess;

	bSuccess = Create( NULL, bManualReset, bInitialState, NULL );
	if( !bSuccess )
	{
		AtlThrowLastWin32();
	}
}

inline CEvent::CEvent(
	_In_opt_ LPSECURITY_ATTRIBUTES pAttributes,
	_In_ BOOL bManualReset,
	_In_ BOOL bInitialState,
	_In_opt_z_ LPCTSTR pszName)
{
	BOOL bSuccess;

	bSuccess = Create( pAttributes, bManualReset, bInitialState, pszName );
	if( !bSuccess )
	{
		AtlThrowLastWin32();
	}
}


inline CEvent::CEvent(_In_ HANDLE h) throw() :
	CHandle( h )
{
}

inline BOOL CEvent::Create(
	_In_opt_ LPSECURITY_ATTRIBUTES pSecurity,
	_In_ BOOL bManualReset,
	_In_ BOOL bInitialState,
	_In_opt_z_ LPCTSTR pszName) throw()
{
	ATLASSUME( m_h == NULL );

#ifdef _ATL_USE_WINAPI_FAMILY_DESKTOP_APP
	m_h = ::CreateEvent( pSecurity, bManualReset, bInitialState, pszName );
#else
	DWORD dwFlags = 0;

	if (bManualReset)
		dwFlags |= CREATE_EVENT_MANUAL_RESET;

	if (bInitialState)
		dwFlags |= CREATE_EVENT_INITIAL_SET;

	m_h = ::CreateEventEx( pSecurity, pszName, dwFlags, EVENT_ALL_ACCESS );
#endif

	return( m_h != NULL );
}

inline BOOL CEvent::Open(
	_In_ DWORD dwAccess,
	_In_ BOOL bInheritHandle,
	_In_z_ LPCTSTR pszName) throw()
{
	ATLASSUME( m_h == NULL );

	m_h = ::OpenEvent( dwAccess, bInheritHandle, pszName );
	return( m_h != NULL );
}

#ifdef _ATL_USE_WINAPI_FAMILY_DESKTOP_APP
inline BOOL CEvent::Pulse() throw()
{
	ATLASSUME( m_h != NULL );

	return( ::PulseEvent( m_h ) );
}
#endif // _ATL_USE_WINAPI_FAMILY_DESKTOP_APP

inline BOOL CEvent::Reset() throw()
{
	ATLASSUME( m_h != NULL );

	return( ::ResetEvent( m_h ) );
}

inline BOOL CEvent::Set() throw()
{
	ATLASSUME( m_h != NULL );

	return( ::SetEvent( m_h ) );
}


inline CMutex::CMutex() throw()
{
}

inline CMutex::CMutex(_Inout_ CMutex& hMutex) throw() :
	CHandle( hMutex )
{
}

inline CMutex::CMutex(_In_ BOOL bInitialOwner)
{
	BOOL bSuccess;

	bSuccess = Create( NULL, bInitialOwner, NULL );
	if( !bSuccess )
	{
		AtlThrowLastWin32();
	}
}

inline CMutex::CMutex(
	_In_opt_ LPSECURITY_ATTRIBUTES pSecurity,
	_In_ BOOL bInitialOwner,
	_In_opt_z_ LPCTSTR pszName)
{
	BOOL bSuccess;

	bSuccess = Create( pSecurity, bInitialOwner, pszName );
	if( !bSuccess )
	{
		AtlThrowLastWin32();
	}
}

inline CMutex::CMutex(_In_ HANDLE h) throw() :
	CHandle( h )
{
}

inline BOOL CMutex::Create(
	_In_opt_ LPSECURITY_ATTRIBUTES pSecurity,
	_In_ BOOL bInitialOwner,
	_In_opt_z_ LPCTSTR pszName) throw()
{
	ATLASSUME( m_h == NULL );
#ifdef _ATL_USE_WINAPI_FAMILY_DESKTOP_APP
	m_h = ::CreateMutex( pSecurity, bInitialOwner, pszName );
#else
	m_h = ::CreateMutexEx( pSecurity, pszName, bInitialOwner? CREATE_MUTEX_INITIAL_OWNER : 0, MUTEX_ALL_ACCESS );
#endif
	return( m_h != NULL );
}

inline BOOL CMutex::Open(
	_In_ DWORD dwAccess,
	_In_ BOOL bInheritHandle,
	_In_z_ LPCTSTR pszName) throw()
{
	ATLASSUME( m_h == NULL );

	m_h = ::OpenMutex( dwAccess, bInheritHandle, pszName );
	return( m_h != NULL );
}

_Releases_lock_(this->m_h)
inline BOOL CMutex::Release() throw()
{
	ATLASSUME( m_h != NULL );

	return( ::ReleaseMutex( m_h ) );
}

inline CSemaphore::CSemaphore() throw()
{
}

inline CSemaphore::CSemaphore(_Inout_ CSemaphore& hSemaphore) throw() :
	CHandle( hSemaphore )
{
}

inline CSemaphore::CSemaphore(
	_In_ LONG nInitialCount,
	_In_ LONG nMaxCount)
{
	BOOL bSuccess = Create( NULL, nInitialCount, nMaxCount, NULL );
	if( !bSuccess )
	{
		AtlThrowLastWin32();
	}
}

inline CSemaphore::CSemaphore(
	_In_opt_ LPSECURITY_ATTRIBUTES pSecurity,
	_In_ LONG nInitialCount,
	_In_ LONG nMaxCount,
	_In_opt_z_ LPCTSTR pszName)
{
	BOOL bSuccess;

	bSuccess = Create( pSecurity, nInitialCount, nMaxCount, pszName );
	if( !bSuccess )
	{
		AtlThrowLastWin32();
	}
}

inline CSemaphore::CSemaphore(_In_ HANDLE h) throw() :
	CHandle( h )
{
}

inline BOOL CSemaphore::Create(
	_In_opt_ LPSECURITY_ATTRIBUTES pSecurity,
	_In_ LONG nInitialCount,
	_In_ LONG nMaxCount,
	_In_opt_z_ LPCTSTR pszName) throw()
{
	ATLASSUME( m_h == NULL );

#ifdef _ATL_USE_WINAPI_FAMILY_DESKTOP_APP
	m_h = ::CreateSemaphore( pSecurity, nInitialCount, nMaxCount, pszName);
#else
	m_h = ::CreateSemaphoreEx( pSecurity, nInitialCount, nMaxCount, pszName, 0, SEMAPHORE_ALL_ACCESS);
#endif
	return( m_h != NULL );
}

inline BOOL CSemaphore::Open(
	_In_ DWORD dwAccess,
	_In_ BOOL bInheritHandle,
	_In_z_ LPCTSTR pszName) throw()
{
	ATLASSUME( m_h == NULL );

	m_h = ::OpenSemaphore( dwAccess, bInheritHandle, pszName );
	return( m_h != NULL );
}

inline BOOL CSemaphore::Release(
	_In_ LONG nReleaseCount,
	_Out_opt_ LONG* pnOldCount) throw()
{
	ATLASSUME( m_h != NULL );

	return( ::ReleaseSemaphore( m_h, nReleaseCount, pnOldCount ) );
}


_Post_same_lock_(mtx, this->m_mtx)
_When_(bInitialLock != 0, _Acquires_lock_(this->m_mtx) _Post_satisfies_(this->m_bLocked != 0))
_When_(bInitialLock == 0, _Post_satisfies_(this->m_bLocked == 0))
inline CMutexLock::CMutexLock(
		_Inout_ CMutex& mtx,
		_In_ bool bInitialLock) :
	m_mtx( mtx ),
	m_bLocked( false )
{
	if( bInitialLock )
	{
		Lock();
	}
}

_When_(this->m_bLocked != 0, _Requires_lock_held_(this->m_mtx) _Releases_lock_(this->m_mtx) _Post_satisfies_(this->m_bLocked == 0))
inline CMutexLock::~CMutexLock() throw()
{
	if( m_bLocked )
	{
		Unlock();
	}
}

_Acquires_lock_(this->m_mtx) _Post_satisfies_(this->m_bLocked != 0)
inline void CMutexLock::Lock()
{
	DWORD dwResult;

	ATLASSERT( !m_bLocked );
	dwResult = ::WaitForSingleObjectEx( m_mtx, INFINITE, FALSE );
	if( dwResult == WAIT_ABANDONED )
	{
		ATLTRACE(atlTraceSync, 0, _T("Warning: abandoned mutex 0x%x\n"), 
			reinterpret_cast<int>(static_cast<HANDLE>(m_mtx)));
	}
	_Analysis_assume_lock_held_(this->m_mtx);
	m_bLocked = true;
}

_Releases_lock_(this->m_mtx) _Post_satisfies_(this->m_bLocked == 0)
inline void CMutexLock::Unlock() throw()
{
	ATLASSUME( m_bLocked );

	_Analysis_assume_lock_held_((this->m_mtx).m_h);
	m_mtx.Release();
	//ATLASSERT in CMutexLock::Lock prevents calling Lock more than 1 time.
	_Analysis_assume_lock_released_(this->m_mtx);
	m_bLocked = false;
}

};  // namespace ATL

#endif  // __ATLSYNC_INL__
