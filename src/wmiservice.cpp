#include "wmiservice.h"
#include <QRegularExpression>

////////////////////////////////////////////////////////////////////////////////

WmiService::WmiService(const tool::WmiObject& o)
	: tool::WmiObject(o)
{
}

const QString WmiService::executableKeyData(const QString& key)
{
	const auto extractKeyData = [&key](const QStringList& args) {
		QStringList result;
		bool skip = false, push = false;
		for (const auto& arg : args)
		{
			if (arg.startsWith('"'))
				skip = true;
			if (arg.endsWith('"'))
				skip = false;
			if (push)
			{
				result.push_back(arg);
				if (!skip)
					break;
			}
			if (skip)
				continue;
			if (arg == key)
				push = true;
		}
		const auto path = result.join(' ');
		if (path.startsWith('"') && path.endsWith('"'))
			return path.mid(1, path.size() - 2);
		return result.join(" ");
	};
	return extractKeyData(fullPath().split(' '));
}

const QString WmiService::fullPath() const
{
	const QString p = property("PathName").toString().replace('\\', '/');
	if (p.startsWith('"') && p.endsWith('"'))
		return p.mid(1, p.size() - 2);
	return p;
}

const QString WmiService::path() const
{
	static const QRegularExpression startWithQuotes{ "^\"([^\"]+).*$" },
		plainRun{ "^(.+\\.(exe|dll|sys)).*$" };
	const QString p = property("PathName").toString();
	return (p.startsWith('\"')
			? startWithQuotes.match(p).captured(1)
			: plainRun.match(p).captured(1))
		.replace('\\', '/');
}

const QString WmiService::executable() const
{
	static const QRegularExpression exec{ "^.+/(.+\\.(exe|dll|sys))" };
	const QString path{ this->path() };
	return exec.match(path).captured(1);
}

const QString WmiService::executableDirectory() const
{
	static const QRegularExpression exec{ "^(.+/)(.+\\.(exe|dll|sys))" };
	const QString path{ this->path() };
	return exec.match(path).captured(1);
}

const QString WmiService::name() const
{
	return property("Name").toString();
}

WmiService::State WmiService::state()
{
	static const std::map<QString, State> serviceStatuses{
		{ "Stopped", Stopped },
		{ "Start Pending", StartPending },
		{ "Stop Pending", StopPending },
		{ "Running", Running },
		{ "Continue Pending", ContinuePending },
		{ "Pause Pending", PausePending },
		{ "Paused", Paused },
		{ "Unknown", Unknown },
	};
	updateObject();

	const auto stateString = this->property("State").toString();
	const auto s = serviceStatuses.find(stateString);
	if (s == serviceStatuses.end())
		throw std::runtime_error("no statuse found");
	return s->second;
}

WmiService::StartMode WmiService::startMode() const
{
	static const std::map<QString, StartMode> stateStatuses{
		{ "Boot", Boot },
		{ "System", System },
		{ "Auto", Auto },
		{ "Manual", Manual },
		{ "Disabled", Disabled },
	};
	const auto stateString = this->property("StartMode").toString();
	const auto s = stateStatuses.find(stateString);
	if (s == stateStatuses.end())
		throw std::runtime_error("no start mode found");
	return s->second;
}

int WmiService::start()
{
	return method("StartService()");
}

int WmiService::stop()
{
	return method("StopService()");
}

int WmiService::resume()
{
	return method("ResumeService()");
}

