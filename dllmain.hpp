#pragma once
#include "pch.hpp"
#include "Framework/Printer.hpp"
#include "Engine/Engine.hpp"

class UnrealObject
{
protected:
	static std::vector<char> m_unsafeChars;
	static std::vector<std::string> m_unsafeNames;

public:
	EClassTypes Type;
	class UObject* Object;
	class UObject* Package;
	std::string FullName;
	std::string ValidName;

public:
	UnrealObject();
	UnrealObject(class UObject* uObject, bool bIsPackage = false);
	UnrealObject(const UnrealObject& unrealObj);
	~UnrealObject();

public:
	bool IsValid() const;
	std::string Hash() const;
	static void ValidateName(std::string& name);
	static std::string CreateValidName(std::string name);

private:
	void Assign(class UObject* uObject, bool bIsPackage);
	void AssignType();
	void AssignName();

public:
	bool operator>(const UnrealObject& unrealObj);
	bool operator<(const UnrealObject& unrealObj);
	bool operator==(const UnrealObject& unrealObj);
	bool operator!=(const UnrealObject& unrealObj);
	UnrealObject& operator=(const UnrealObject& unrealObj);
};

class UnrealProperty
{
public:
	EPropertyTypes Type;
	class UProperty* Property;
	std::string ValidName;

public:
	UnrealProperty();
	UnrealProperty(class UProperty* uProperty);
	UnrealProperty(const UnrealProperty& unrealProp);
	~UnrealProperty();

public:
	bool IsValid() const;
	std::string Hash() const;

public:
	bool IsContainer() const;
	bool IsParameter() const;
	bool IsReturnParameter() const;
	bool IsOutParameter() const;
	bool IsOptionalParameter() const;
	bool IsAnArray() const;
	bool CantConst() const;
	bool CantReference() const;
	bool CantMemcpy() const;
	bool IsBitfield() const;
public:
	size_t GetSize() const;
	std::string GetType(bool bIgnoreEnum, bool bFunctionParam, bool bIgnoreConst) const;
	std::string GetTypeForClass() const;
	std::string GetTypeForStruct() const;
	std::string GetTypeForParameter(bool bIgnoreConst = false) const;

private:
	void Assign(class UProperty* uProperty);
	void AssignType();
	void AssignName();

public:
	bool operator>(const UnrealProperty& unrealProp);
	bool operator<(const UnrealProperty& unrealProp);
	bool operator==(const UnrealProperty& unrealProp);
	bool operator!=(const UnrealProperty& unrealProp);
	UnrealProperty& operator=(const UnrealProperty& unrealProp);
};

namespace std
{
	template<>
	struct hash<UnrealObject>
	{
		size_t operator()(const UnrealObject& unrealObj) const
		{
			return hash<string>()(unrealObj.Hash());
		}
	};

	template<>
	struct hash<UnrealProperty>
	{
		size_t operator()(const UnrealProperty& unrealProp) const
		{
			return hash<string>()(unrealProp.Hash());
		}
	};
}

// Caches all need objects and strings for sdk generation, caching is only needed to be done one time on initialization.
class GCache
{
private:
	using ObjectCountKey = std::pair<std::string, class UClass*>;

	struct ObjectCountKeyHash
	{
		size_t operator()(const ObjectCountKey& key) const noexcept
		{
			size_t nameHash = std::hash<std::string>{}(key.first);
			size_t classHash = std::hash<class UClass*>{}(key.second);
			return (nameHash ^ (classHash << 1));
		}
	};

	static inline std::unordered_map<ObjectCountKey, uint32_t, ObjectCountKeyHash> m_objects;
	static inline std::unordered_map<class UObject*, std::vector<UnrealObject>> m_consts;
	static inline std::unordered_map<class UObject*, std::vector<UnrealObject>> m_enums;
	static inline std::unordered_map<class UObject*, std::vector<UnrealObject>> m_structs;
	static inline std::unordered_map<class UObject*, std::vector<UnrealObject>> m_classes;
	static inline std::unordered_map<class UObject*, std::unordered_set<class UObject*>> m_constHashes;
	static inline std::unordered_map<class UObject*, std::unordered_set<class UObject*>> m_enumHashes;
	static inline std::unordered_map<class UObject*, std::unordered_set<class UObject*>> m_structHashes;
	static inline std::unordered_map<class UObject*, std::unordered_set<class UObject*>> m_classHashes;
	static inline std::map<std::string, class UObject*> m_constants;
	static inline std::unordered_map<class UObject*, std::pair<std::string, class UObject*>> m_constantLookup;
	static inline std::vector<UnrealObject> m_packages;
	static inline std::unordered_set<class UObject*> m_cachedPackages;

public:
	static void Initialize();
	static void ClearCache();
	static std::vector<UnrealObject>* GetCache(class UObject* packageObj, EClassTypes type);
	static std::map<std::string, class UObject*>* GetConstants();
	static std::vector<UnrealObject>* GetPackages();

public:
	static std::pair<std::string, class UObject*> GetConstant(const UnrealObject& unrealObj);
	static UnrealObject GetClass(const std::string& classFullName);
	template<typename T> static uint32_t CountObject(const std::string& objectName)
	{
		ObjectCountKey objectPair{ objectName, T::StaticClass() };
		auto objectIt = m_objects.find(objectPair);

		if (objectIt != m_objects.end())
		{
			return objectIt->second;
		}

		return 0;
	}

private:
	static void CacheObject(UnrealObject& unrealObj);
	static void CacheConstant(UnrealObject& unrealObj);
	static void CacheCount(UnrealObject& unrealObj);
	static std::unordered_map<class UObject*, std::unordered_set<class UObject*>>* GetCacheHashes(EClassTypes type);

public:
	GCache() = delete;
};

