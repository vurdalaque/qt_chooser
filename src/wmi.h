#pragma once
#include <QObject>
#include <QVariant>

#include <deque>
#include <set>

struct IWbemLocator;
struct IWbemServices;
struct IWbemClassObject;

using Locator = std::shared_ptr<IWbemLocator>;
using Services = std::shared_ptr<IWbemServices>;
using ClassObject = std::shared_ptr<IWbemClassObject>;

namespace tool
{
	////////////////////////////////////////////////////////////////////////////////

	template <class T>
	QVariant ref(T& v)
	{ // wrap output values for wmi methods. hide rtti killer
		return QVariant::fromValue((void*)&v);
	}

	extern const QVariant nullWmiObject;

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

	struct WmiProperty
	{
		QString name;
		QVariant value{ QVariant::Invalid };
		int cimType{ 0 };

		operator QVariant() const;
		QVariant::Type type() const;
		QString toString() const;
		const ushort* utf16() const;
	};

	////////////////////////////////////////////////////////////////////////////////

	class WmiObject : public QObject
	{
	public:
		virtual const QMetaObject* metaObject() const override;
		virtual void* qt_metacast(const char*) override;
		virtual int qt_metacall(QMetaObject::Call, int, void**) override;
		static void qt_static_metacall(QObject*, QMetaObject::Call, int, void**);

	public:
		using List = std::deque<WmiObject>;
		using PropertyList = std::map<QString, WmiProperty>;
		using MethodParameters = std::deque<std::pair<QString, QVariant>>;
		using MethodDefinition = std::pair<QString, std::pair<MethodParameters, MethodParameters>>;
		using MethodList = std::set<MethodDefinition>;

		WmiObject();
		WmiObject(const WmiObject&);
		~WmiObject();
		IWbemClassObject* object() const;
		operator QVariant() const;
		void updateObject();
		WmiObject spawnInstance() const;
		int method(const QByteArray&, const QVariantList& = QVariantList{}, Qt::ConnectionType = Qt::DirectConnection) const;

		WmiObject& operator=(const WmiObject&);
		WmiObject& operator=(WmiObject&&);

		static const List objects(const QString&);
		static WmiObject object(const QString&);

	protected:
		WmiObject(IWbemClassObject*, const QString& = QString{});
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
		std::map<int, WmiProperty> m_properties;
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
Q_DECLARE_METATYPE(tool::WmiObject);
