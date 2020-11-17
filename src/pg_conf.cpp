#include "pg_conf.h"

#include <QFileInfo>
#include <QRegularExpression>
#include <QDebug>
#include <QTimeZone>

namespace pg
{
	////////////////////////////////////////////////////////////////////////////////

	QStringList findLooksLike(const QStringList& keys, const QString& v)
	{
		if (v.isEmpty() || keys.isEmpty())
			return {};
		QString pattern{ v };
		while (!pattern.isEmpty())
		{
			QStringList matches = keys.filter(QRegularExpression{ QString{ "%1" }.arg(pattern) });
			if (!matches.isEmpty())
				return matches;
			pattern.chop(1);
		}
		return {};
	}

	////////////////////////////////////////////////////////////////////////////////

	Parameter::Parameter(const Position& pos, const QString& n, bool c)
		: position(pos)
		, value(n)
		, commented(c)
	{}

	Parameter::Parameter(const QString& n, bool c)
		: Parameter{ Position{ 0, 0 }, n, c }
	{}

	bool Parameter::operator<(const Parameter& p) const
	{
		return position.line < p.position.line;
	}

	QString Parameter::dequoteValue() const
	{
		if (value.startsWith('"') && value.endsWith('"'))
			return value.mid(1, value.size() - 2);
		if (value.startsWith('\'') && value.endsWith('\''))
			return value.mid(1, value.size() - 2);
		return value;
	}

	////////////////////////////////////////////////////////////////////////////////

	Patch::Patch(const QString& f)
		: m_filename(f)
	{}

	QString Patch::timestamp() const
	{
		QString result;
		QTextStream stream{ &result };
		auto dt = QFileInfo{ m_filename }.lastModified();
		auto systemUtcOffset = QTimeZone::systemTimeZone().offsetFromUtc(QDateTime::currentDateTime());
		auto tztime = QTime(0, 0, 0).addSecs(std::abs(systemUtcOffset));

		stream
			<< dt.date().toString("yyyy-MM-dd") << " "
			<< dt.time().toString("hh:mm:ss.zzz") << " "
			<< (systemUtcOffset < 0 ? "-" : "+")
			<< tztime.toString("HHmm");

		return result;
	}

