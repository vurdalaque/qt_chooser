#include "tooling.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winioctl.h> // DeviceIoControl

#include <QFileInfo>

////////////////////////////////////////////////////////////////////////////////
/* #include <ntifs.h> */

typedef struct _REPARSE_DATA_BUFFER
{
	ULONG ReparseTag;
	USHORT ReparseDataLength;
	USHORT Reserved;
	union
	{
		struct
		{
			USHORT SubstituteNameOffset;
			USHORT SubstituteNameLength;
			USHORT PrintNameOffset;
			USHORT PrintNameLength;
			ULONG Flags;
			WCHAR PathBuffer[1];
		} SymbolicLinkReparseBuffer;
		struct
		{
			USHORT SubstituteNameOffset;
			USHORT SubstituteNameLength;
			USHORT PrintNameOffset;
			USHORT PrintNameLength;
			WCHAR PathBuffer[1];
		} MountPointReparseBuffer;
		struct
		{
			UCHAR DataBuffer[1];
		} GenericReparseBuffer;
	} DUMMYUNIONNAME;
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;

////////////////////////////////////////////////////////////////////////////////
namespace tool
{
	////////////////////////////////////////////////////////////////////////////////

	symlink::symlink(const QString& mp)
		: m_mountPoint(mp)
	{}

	QString symlink::mountPoint()
	{
		if (m_mountPoint.isEmpty())
			throw std::runtime_error("empty mount point");
		HANDLE handle = CreateFileW(
			reinterpret_cast<const wchar_t*>(m_mountPoint.utf16()), 0,
			FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
			FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, nullptr);
		if (handle == 0)
			throw std::runtime_error("CreateFile");

		static const size_t buffer_size = 16 * 1024;
		char* buffer = new char[buffer_size];
		DWORD bytesread = 0;

		if (!DeviceIoControl(handle, FSCTL_GET_REPARSE_POINT, nullptr, 0, buffer,
				buffer_size, &bytesread, nullptr))
		{
			delete[] buffer, buffer = nullptr;
			throw std::runtime_error("DeviceIoControl");
		}

		QString result;
		REPARSE_DATA_BUFFER* data = reinterpret_cast<REPARSE_DATA_BUFFER*>(buffer);

		if (data->ReparseTag == IO_REPARSE_TAG_SYMLINK)
		{
			auto& sym = data->SymbolicLinkReparseBuffer;
			result = QString::fromUtf16(reinterpret_cast<const ushort*>(
											&data->SymbolicLinkReparseBuffer
												 .PathBuffer[sym.PrintNameOffset / 2]),
				sym.PrintNameLength / 2);
		}
		if (data->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT)
		{
			auto& m = data->MountPointReparseBuffer;
			result = QString::fromUtf16(reinterpret_cast<const ushort*>(
											&data->MountPointReparseBuffer
												 .PathBuffer[m.PrintNameOffset / 2]),
				m.PrintNameLength / 2);
		}
		if (result.isEmpty())
		{
			delete[] buffer, buffer = nullptr;
			throw std::runtime_error("invalid mounting point");
		}
		if (result.startsWith("\\??\\"))
			// dont remember why, but py prototype do this
			result = result.mid(4);
		::CloseHandle(handle);
		delete[] buffer, buffer = nullptr;
		return result;
	}

	void symlink::link(const QString& target)
	{
		::CreateSymbolicLinkW(
			reinterpret_cast<const wchar_t*>(m_mountPoint.utf16()),
			reinterpret_cast<const wchar_t*>(target.utf16()),
			SYMBOLIC_LINK_FLAG_DIRECTORY);
	}

	void symlink::unlink()
	{
		if (QFileInfo(m_mountPoint).exists())
			::RemoveDirectoryW(
				reinterpret_cast<const wchar_t*>(m_mountPoint.utf16()));
	}

	////////////////////////////////////////////////////////////////////////////////
} /* namespace tool */
