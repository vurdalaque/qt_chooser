#pragma once
#include <QObject>
#include <QVariant>

#include <deque>
#include <set>

#include <QDebug>

struct IWbemLocator;
struct IWbemServices;
struct IWbemClassObject;

using Locator = std::shared_ptr<IWbemLocator>;
using Services = std::shared_ptr<IWbemServices>;
using ClassObject = std::shared_ptr<IWbemClassObject>;

namespace tool
{
	////////////////////////////////////////////////////////////////////////////////

	struct CoInitialize
	{
		enum SingletonPolicy
		{
			GET,
			RESET
		};
		CoInitialize();
		~CoInitialize();

		static IWbemLocator* locator(SingletonPolicy = GET);
		static IWbemServices* services(SingletonPolicy = GET);
	};

	////////////////////////////////////////////////////////////////////////////////

	class WmiObject : public QObject
	{
	public:
		void updateObject();

		virtual const QMetaObject* metaObject() const override;
		virtual void* qt_metacast(const char*) override;
		virtual int qt_metacall(QMetaObject::Call, int, void**) override;
		static void qt_static_metacall(QObject*, QMetaObject::Call, int, void**);

	public:
		using List = std::deque<WmiObject>;
		using PropertyList = std::map<QString, QVariant>;
		using MethodParameters = std::deque<std::pair<QString, QVariant>>;
		using MethodDefinition = std::pair<QString, std::pair<MethodParameters, MethodParameters>>;
		using MethodList = std::set<MethodDefinition>;

		WmiObject(const WmiObject&);
		~WmiObject();
		WmiObject& operator=(const WmiObject&);
		WmiObject& operator=(WmiObject&&);

		static const List objects(const QString&);

	protected:
		WmiObject(IWbemClassObject*);
		static QString createSignature(const MethodDefinition&);
		static QList<QByteArray> getParameterNames(const MethodDefinition&);

		static const std::deque<std::pair<QString, QVariant>> qualifiers(IWbemClassObject*);
		static const PropertyList properties(IWbemClassObject*, bool includeSpecialProperties = true);
		static const MethodList methods(IWbemClassObject*);
		static const QString getObjectText(IWbemClassObject*);
		void execMethod(const MethodDefinition&, void** args);

	protected:
		IWbemClassObject *m_object = nullptr, *m_classObject = nullptr;
		QMetaObject* m_metaObject = nullptr;
		std::map<int, QString> m_properties;
		std::map<int, MethodDefinition> m_methods;
	};

	////////////////////////////////////////////////////////////////////////////////

	class WmiNotification
	{
	public:
		WmiNotification();
		~WmiNotification();

	protected:
		struct impl;
		std::shared_ptr<impl> m_impl;
	};

	////////////////////////////////////////////////////////////////////////////////

} /* namespace tool */
