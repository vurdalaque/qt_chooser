#include "wmi.h"
#include <stdexcept>
#include <utility>
#define _WIN32_DCOM
#include <comdef.h>
#include <Wbemidl.h>

#pragma comment(lib, "wbemuuid.lib")

#include <ActiveQt/qaxtypes.h>
#include <private/qmetaobjectbuilder_p.h>

#include <atomic>

// debug includes
#include <iostream>
#include <QDebug>

namespace tool
{
	const QVariant nullWmiObject = QVariant::fromValue(static_cast<QObject*>(nullptr));

	////////////////////////////////////////////////////////////////////////////////

	QVariant::Type cim2qvariant(const CIMTYPE ct)
	{
		CIMTYPE_ENUMERATION ce = static_cast<CIMTYPE_ENUMERATION>(ct);
		switch (ce)
		{
		case CIM_SINT8:
		case CIM_SINT16:
		case CIM_SINT32:
		case CIM_SINT64:
			return QVariant::Int;
		case CIM_UINT8:
		case CIM_UINT16:
		case CIM_UINT32:
		case CIM_UINT64:
			return QVariant::UInt;
		case CIM_REAL32:
		case CIM_REAL64:
			return QVariant::Double;
		case CIM_BOOLEAN:
			return QVariant::Bool;
		case CIM_STRING:
			return QVariant::String;
		case CIM_DATETIME:
			return QVariant::DateTime;
		case CIM_REFERENCE:
		case CIM_CHAR16:
			return QVariant::Char;
		case CIM_FLAG_ARRAY:
			return QVariant::ByteArray;
		case CIM_OBJECT: {
			WmiObject* obj{ nullptr };
			return QVariant::fromValue(obj).type();
		}
		case CIM_EMPTY:
		case CIM_ILLEGAL:
			break;
		};
		return QVariant::Invalid;
	}

