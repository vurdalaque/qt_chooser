#pragma once
#include <set>
#include <QString>
#include <QFile>
#include <QVariant>
#include <QTextStream>

namespace pg
{
	////////////////////////////////////////////////////////////////////////////////

	struct Parameter
	{
		struct Position
		{
			unsigned line;
			long long seek;
		} position;
		QString value;
		bool commented;
		Parameter(const QString& = QString{}, bool = false);
		Parameter(const Position&, const QString&, bool = false);
		bool operator<(const Parameter&) const;
		QString dequoteValue() const;
	};

	////////////////////////////////////////////////////////////////////////////////

	class Patch
	{
	public:
		Patch(const QString&);
		using Line = std::pair<QString, QString>;
		using Data = std::map<unsigned, Line>;

		void commit(QTextStream& stream, const QString& label, bool write = true);

	protected:
		QString timestamp() const;
		QString chunk(Data::iterator&);

	protected:
		QString m_filename;
		Patch::Data m_changes;
	};

	////////////////////////////////////////////////////////////////////////////////

	class Configuration : public Patch
	{
	public:
		Configuration(const QString&);

		Parameter get(const QString&) const;
		void set(const QString&, const Parameter&);
		QStringList parameters() const;

	protected:
		void load();

	protected:
		QHash<QString, Parameter> m_config;
	};

	////////////////////////////////////////////////////////////////////////////////
} /* namespace pg */

