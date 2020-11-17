#pragma once
#include <QString>
#include <deque>

namespace tool
{
	////////////////////////////////////////////////////////////////////////////////
	/** directories symlinker and mount point determiner */
	class symlink
	{
	public:
		symlink(const QString& mp);

		QString mountPoint();
		void link(const QString&);
		void unlink();

	protected:
		const QString m_mountPoint;
	};

	////////////////////////////////////////////////////////////////////////////////
} /* namespace tool */