	void Patch::commit(QTextStream& stream, const QString& label, bool write)
	{
		if (m_changes.empty())
			return;
		QFile
			file{ m_filename };
		QStringList content;
		QString filename{ QFileInfo{ m_filename }.fileName() };
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) // | QIODevice::Append))
			throw std::runtime_error{ "cannot open file" };
		stream
			<< "--- "
			<< left << qSetFieldWidth(filename.size() < 28 ? 21 : filename.size() + 3)
			<< (label.isEmpty() ? filename : label) << reset << timestamp() << endl
			<< "+++ "
			<< left << qSetFieldWidth(filename.size() < 28 ? 21 : filename.size() + 3)
			<< filename << reset << timestamp() << endl;
		file.seek(0);
		long long lineNo = 0;
		while (!file.atEnd())
		{
			++lineNo;
			const QString line = file.readLine();
			auto change = m_changes.find(lineNo);
			if (change != m_changes.end())
			{
				if (line != change->second.first)
					throw std::runtime_error{ "cannot patch file" };
				content.push_back(change->second.second);
			}
			else
				content.push_back(line);
		}
		file.close();
		Data::iterator it = m_changes.begin();
		while (it != m_changes.end())
			stream << chunk(it);
		if (write)
		{
			if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
				throw std::runtime_error{ "cannot open file for writing" };
			for (const auto& line : content)
				file.write(line.toUtf8());
			file.close();
		}
	}

	QString Patch::chunk(Data::iterator& it)
	{
		QString result, add;
		QTextStream stream{ &result }, sa{ &add };
		int headerLines = 0;
		auto idx = it->first;
		while (true)
		{
			++headerLines;
			auto f = m_changes.find(idx + headerLines);
			if (f == m_changes.end())
				break;
		}

		stream
			<< "@@ "
			<< "-" << it->first << (headerLines == 1 ? "" : "," + QString::number(headerLines))
			<< " "
			<< "+" << it->first << (headerLines == 1 ? "" : "," + QString::number(headerLines))
			<< " @@" << endl;
		idx = it->first;
		for (int i = 0; i != headerLines; ++i)
		{
			it = m_changes.find(idx + i);
			stream << "-" << it->second.first;
			sa << "+" << it->second.second;
		}
		stream << add;
		it = std::next(it);

		return result;
	}

	////////////////////////////////////////////////////////////////////////////////

	Configuration::Configuration(const QString& fname)
		: Patch(fname)
	{
		if (!QFileInfo{ fname }.exists())
			throw std::runtime_error{ "configuration file not found" };
		load();
	}

	Parameter Configuration::get(const QString& param) const
	{
		if (!m_config.contains(param))
		{
			QString errorMessage = QString{ "parameter %1 not found" }.arg(param);
			auto alt = findLooksLike(m_config.keys(), param);
			if (!alt.isEmpty())
				errorMessage = QString{ "%1. Alternatives: [%2]" }.arg(errorMessage).arg(alt.join(", "));
			throw std::runtime_error{ errorMessage.toUtf8() };
		}
		return m_config[param];
	}

	QStringList Configuration::parameters() const
	{
		return m_config.keys();
	}

	void Configuration::set(const QString& param, const Parameter& replace)
	{
		static const std::vector<std::pair<QString, QString>> re_screener = {
			{ "-", "\\-" },
			{ "+", "\\+" },
			{ "*", "\\*" },
			{ ".", "\\." },
			{ "(", "\\(" },
			{ ")", "\\)" },
			{ "$", "\\$" },
			{ "^", "\\^" },
		};
		auto original = get(param);
		QFile
			file{ m_filename };
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
			throw std::runtime_error{ "cannot open file" };
		if (m_config[param].value == replace.value && m_config[param].commented == replace.commented)
			return;

		file.seek(original.position.seek);
		QString line = file.readLine(), repl, replValue{ original.value };

		for (const auto& p : re_screener)
			replValue = replValue.replace(p.first, p.second);

		const QRegularExpression lineExpr{
			QString{ "^\\s*(#?)\\s*(%1).*"
					 "(?:(?:\\s*(=)\\s*)|(?:\\s+))"
					 "\\s*(%2)([^\n]*)$" }
				.arg(param)
				.arg(replValue)
		};
		auto m = lineExpr.match(line);
		if (!m.hasMatch())
			throw std::runtime_error{ "cannot change value" };
		QTextStream stream{ &repl };
		QStringRef
			comment = m.capturedRef(1),
			value = m.capturedRef(4),
			rest = m.capturedRef(5);
		stream
			<< line.mid(0, comment.position())
			<< (replace.commented ? "#" : "")
			<< line.mid(comment.position() + comment.size(), value.position() - comment.position() - comment.size())
			<< replace.value
			<< m.capturedRef(5) << endl;

		file.close();
		m_config[param] = replace;
		m_changes[original.position.line] = { line, repl };
	}

	void Configuration::load()
	{
		QFile file{ m_filename };
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
			throw std::runtime_error{ "cannot open file" };
		static const QRegularExpression lineExpr{
			"^\\s{0,2}(#?)\\s*([\\w\\d_]+)" // line begin. capture comment, if exists
			"(?:(?:\\s*(=)\\s*)|(?:\\s+))"  // no capture. Conf's separator can be '=' or space character
			"("                             // begin values matching
			"(?:[+-]?[\\d\\.]+[\\w]*)"      // integers and units
			"|(?:'(?:[^']*'))"              // string
			"|(?:on|off|true|false|yes|no)" // boolean
			"|(?:[\\w_]+)"                  // enums
			")"                             // end values matching
			"(.*)$"                         // rest of line
		},
			restExpr{ "^((?:\\s+$)|(?:\\s*#.+$)|(?:$))" };

		// QTextStream in(&file);
		unsigned lineNo = 0;
		while (!file.atEnd())
		{
			++lineNo;
			auto pos = file.pos();
			auto line = file.readLine();
			if (line.isEmpty())
				continue;
			auto match = lineExpr.match(line);
			if (match.hasMatch())
			{
				bool comment = (match.captured(1) == "#");
				if (comment)
				{
					if (match.captured(3) != "=")
						continue;
					if (!restExpr.match(match.captured(5)).hasMatch())
						continue;
				}
				m_config[match.captured(2)] = Parameter{ { lineNo, pos }, match.captured(4), comment };
			}
		}
		file.close();
		// dirty hack. some values from standart header gets through re
		m_config.remove("d");
		m_config.remove("value");
	}

	////////////////////////////////////////////////////////////////////////////////
} /* namespace pg */