// This is only used for the log file if you have "NO_LOGGING" comment out in your config.
class GLogger
{
private:
	static inline std::ofstream m_file;

public:
	static bool Open();
	static void Close();
	static void Flush();

public:
	static void Log(const std::string& str, bool bFlush = false);
	static void LogObject(const std::string& title, const UnrealObject& unrealObj);
	static void LogClassSize(class UClass* uClass, size_t localSize);
	static void LogStructPadding(class UScriptStruct* uScriptStruct, size_t padding);

public:
	GLogger() = delete;
};

namespace Utils
{
	void MessageboxInfo(const std::string& message);
	void MessageboxWarn(const std::string& message);
	void MessageboxError(const std::string& message);

	bool SortProperty(const UnrealProperty& unrealPropA, const UnrealProperty& unrealPropB);
	bool SortPropertyPair(const std::pair<UnrealProperty, std::string>& pairA, const std::pair<UnrealProperty, std::string>& pairB);
}

namespace Retrievers
{
	void GetAllFunctionFlags(std::ostringstream& stream, uint64_t functionFlags);
	void GetAllPropertyFlags(std::ostringstream& stream, uint64_t propertyFlags);

	uintptr_t GetBaseAddress();
	uintptr_t GetOffset(void* pointer);
	uintptr_t FindPattern(const uint8_t* pattern, const std::string& mask);
}

namespace ConstGenerator
{
	void GenerateConstName(UnrealObject* unrealObj);
	void GenerateConst(std::ofstream& stream, const UnrealObject& unrealObj);
	void ProcessConsts(std::ofstream& stream, class UObject* packageObj);
}

namespace EnumGenerator
{
	void GenerateEnumName(UnrealObject* unrealObj);
	void GenerateEnum(std::ofstream& stream, const UnrealObject& unrealObj);
	void ProcessEnums(std::ofstream& stream, class UObject* packageObj);
}

namespace StructGenerator
{
	void GenerateStructMembers(std::ofstream& structStream, EClassTypes type);
	void GenerateStruct(std::ofstream& stream, const UnrealObject& unrealObj);
	void GenerateStructPre(std::ofstream& stream, const UnrealObject& unrealObj);
	void ProcessStructs(std::ofstream& stream, class UObject* packageObj);
}

namespace ClassGenerator
{
	void GenerateClassMembers(std::ostringstream& classStream, class UClass* uClass, EClassTypes classType);
	void GenerateClass(std::ofstream& stream, const UnrealObject& unrealObj);
	void GenerateClassPre(std::ofstream& stream, const UnrealObject& unrealObj, class UObject* packageObj);
	void ProcessClasses(std::ofstream& stream, class UObject* packageObj);
}

namespace ParameterGenerator
{
	void GenerateParameter(std::ofstream& stream, const UnrealObject& unrealObj);
	void ProcessParameters(std::ofstream& stream, class UObject* packageObj);
}

namespace FunctionGenerator
{
	void GenerateVirtualFunctions(std::ofstream& stream);
	void GenerateFunctionCode(std::ofstream& stream, const UnrealObject& unrealObj);
	void GenerateFunctionParameters(std::ofstream& stream, const UnrealObject& unrealObj);
	void ProcessFunctions(std::ofstream& stream, class UObject* packageObj);
}

namespace Generator
{
	void GenerateConstants();
	void GenerateHeaders();
	void GenerateDefines();
	void ProcessPackages(const std::filesystem::path& headersDirectory, const std::filesystem::path& sourceDirectory);
	void GenerateSDK();

	bool Initialize(bool bCreateLog);
	void DumpInstances(bool bNames, bool bObjects);
	void DumpGObjects();
	void DumpGNames();

	bool AreGObjectsValid();
	bool AreGNamesValid();
	bool AreGlobalsValid();
}