	template <class T>
	T cast(void* arg)
	{
		T result;
		std::memcpy(static_cast<void*>(&result), arg, sizeof(T));
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////

	VARIANT toVariant(const CIMTYPE ct, void* arg)
	{
		VARIANT _variant;
		::VariantInit(&_variant);

		CIMTYPE_ENUMERATION ce = static_cast<CIMTYPE_ENUMERATION>(ct);
		switch (ce)
		{
		case CIM_BOOLEAN:
			_variant.vt = VT_BOOL;
			_variant.boolVal = cast<bool>(arg);
			break;
		case CIM_SINT8:
			_variant.vt = VT_I1;
			_variant.cVal = cast<int8_t>(arg);
			break;
		case CIM_SINT16:
			_variant.vt = VT_I2;
			_variant.iVal = cast<int16_t>(arg);
			break;
		case CIM_SINT32:
			_variant.vt = VT_I4;
			_variant.lVal = cast<int32_t>(arg);
			break;
		case CIM_SINT64:
			_variant.vt = VT_I8;
			_variant.llVal = cast<int64_t>(arg);
			break;
		case CIM_UINT8:
		case CIM_UINT16: // kinda strange mapping
			_variant.vt = VT_UI1;
			_variant.bVal = cast<uint8_t>(arg);
			break;
		// case CIM_UINT16:
		// _variant.vt = VT_UI2;
		// _variant.uiVal = cast<uint16_t>(arg);
		// break;
		case CIM_UINT32:
			_variant.vt = VT_UI4;
			_variant.ulVal = cast<uint32_t>(arg);
			break;
		case CIM_UINT64:
			_variant.vt = VT_UI8;
			_variant.ullVal = cast<uint64_t>(arg);
			break;
		case CIM_REAL32:
			_variant.vt = VT_R4;
			_variant.fltVal = cast<float>(arg);
			break;
		case CIM_REAL64:
			_variant.vt = VT_R8;
			_variant.dblVal = cast<double>(arg);
			break;
		case CIM_CHAR16:
		case CIM_STRING: {
			QString* s = static_cast<QString*>(arg);
			_variant.vt = VT_BSTR;
			_variant.bstrVal = (!s->isEmpty()
					? ::SysAllocString(reinterpret_cast<const wchar_t*>(s->utf16()))
					: nullptr);
			break;
		}
		case CIM_OBJECT: {
			QVariant* value = static_cast<QVariant*>(arg);
			_variant.vt = VT_UNKNOWN;
			if (value->value<void*>() == nullptr)
			{
				_variant.punkVal = nullptr;
				break;
			}
			WmiObject* obj = static_cast<WmiObject*>(value->value<void*>());
			_variant.punkVal = obj->object();
			obj->object()->AddRef();
			break;
		}
		case CIM_DATETIME:
		case CIM_REFERENCE:
		case CIM_FLAG_ARRAY:
		case CIM_EMPTY:
		case CIM_ILLEGAL:
			qDebug() << "Unknown type!";
			throw std::runtime_error("failed to cast type");
		};
		return _variant;
	}

	void fromVariant(const CIMTYPE ct, VARIANT& _variant, void* arg)
	{
		void* result = static_cast<QVariant*>(arg)->value<void*>();
		CIMTYPE_ENUMERATION ce = static_cast<CIMTYPE_ENUMERATION>(ct);
		switch (ce)
		{
		case CIM_BOOLEAN:
			// _variant.vt = VT_BOOL;
			*static_cast<bool*>(result) = _variant.boolVal;
			break;
		case CIM_SINT8:
			// _variant.vt = VT_I1;
			*static_cast<int8_t*>(result) = _variant.cVal;
			break;
		case CIM_SINT16:
			// _variant.vt = VT_I2;
			*static_cast<int16_t*>(result) = _variant.iVal;
			break;
		case CIM_SINT32:
			// _variant.vt = VT_I4;
			*static_cast<int32_t*>(result) = _variant.lVal;
			break;
		case CIM_SINT64:
			// _variant.vt = VT_I8;
			*static_cast<int64_t*>(result) = _variant.llVal;
			break;
		case CIM_UINT8:
			// _variant.vt = VT_UI1;
			*static_cast<uint8_t*>(result) = _variant.bVal;
			break;
		case CIM_UINT16:
			// _variant.vt = VT_UI2;
			*static_cast<uint16_t*>(result) = _variant.uiVal;
			break;
		case CIM_UINT32:
			// _variant.vt = VT_UI4;
			*static_cast<uint32_t*>(result) = _variant.ulVal;
			break;
		case CIM_UINT64:
			// _variant.vt = VT_UI8;
			*static_cast<uint64_t*>(result) = _variant.ullVal;
			break;
		case CIM_REAL32:
			// _variant.vt = VT_R4;
			*static_cast<float*>(result) = _variant.fltVal;
			break;
		case CIM_REAL64:
			// _variant.vt = VT_R8;
			*static_cast<double*>(result) = _variant.dblVal;
			break;
		case CIM_CHAR16:
		case CIM_STRING: {
			QString* s = static_cast<QString*>(result);
			*s = QString::fromUtf16(reinterpret_cast<const ushort*>(_variant.bstrVal));
			// _variant.vt = VT_BSTR;
			break;
		}
		case CIM_OBJECT: {
			throw std::runtime_error{ "not implemented yet" };
			// WmiObject* obj = static_cast<WmiObject*>(result);
			// _variant.vt = VT_UNKNOWN;
			// _variant.punkVal = obj->object();
			break;
		}
		case CIM_DATETIME:
		case CIM_REFERENCE:
		case CIM_FLAG_ARRAY:
		case CIM_EMPTY:
		case CIM_ILLEGAL:
			qDebug() << "Unknown type!";
			throw std::runtime_error("failed to cast type");
		};
	}

	////////////////////////////////////////////////////////////////////////////////

	struct bstr_wrapper
	{
		BSTR m_string;
		bstr_wrapper() {} // -V730
		bstr_wrapper(BSTR&& s)
			: m_string(s)
		{
			qDebug() << "directalloc";
		}
		bstr_wrapper(const QString& s)
			: m_string(::SysAllocString(reinterpret_cast<const wchar_t*>(s.utf16())))
		{}
		~bstr_wrapper()
		{
			::SysFreeString(m_string);
		}
		operator BSTR*() { return &m_string; }
		operator const BSTR*() { return &m_string; }
		operator const BSTR() { return m_string; }
		operator QString() const { return QString::fromUtf16(reinterpret_cast<const ushort*>(m_string)); }
	};

	////////////////////////////////////////////////////////////////////////////////

	template <class F, class... Args>
	auto ComCallWrapper(const std::string& fname, F&& f, Args&&... args)
	{
		HRESULT hr = f(args...);
		if (FAILED(hr))
		{
			IWbemStatusCodeText* pStatus = NULL;

			HRESULT hres = CoCreateInstance(CLSID_WbemStatusCodeText, 0, CLSCTX_INPROC_SERVER,
				IID_IWbemStatusCodeText, (LPVOID*)&pStatus);

			QString error;

			if (S_OK == hres)
			{
				bstr_wrapper bstrError;
				hres = pStatus->GetErrorCodeText(hr, 0, 0, bstrError);

				if (S_OK != hres)
					bstrError = QString{ "Get last error failed" };

				error = bstrError;
				pStatus->Release();
			}
			std::cout << fname << " 0x" << std::hex << hr << " " << (error.isEmpty() ? "" : error.toUtf8().constData()) << std::endl;
			throw std::runtime_error(fname);
		}
		return hr;
	}

#define COM(foo, ...) ComCallWrapper(#foo, foo, __VA_ARGS__)
// msvc bind не распознает std::ref и не получается использовать invoke
#define COMP(foo, ptr, ...) ComCallWrapper(#foo, [&]() { return (ptr->*foo)(__VA_ARGS__); })

	////////////////////////////////////////////////////////////////////////////////

	CoInitialize::CoInitialize()
	{
		COM(CoInitializeEx, nullptr, COINIT_MULTITHREADED);
		COM(CoInitializeSecurity, nullptr,
			-1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE,
			nullptr, EOAC_NONE, nullptr);
	}

	CoInitialize::~CoInitialize()
	{
		services(RESET);
		locator(RESET);
		::CoUninitialize();
	}

	IWbemLocator* CoInitialize::locator(SingletonPolicy policy)
	{
		static IWbemLocator* singleton = nullptr;
		if (singleton == nullptr && policy == GET)
		{
			COM(CoCreateInstance,
				CLSID_WbemLocator,
				nullptr,
				CLSCTX_INPROC_SERVER,
				IID_IWbemLocator,
				(void**)&singleton);
		}
		if (singleton != nullptr && policy == RESET)
		{
			singleton->Release();
			singleton = nullptr;
		}
		return singleton;
	}

	IWbemServices* CoInitialize::services(SingletonPolicy policy)
	{
		static IWbemServices* singleton = nullptr;
		static const QString serv{ "ROOT\\CIMV2" };
		if (singleton == nullptr && policy == GET)
		{
			auto conn_string = ::SysAllocString(reinterpret_cast<const wchar_t*>(serv.utf16()));

			COMP(&IWbemLocator::ConnectServer, locator(),
				conn_string, nullptr, nullptr, 0, 0, 0, 0, &singleton);

			COM(&CoSetProxyBlanket,
				singleton, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
				nullptr, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
				nullptr, EOAC_NONE);
		}
		if (singleton != nullptr && policy == RESET)
		{
			singleton->Release();
			singleton = nullptr;
		}
		return singleton;
	}

	////////////////////////////////////////////////////////////////////////////////

	WmiProperty::operator QVariant() const { return value; }

	QVariant::Type WmiProperty::type() const { return value.type(); }

	QString WmiProperty::toString() const { return value.toString(); }

	const ushort* WmiProperty::utf16() const { return name.utf16(); }

	////////////////////////////////////////////////////////////////////////////////

	WmiObject::WmiObject()
	{
		setObjectName("WmiObject");
	}

	WmiObject::WmiObject(IWbemClassObject* o, const QString& objectName)
		: m_object(o)
	{
		setObjectName("WmiObject");
		auto wmiProperties = properties(o);
		// for (const auto& p : wmiProperties)
		// qDebug() << p.first << p.second;

		const auto nit = wmiProperties.find("CreationClassName");
		QString name = objectName;
		if (nit != wmiProperties.end() && !nit->second.toString().isEmpty())
			name = nit->second.toString();
		if (name.isEmpty())
			throw std::runtime_error("Failed to set class name");

		const QMetaObject* basic = QObject::metaObject();
		QMetaObjectBuilder b(basic);

		b.setClassName(name.toUtf8().constData());
		// b.setSuperClass(&QObject::staticMetaObject);
		b.setStaticMetacallFunction(&WmiObject::qt_static_metacall);

		bstr_wrapper className(name);
		COMP(&IWbemServices::GetObject, CoInitialize::services(),
			className, 0, NULL, &m_classObject, NULL);

		for (const auto& prop : wmiProperties)
		{
			int type = prop.second.type();
			auto pb = b.addProperty(prop.first.toUtf8().constData(), QVariant::typeToName(type));
			pb.setReadable(true);
			pb.setWritable(true);
			pb.setResettable(false);
			pb.setDesignable(false);
			pb.setScriptable(true);
			pb.setStored(false);
			pb.setEditable(false);
			pb.setUser(false);
			pb.setStdCppSet(false);
			pb.setEnumOrFlag(false);
			pb.setConstant(false);
			pb.setFinal(false);
			m_properties.insert({ pb.index(), prop.second });
		}

		for (const auto method : methods(m_classObject))
		{
			auto mb = b.addMethod(createSignature(method).toUtf8());
			// qDebug() << "sig: " << createSignature(method).toUtf8();
			mb.setReturnType("int");
			mb.setParameterNames(getParameterNames(method));
			mb.setAccess(QMetaMethod::Public);
			m_methods.insert({ mb.index(), method });
		}

		m_metaObject = b.toMetaObject();
	}

	WmiObject::WmiObject(const WmiObject& o)
		: m_object(o.m_object)
		, m_classObject(o.m_classObject)
		, m_properties(o.m_properties)
		, m_methods(o.m_methods)

	{
		if (m_object != nullptr)
			m_object->AddRef();
		if (m_classObject != nullptr)
			m_classObject->AddRef();
		if (o.metaObject() != nullptr)
			m_metaObject = QMetaObjectBuilder(o.metaObject()).toMetaObject();
	}

	WmiObject::~WmiObject()
	{
		if (m_object)
			m_object->Release();
		if (m_classObject)
			m_classObject->Release();
		if (m_metaObject)
			delete m_metaObject, m_metaObject = nullptr;
		m_properties.clear();
		m_methods.clear();
	}

	IWbemClassObject* WmiObject::object() const
	{
		return m_object;
	}

	WmiObject::operator QVariant() const
	{
		return QVariant::fromValue((void*)this);
	}

	void WmiObject::updateObject()
	{
		VARIANT vtPath;
		COMP(&IWbemClassObject::Get, m_object,
			bstr_wrapper{ "__PATH" }, 0, &vtPath, 0, 0);
		bstr_wrapper objectPath{ VARIANTToQVariant(vtPath, 0).toString() };
		VariantClear(&vtPath);
		m_object->Release();
		COMP(&IWbemServices::GetObject, CoInitialize::services(),
			objectPath, 0, NULL, &m_object, NULL);
	}

	WmiObject WmiObject::spawnInstance() const
	{
		IWbemClassObject* inst{ nullptr };
		COMP(&IWbemClassObject::SpawnInstance, m_object, 0, &inst);
		return WmiObject{ inst, this->metaObject()->className() };
	}

	int WmiObject::method(const QByteArray& signature, const QVariantList& vargs, Qt::ConnectionType ct) const
	{
		int retval = -1, mi = 0;
		const QMetaObject* o = metaObject();
		mi = o->indexOfMethod(signature);
		if (mi < 0)
			throw std::runtime_error("cannot find specified method");
		QMetaMethod method = o->method(mi);
		QGenericArgument args[10];
		for (int idx = 0; idx != (vargs.size() <= 9 ? vargs.size() : 9); ++idx)
			args[idx] = QGenericArgument(vargs[idx].typeName(), vargs[idx].data());

		if (!method.invoke(const_cast<tool::WmiObject*>(this),
				ct,
				Q_RETURN_ARG(int, retval),
				args[0], args[1], args[2], args[3], args[4],
				args[5], args[6], args[7], args[8], args[9]))
			throw std::runtime_error("method call failed");

		return retval;
	}

	WmiObject& WmiObject::operator=(const WmiObject& o)
	{
		if (this == &o)
			return *this;
		if (m_object)
			m_object->Release();
		if (m_classObject)
			m_classObject->Release();
		if (m_metaObject)
			delete m_metaObject, m_metaObject = nullptr;
		m_object = o.m_object;
		m_classObject = o.m_classObject;
		m_object->AddRef();
		m_classObject->AddRef();
		m_metaObject = QMetaObjectBuilder(o.metaObject()).toMetaObject();
		m_properties = o.m_properties;
		m_methods = o.m_methods;
		return *this;
	}

	WmiObject& WmiObject::operator=(WmiObject&& o)
	{
		if (this == &o)
			return *this;
		if (m_object)
			m_object->Release();
		if (m_classObject)
			m_classObject->Release();
		if (m_metaObject)
			delete m_metaObject, m_metaObject = nullptr;
		m_object = std::move(o.m_object);
		m_classObject = std::move(o.m_classObject);
		m_metaObject = std::move(o.m_metaObject);
		o.m_object = o.m_classObject = nullptr;
		o.m_metaObject = nullptr;
		m_properties = std::move(o.m_properties);
		m_methods = std::move(o.m_methods);
		return *this;
	}

	void WmiObject::execMethod(const MethodDefinition& m, void** args)
	{
		const MethodParameters
			&in = m.second.first,
			&out = m.second.second;

		IWbemClassObject
			*method = nullptr,
			*spawnInst = nullptr,
			*inParams = nullptr,
			*outParams = nullptr;
		// qt method return type was declared as int, so check if fits
		static_assert(sizeof(int) == sizeof(HRESULT));

		VARIANT vtPath;
		COMP(&IWbemClassObject::Get, m_object,
			bstr_wrapper{ "__PATH" }, 0, &vtPath, 0, 0);
		bstr_wrapper methodName{ m.first },
			objectPath{ VARIANTToQVariant(vtPath, 0).toString() };
		VariantClear(&vtPath);

		COMP(&IWbemClassObject::GetMethod, m_classObject,
			methodName, 0, &inParams, &method);

		// method accepts input arguments
		if (inParams != nullptr)
			for (size_t idx = 0; idx != in.size(); ++idx)
			{
				bstr_wrapper paramName{ in[idx].first };

				VARIANT vtInput;
				CIMTYPE cim;
				COMP(&IWbemClassObject::Get, inParams,
					paramName, 0, nullptr, &cim, 0);
				vtInput = toVariant(cim, args[idx + 1]);

				COMP(&IWbemClassObject::Put, inParams,
					paramName, 0, &vtInput, 0);
				VariantClear(&vtInput);
			}

		COMP(&IWbemClassObject::SpawnInstance, method, 0, &spawnInst);
		COMP(&IWbemServices::ExecMethod, CoInitialize::services(),
			objectPath, methodName, 0,
			// ctx   inParams  outParams   result
			nullptr, inParams, &outParams, nullptr);
		// hres = pSvc->ExecMethod(ClassName,
		//              MethodName, 0,
		// NULL, pClassInstance, &pOutParams, NULL);

		// method got result output
		if (outParams != nullptr)
		{
			// args[] = { returnValue.data(), val0.data(), val1.data(), ... }
			if (args[0] != nullptr)
			{
				// QVariant* ret = static_cast<QVariant*>(args[0]);
				COMP(&IWbemClassObject::Get, outParams,
					bstr_wrapper{ "ReturnValue" }, 0, &vtPath, 0, 0);
				*reinterpret_cast<int*>(args[0]) = std::move(vtPath.intVal);
				VariantClear(&vtPath);
			}
			else
				qDebug() << "no return value";

			for (size_t idx = 0; idx != out.size(); ++idx)
			{
				bstr_wrapper paramName{ out[idx].first };
				VARIANT vtResult;
				if (args[in.size() + idx + 1] == nullptr)
					throw std::runtime_error("cannot cast return value to variant");
				CIMTYPE cim;
				COMP(&IWbemClassObject::Get, outParams,
					paramName, 0, &vtResult, &cim, 0);
				fromVariant(cim, vtResult, args[in.size() + idx + 1]);
				qDebug() << out[idx].first << out[idx].second;
				VariantClear(&vtResult);
			}
		}
		else
			throw std::runtime_error("failed to set return value");
		method->Release();
		spawnInst->Release();
		if (inParams != nullptr)
			inParams->Release();
		if (outParams != nullptr)
			outParams->Release();
	}

	////////////////////////////////////////////////////////////////////////////////
	// static functions

	const WmiObject::List WmiObject::objects(const QString& name)
	{
		// QString request = QString{ "SELECT * FROM Win32_%1" }.arg(name.mid(0, 1).toUpper() + name.mid(1).toLower());
		// auto brequest = ::SysAllocString(reinterpret_cast<const wchar_t*>(request.utf16()));
		// COMP(&IWbemServices::ExecQuery, svc, bstr_t("WQL"), brequest, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
		bstr_wrapper brequest(QString{ "Win32_%1" }.arg(name.mid(0, 1).toUpper() + name.mid(1).toLower()));
		IEnumWbemClassObject* pEnumerator = NULL;

		COMP(&IWbemServices::CreateInstanceEnum, CoInitialize::services(), brequest,
			WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
			NULL,
			&pEnumerator);

		IWbemClassObject* obj = NULL;
		ULONG uReturn = 0;

		std::deque<WmiObject> result;
		while (pEnumerator)
		{
			COMP(&IEnumWbemClassObject::Next, pEnumerator,
				WBEM_INFINITE, 1, &obj, &uReturn);
			if (0 == uReturn)
				break;
			result.emplace_back(WmiObject(obj));
		}

		return result;
	}

	WmiObject WmiObject::object(const QString& name)
	{
		bstr_wrapper objectName{ QString{ "Win32_%1" }.arg(name) };
		IWbemClassObject* object{ nullptr };
		COMP(&IWbemServices::GetObject, CoInitialize::services(),
			objectName, 0, NULL, &object, NULL);
		return WmiObject{ object, QString{ "Win32_%1" }.arg(name) };
	}

	const std::deque<std::pair<QString, QVariant>> WmiObject::qualifiers(IWbemClassObject* object)
	{
		std::deque<std::pair<QString, QVariant>> result;
		if (object == nullptr)
			return result;
		IWbemQualifierSet* set = nullptr;
		COMP(&IWbemClassObject::GetQualifierSet, object, &set);
		COMP(&IWbemQualifierSet::BeginEnumeration, set, 0);
		while (1)
		{
			bstr_wrapper propstr;
			VARIANT vtProp;
			if (WBEM_S_NO_MORE_DATA == COMP(&IWbemQualifierSet::Next, set, 0, propstr, &vtProp, nullptr))
				break;
			QVariant value = VARIANTToQVariant(vtProp, 0);
			VariantClear(&vtProp);
			result.push_back({ static_cast<QString>(propstr), value });
		}
		COMP(&IWbemQualifierSet::EndEnumeration, set);
		set->Release();
		return result;
	}

	const WmiObject::PropertyList WmiObject::properties(IWbemClassObject* object, bool withHiddenProps)
	{
		PropertyList result;
		if (object == nullptr)
			return result;
		COMP(&IWbemClassObject::BeginEnumeration, object, 0);
		while (1)
		{
			bstr_wrapper strName;
			if (WBEM_S_NO_MORE_DATA == COMP(&IWbemClassObject::Next, object, 0, strName, nullptr, nullptr, nullptr))
				break;
			if (withHiddenProps && static_cast<QString>(strName).startsWith("__"))
				continue;

			VARIANT vtProp;
			CIMTYPE cim;
			COMP(&IWbemClassObject::Get, object, strName, 0, &vtProp, &cim, 0);
			QVariant value = VARIANTToQVariant(vtProp, 0);
			VariantClear(&vtProp);
			if (value.type() == QVariant::Invalid)
				value = QVariant{ cim2qvariant(cim) };

			result.insert({ static_cast<QString>(strName), WmiProperty{ strName, value, cim } });
		}
		COMP(&IWbemClassObject::EndEnumeration, object);
		return result;
	}

	const QString WmiObject::getObjectText(IWbemClassObject* object)
	{
		if (object == nullptr)
			return QString{ "nullptr" };

		bstr_wrapper text;
		COMP(&IWbemClassObject::GetObjectText, object, 0, text);
		return static_cast<QString>(text);
	}

	const WmiObject::MethodList WmiObject::methods(IWbemClassObject* object)
	{
		MethodList result;
		if (object == nullptr)
			return result;
		COMP(&IWbemClassObject::BeginMethodEnumeration, object, 0);
		while (1)
		{
			bstr_wrapper strName;
			IWbemClassObject *in = nullptr, *out = nullptr;
			if (WBEM_S_NO_MORE_DATA == COMP(&IWbemClassObject::NextMethod, object, 0, strName, &in, &out))
				break;

			MethodParameters inparams, outparams;
			auto inprops = properties(in),
				 outprops = properties(out);
			if (in != nullptr)
				in->Release();
			if (out != nullptr)
				out->Release();
			for (const auto& ip : inprops)
				inparams.push_back(ip);
			for (const auto& op : outprops)
			{
				if (op.first == "ReturnValue")
					continue;
				outparams.push_back(op);
			}

			result.insert({ static_cast<QString>(strName), { inparams, outparams } });
		}
		COMP(&IWbemClassObject::EndMethodEnumeration, object);
		return result;
	}

	QString WmiObject::createSignature(const MethodDefinition& md)
	{
		const auto getType = [](const QVariant& v) -> auto
		{
			if (v.type() == QVariant::Invalid)
				return QString{ "QVariant" };
			QString type{ v.typeName() };
			if (type.isEmpty())
				throw std::runtime_error("typename empty");
			return type;
		};

		QString result;

		QTextStream stream(&result);
		stream << md.first << "(";
		const MethodParameters &constParams = md.second.first,
							   &refParams = md.second.second;
		for (auto iter = constParams.cbegin(); iter != constParams.cend(); ++iter)
		{
			stream << getType(iter->second);
			if (std::next(iter) != constParams.cend())
				stream << ",";
		}
		if (!constParams.empty() && !refParams.empty())
			stream << ",";
		for (auto iter = refParams.cbegin(); iter != refParams.cend(); ++iter)
		{
			stream << getType(iter->second) << "&";
			if (std::next(iter) != refParams.cend())
				stream << ",";
		}
		stream << ")";
		return QMetaObject::normalizedSignature(result.toUtf8());
	}

	QList<QByteArray> WmiObject::getParameterNames(const MethodDefinition& md)
	{
		QList<QByteArray> result;
		const MethodParameters &constParams = md.second.first,
							   &refParams = md.second.second;
		for (auto iter = constParams.cbegin(); iter != constParams.cend(); ++iter)
			result.push_back(QString{ "in_%1" }.arg(iter->first).toUtf8());
		for (auto iter = refParams.cbegin(); iter != refParams.cend(); ++iter)
			result.push_back(QString{ "out_%1" }.arg(iter->first).toUtf8());
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	// qt specific overrides

	const QMetaObject* WmiObject::metaObject() const
	{
		return m_metaObject;
	}

	int WmiObject::qt_metacall(QMetaObject::Call c, int id, void** arg)
	{
		static const QStringList explain{ "InvokeMetaMethod", "ReadProperty", "WriteProperty", "ResetProperty", "QueryPropertyDesignable", "QueryPropertyScriptable", "QueryPropertyStored", "QueryPropertyEditable", "QueryPropertyUser", "CreateInstance", "IndexOfMethod", "RegisterPropertyMetaType", "RegisterMethodArgumentMetaType" };

		const auto propIt = m_properties.find(id);

		if (c == QMetaObject::ReadProperty && propIt != m_properties.end())
		{ // ReadProperty { 0, &QVariant{}, &(int)status }

			VARIANT vtProp;
			COMP(&IWbemClassObject::Get, m_object,
				reinterpret_cast<const wchar_t*>(propIt->second.utf16()), 0, &vtProp, 0, 0);
			QVariant value = VARIANTToQVariant(vtProp, 0);
			VariantClear(&vtProp);
			QVariant* ret = reinterpret_cast<QVariant*>(arg[1]);
			reinterpret_cast<int&>(arg[2]) = 0;
			ret->create(value.type(), value.constData());
			return 0;
		}
		if (c == QMetaObject::WriteProperty && propIt != m_properties.end())
		{ // WriteProperty { void* constData, qvariant *newValue, int &status, int &flags }
			QVariant* value = static_cast<QVariant*>(arg[1]);
			VARIANT vtProp = toVariant(propIt->second.cimType, value->data());
			COMP(&IWbemClassObject::Put, m_object,
				reinterpret_cast<const wchar_t*>(propIt->second.utf16()), 0, &vtProp, 0);
			VariantClear(&vtProp);
			return 0;
		}
		if (c == QMetaObject::InvokeMetaMethod)
		{
			auto m = m_methods.find(id);
			if (m == m_methods.end()) // standart qt's method
				return QObject::qt_metacall(c, id, arg);

			execMethod(m->second, arg);
			return 0;
		}

		qDebug() << "qt_metacall(" << explain[static_cast<size_t>(c)].toUtf8().constData() << ", " << id << ", ...)";
		return QObject::qt_metacall(c, id, arg);
	}

	void* WmiObject::qt_metacast(const char* name)
	{
		qDebug() << "qt_metacast " << name;
		// if (our_name == name) return this;
		return QObject::qt_metacast(name);
	}

	void WmiObject::qt_static_metacall(QObject* object, QMetaObject::Call call, int id, void** arg)
	{
		WmiObject* w = dynamic_cast<WmiObject*>(object);
		if (w == nullptr)
			throw std::runtime_error("objects arent castable to WmiObject");
		w->qt_metacall(call, id, arg);
	}

	////////////////////////////////////////////////////////////////////////////////

	struct WmiNotification::impl : public IWbemObjectSink
	{
		std::atomic_int m_refCount;
		virtual ULONG STDMETHODCALLTYPE AddRef() override
		{
			return m_refCount++;
		}

		virtual ULONG STDMETHODCALLTYPE Release() override
		{
			auto result = (m_refCount--);
			if (result == 0)
				delete this;
			return result;
		}

		virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override
		{
			if (riid == IID_IUnknown || riid == IID_IWbemObjectSink)
			{
				*ppv = (IWbemObjectSink*)this;
				AddRef();
				return WBEM_S_NO_ERROR;
			}
			else
				return E_NOINTERFACE;
		}

		virtual HRESULT STDMETHODCALLTYPE Indicate(
			/* [in] */ LONG lObjectCount,
			/* [size_is][in] */ IWbemClassObject __RPC_FAR* __RPC_FAR* apObjArray) override
		{
			qDebug() << "indicate!";
			Q_UNUSED(lObjectCount);
			Q_UNUSED(apObjArray);
			return WBEM_S_NO_ERROR;
		}

		virtual HRESULT STDMETHODCALLTYPE SetStatus(
			/* [in] */ LONG lFlags,
			/* [in] */ HRESULT hResult,
			/* [in] */ BSTR strParam,
			/* [in] */ IWbemClassObject __RPC_FAR* pObjParam) override
		{
			qDebug() << "set status";
			Q_UNUSED(lFlags);
			Q_UNUSED(hResult);
			Q_UNUSED(strParam);
			Q_UNUSED(pObjParam);
			return WBEM_S_NO_ERROR;
		}
	};

	////////////////////////////////////////////////////////////////////////////////

	WmiNotification::WmiNotification()
		: m_impl{ std::make_shared<impl>() }
	{
		qDebug() << "hello world";
	}

	WmiNotification::~WmiNotification()
	{}

	////////////////////////////////////////////////////////////////////////////////
} /* namespace tool */
