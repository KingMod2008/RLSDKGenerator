#include "dllmain.hpp"

static constexpr uint32_t UNKNOWN_DATA_SPACING = 2;
static constexpr uint32_t LOG_FILE_SPACING = 75;
static constexpr uint32_t CONST_VALUE_SPACING = 175;
static constexpr uint32_t INSTANCE_DUMP_SPACING = 50;

std::vector<char> UnrealObject::m_unsafeChars = {
    '`', '!', '@', '#', '$',  '%', '^', '&', '*',  '(', ')', '-', '=', '+',
    '[', '{', ']', '}', '\\', '|', ';', ':', '\'', '"', ',', '/', '?'};

// These are functions that share the same names as macros, which will result in
// errors in the final sdk due to conflicting names.
std::vector<std::string> UnrealObject::m_unsafeNames = {
    "GetCurrentTime", "GetObject",   "DeleteFile",
    "DrawText",       "SendMessage", "GetMessage"};

UnrealObject::UnrealObject()
    : Type(EClassTypes::Unknown), Object(nullptr), Package(nullptr) {}

UnrealObject::UnrealObject(class UObject *uObject, bool bIsPackage)
    : Type(EClassTypes::Unknown), Object(nullptr), Package(nullptr) {
  Assign(uObject, bIsPackage);
}

UnrealObject::UnrealObject(const UnrealObject &unrealObj)
    : Type(unrealObj.Type), Object(unrealObj.Object),
      Package(unrealObj.Package), FullName(unrealObj.FullName),
      ValidName(unrealObj.ValidName) {}

UnrealObject::~UnrealObject() {}

bool UnrealObject::IsValid() const {
  return ((Type != EClassTypes::Unknown) && Object && Package);
}

std::string UnrealObject::Hash() const {
  if (IsValid()) {
    return (std::to_string(static_cast<int32_t>(Type)) + "." + FullName + "." +
            ValidName);
  }

  return "";
}

void UnrealObject::ValidateName(std::string &name) {
  if (!name.empty()) {
    for (char &a : name) {
      for (char b : m_unsafeChars) {
        if (a == b) {
          a = '_';
          break;
        }
      }
    }

    if (name == "_") // This is common for auto generated lambda functions which
                     // can be really annoying.
    {
      name = "instance";
    }
  }
}

std::string UnrealObject::CreateValidName(std::string name) {
  ValidateName(name);
  return name;
}

void UnrealObject::Assign(class UObject *uObject, bool bIsPackage) {
  if (uObject) {
    UObject *packageObj = (bIsPackage ? uObject : uObject->GetPackageObj());

    if (packageObj) {
      Object = uObject;
      Package = packageObj;
      ValidName = Object->GetName();

      if (!bIsPackage) {
        AssignType();
      } else {
        Type = EClassTypes::UObject;
      }

      if (Type != EClassTypes::Unknown) {
        FullName = Object->GetFullName();
        AssignName();
      }
    }
  }
}

void UnrealObject::AssignType() {
  if (Object) {
    if (Object->IsA<UConst>()) {
      ConstGenerator::GenerateConstName(this);

      if (!ValidName.empty()) {
        Type = EClassTypes::UConst;
      }
    } else if (Object->IsA<UEnum>()) {
      EnumGenerator::GenerateEnumName(this);

      if (!ValidName.empty()) {
        Type = EClassTypes::UEnum;
      }
    } else if (Object->IsA<UScriptStruct>()) {
      if ((ValidName.find("Default__") == std::string::npos) &&
          (ValidName.find("<uninitialized>") == std::string::npos)) {
        Type = EClassTypes::UStruct;
      }
    } else if (Object->IsA<UClass>()) {
      if ((ValidName.find("Default__") == std::string::npos) &&
          (ValidName.find("<uninitialized>") == std::string::npos)) {
        Type = EClassTypes::UClass;
      }
    } else if (Object->IsA<UFunction>()) {
      Type = EClassTypes::UFunction;
    }
  }
}

void UnrealObject::AssignName() {
  ValidateName(FullName);

  if (!ValidName.empty()) {
    ValidateName(ValidName);

    if (GConfig::UsingWindows() && (Type == EClassTypes::UFunction)) {
      for (const std::string &name : m_unsafeNames) {
        if (ValidName.find(name) != std::string::npos) {
          ValidName += "W";
          break;
        }
      }
    }
  }
}

bool UnrealObject::operator>(const UnrealObject &unrealObj) {
  return (Hash() > unrealObj.Hash());
}

bool UnrealObject::operator<(const UnrealObject &unrealObj) {
  return (Hash() < unrealObj.Hash());
}

bool UnrealObject::operator==(const UnrealObject &unrealObj) {
  return (Hash() == unrealObj.Hash());
}

bool UnrealObject::operator!=(const UnrealObject &unrealObj) {
  return (Hash() != unrealObj.Hash());
}

UnrealObject &UnrealObject::operator=(const UnrealObject &unrealObj) {
  Type = unrealObj.Type;
  Object = unrealObj.Object;
  Package = unrealObj.Package;
  FullName = unrealObj.FullName;
  ValidName = unrealObj.ValidName;
  return *this;
}

UnrealProperty::UnrealProperty()
    : Type(EPropertyTypes::Unknown), Property(nullptr) {}

UnrealProperty::UnrealProperty(class UProperty *uProperty)
    : Type(EPropertyTypes::Unknown), Property(nullptr) {
  Assign(uProperty);
}

UnrealProperty::UnrealProperty(const UnrealProperty &unrealProp)
    : Type(unrealProp.Type), Property(unrealProp.Property),
      ValidName(unrealProp.ValidName) {}

UnrealProperty::~UnrealProperty() {}

bool UnrealProperty::IsValid() const {
  return ((Type != EPropertyTypes::Unknown) && Property &&
          (Property->PropertySize > 0));
}

std::string UnrealProperty::Hash() const {
  if (IsValid()) {
    return (std::to_string(static_cast<int32_t>(Type)) + "." +
            std::to_string(reinterpret_cast<uintptr_t>(Property)) + "." +
            ValidName);
  }

  return "";
}

bool UnrealProperty::IsContainer() const {
  if (IsValid()) {
    return ((Type == EPropertyTypes::FName) ||
            (Type == EPropertyTypes::FString) ||
            (Type == EPropertyTypes::FScriptDelegate) ||
            (Type == EPropertyTypes::FStruct) ||
            (Type == EPropertyTypes::TArray) || (Type == EPropertyTypes::TMap));
  }

  return false;
}

bool UnrealProperty::IsParameter() const {
  if (IsValid()) {
    return (Property->PropertyFlags & EPropertyFlags::CPF_Parm);
  }

  return false;
}

bool UnrealProperty::IsReturnParameter() const {
  if (IsValid()) {
    return (IsParameter() &&
            (Property->PropertyFlags & EPropertyFlags::CPF_ReturnParm));
  }

  return false;
}

bool UnrealProperty::IsOutParameter() const {
  if (IsValid()) {
    return (IsParameter() &&
            (Property->PropertyFlags & EPropertyFlags::CPF_OutParm));
  }

  return false;
}

bool UnrealProperty::IsOptionalParameter() const {
  if (IsValid()) {
    return (IsParameter() &&
            (Property->PropertyFlags & EPropertyFlags::CPF_OptionalParm));
  }

  return false;
}

bool UnrealProperty::IsAnArray() const {
  if (IsValid()) {
    return (Property->ArraySize > 1);
  }

  return false;
}

bool UnrealProperty::IsBitfield() const {
  if (!IsValid())
    return false;

  if (Property->IsA<UBoolProperty>()) {
    if (auto boolProp = (UBoolProperty *)Property) {
      return boolProp->BitMask != 0;
    }
  }

  return false;
}

bool UnrealProperty::CantConst() const {
  if (IsValid()) {
    return (
        IsReturnParameter() || IsOutParameter() ||
        ((Type != EPropertyTypes::FName) && (Type != EPropertyTypes::FString) &&
         (Type != EPropertyTypes::FScriptDelegate) &&
         (Type != EPropertyTypes::FStruct) &&
         (Type != EPropertyTypes::TArray) && (Type != EPropertyTypes::TMap)));
  }

  return false;
}

bool UnrealProperty::CantReference() const {
  if (IsValid()) {
    return IsAnArray();
  }

  return false;
}

bool UnrealProperty::CantMemcpy() const {
  if (IsValid()) {
    return ((Type == EPropertyTypes::Bool) ||
            (!IsAnArray() && ((Type == EPropertyTypes::UClass) ||
                              (Type == EPropertyTypes::UObject) ||
                              (Type == EPropertyTypes::UInterface))));
  }

  return false;
}

size_t UnrealProperty::GetSize() const {
  if (IsValid()) {
    if (Type == EPropertyTypes::Int32) {
      return sizeof(int32_t);
    } else if (Type == EPropertyTypes::UInt8) {
      return Property->PropertySize;
    } else if (Type == EPropertyTypes::UInt32) {
      return sizeof(uint32_t);
    } else if (Type == EPropertyTypes::UInt64) {
      return sizeof(uint64_t);
    } else if (Type == EPropertyTypes::Double) {
      return sizeof(double);
    } else if (Type == EPropertyTypes::Float) {
      return sizeof(float);
    } else if (Type == EPropertyTypes::Bool) {
      return Property->PropertySize;
    } else if (Type == EPropertyTypes::FName) {
      return sizeof(FName);
    } else if (Type == EPropertyTypes::FString) {
      return sizeof(FString);
    } else if (Type == EPropertyTypes::FScriptDelegate) {
      return sizeof(FScriptDelegate);
    } else if (Type == EPropertyTypes::FStruct) {
      return Property->PropertySize;
    } else if (Type == EPropertyTypes::UClass) {
      return sizeof(uintptr_t);
    } else if (Type == EPropertyTypes::UObject) {
      return sizeof(uintptr_t);
    } else if (Type == EPropertyTypes::UInterface) {
      return sizeof(uintptr_t);
    } else if (Type == EPropertyTypes::TArray) {
      return sizeof(TArray<uintptr_t>);
    } else if (Type == EPropertyTypes::TMap) {
      return Property->PropertySize;
    }
  }

  return 0;
}

std::string UnrealProperty::GetType(bool bIgnoreEnum, bool bFunctionParam,
                                    bool bIgnoreConst) const {
  std::string typeStr;

  if (IsValid()) {
    if (Type == EPropertyTypes::Int32) {
      typeStr = "int32_t";
    } else if (Type == EPropertyTypes::UInt8) {
      if (!bIgnoreEnum && GConfig::UsingEnumClasses()) {
        UByteProperty *byteProperty = static_cast<UByteProperty *>(Property);

        if (byteProperty && byteProperty->Enum) {
          UnrealObject enumObj(byteProperty->Enum);

          if (enumObj.IsValid()) {
            typeStr = enumObj.ValidName;
          }
        }
      }

      if (typeStr.empty()) {
        typeStr = "uint8_t";
      }
    } else if (Type == EPropertyTypes::UInt32) {
      typeStr = "uint32_t";
    } else if (Type == EPropertyTypes::UInt64) {
      typeStr = "uint64_t";
    } else if (Type == EPropertyTypes::Double) {
      typeStr = "double";
    } else if (Type == EPropertyTypes::Float) {
      typeStr = "float";
    } else if (Type == EPropertyTypes::Bool) {
      if (bFunctionParam || IsReturnParameter()) {
        typeStr = "bool";
      } else {
        typeStr = "uint32_t";
      }
    } else if (Type == EPropertyTypes::FName) {
      typeStr = "class FName";
    } else if (Type == EPropertyTypes::FString) {
      typeStr = "class FString";
    } else if (Type == EPropertyTypes::FScriptDelegate) {
      typeStr = "struct FScriptDelegate";
    } else if (Type == EPropertyTypes::FStruct) {
      UStructProperty *structProperty =
          static_cast<UStructProperty *>(Property);

      if (structProperty && structProperty->Struct) {
        uint32_t propertyCount = GCache::CountObject<UScriptStruct>(
            structProperty->Struct->GetName());

        if (propertyCount > 1) {
          if (structProperty->Struct->Outer) {
            typeStr = ("struct " +
                       UnrealObject::CreateValidName(
                           structProperty->Struct->Outer->GetNameCPP()) +
                       "_" +
                       UnrealObject::CreateValidName(
                           structProperty->Struct->GetNameCPP()));
          } else {
            typeStr =
                ("struct " +
                 UnrealObject::CreateValidName(
                     structProperty->Struct->GetNameCPP()) +
                 "_Outer" + Printer::Decimal(propertyCount, EWidthTypes::Byte));
          }
        } else {
          typeStr = ("struct " + UnrealObject::CreateValidName(
                                     structProperty->Struct->GetNameCPP()));
        }
      }
    } else if (Type == EPropertyTypes::UClass) {
      UClassProperty *classProperty = static_cast<UClassProperty *>(Property);

      if (classProperty && classProperty->MetaClass) {
        typeStr = ("class " +
                   UnrealObject::CreateValidName(
                       classProperty->MetaClass->GetNameCPP()) +
                   "*");
      }
    } else if (Type == EPropertyTypes::UObject) {
      UObjectProperty *objectProperty =
          static_cast<UObjectProperty *>(Property);

      if (objectProperty && objectProperty->PropertyClass) {
        typeStr = ("class " +
                   UnrealObject::CreateValidName(
                       objectProperty->PropertyClass->GetNameCPP()) +
                   "*");
      }
    } else if (Type == EPropertyTypes::UInterface) {
      UInterfaceProperty *interfaceProperty =
          static_cast<UInterfaceProperty *>(Property);

      if (interfaceProperty && interfaceProperty->InterfaceClass) {
        typeStr = ("class " +
                   UnrealObject::CreateValidName(
                       interfaceProperty->InterfaceClass->GetNameCPP()) +
                   "*");
      }
    } else if (Type == EPropertyTypes::TArray) {
      UArrayProperty *arrayProperty = static_cast<UArrayProperty *>(Property);

      if (arrayProperty && arrayProperty->Inner) {
        UnrealProperty innerProperty(arrayProperty->Inner);

        if (innerProperty.IsValid()) {
          typeStr = ("class TArray<" +
                     innerProperty.GetType((bIgnoreEnum || IsReturnParameter()),
                                           false, true) +
                     ">");
        }
      }
    } else if (Type == EPropertyTypes::TMap) {
      UMapProperty *mapProperty = static_cast<UMapProperty *>(Property);

      if (mapProperty && mapProperty->Key && mapProperty->Value) {
        UnrealProperty keyProperty(mapProperty->Key);
        UnrealProperty valueProperty(mapProperty->Value);

        if (keyProperty.IsValid() && valueProperty.IsValid()) {
          typeStr = ("class TMap<" +
                     keyProperty.GetType((bIgnoreEnum || IsReturnParameter()),
                                         false, true) +
                     ", " +
                     valueProperty.GetType((bIgnoreEnum || IsReturnParameter()),
                                           false, true) +
                     ">");
        }
      }
    }
  }

  if (!typeStr.empty() && bFunctionParam && !bIgnoreConst && !CantReference() &&
      !CantConst()) {
    typeStr = ("const " + typeStr + "&");
  }

  return typeStr;
}

std::string UnrealProperty::GetTypeForClass() const {
  return GetType(false, false, false);
}

std::string UnrealProperty::GetTypeForStruct() const {
  return GetType(false, false, false);
}

std::string UnrealProperty::GetTypeForParameter(bool bIgnoreConst) const {
  return GetType(false, true, bIgnoreConst);
}

void UnrealProperty::Assign(class UProperty *uProperty) {
  if (uProperty) {
    Property = uProperty;
    ValidName = uProperty->GetName();
    AssignType();
    AssignName();
  }
}

void UnrealProperty::AssignType() {
  if (Property) {
    if (Property->IsA<UStructProperty>()) {
      UStructProperty *structProperty =
          static_cast<UStructProperty *>(Property);

      if (structProperty && structProperty->Struct) {
        Type = EPropertyTypes::FStruct;
      }
    } else if (Property->IsA<UStrProperty>()) {
      Type = EPropertyTypes::FString;
    } else if (Property->IsA<UQWordProperty>()) {
      Type = EPropertyTypes::UInt64;
    } else if (Property->IsA<UObjectProperty>()) {
      UObjectProperty *objectProperty =
          static_cast<UObjectProperty *>(Property);

      if (objectProperty && objectProperty->PropertyClass) {
        Type = EPropertyTypes::UObject;
      }
    } else if (Property->IsA<UClassProperty>()) {
      UClassProperty *classProperty = static_cast<UClassProperty *>(Property);

      if (classProperty && classProperty->MetaClass) {
        Type = EPropertyTypes::UClass;
      }
    } else if (Property->IsA<UInterfaceProperty>()) {
      UInterfaceProperty *interfaceProperty =
          static_cast<UInterfaceProperty *>(Property);

      if (interfaceProperty && interfaceProperty->InterfaceClass) {
        Type = EPropertyTypes::UInterface;
      }
    } else if (Property->IsA<UNameProperty>()) {
      Type = EPropertyTypes::FName;
    } else if (Property->IsA<UMapProperty>()) {
      UMapProperty *mapProperty = static_cast<UMapProperty *>(Property);
      std::string mapKey, mapValue;

      if (mapProperty && mapProperty->Key && mapProperty->Value) {
        if (UnrealProperty(mapProperty->Key).IsValid() &&
            UnrealProperty(mapProperty->Value).IsValid()) {
          Type = EPropertyTypes::TMap;
        }
      }
    } else if (Property->IsA<UIntProperty>()) {
      Type = EPropertyTypes::Int32;
    } else if (Property->IsA<UFloatProperty>()) {
      Type = EPropertyTypes::Float;
    } else if (Property->IsA<UDelegateProperty>()) {
      Type = EPropertyTypes::FScriptDelegate;
    } else if (Property->IsA<UByteProperty>()) {
      Type = EPropertyTypes::UInt8;
    } else if (Property->IsA<UBoolProperty>()) {
      Type = EPropertyTypes::Bool;
    } else if (Property->IsA<UArrayProperty>()) {
      UArrayProperty *arrayProperty = static_cast<UArrayProperty *>(Property);

      if (arrayProperty && arrayProperty->Inner) {
        UnrealProperty innerProperty(arrayProperty->Inner);

        if (innerProperty.IsValid()) {
          Type = EPropertyTypes::TArray;
        }
      }
    }
  }
}

void UnrealProperty::AssignName() { UnrealObject::ValidateName(ValidName); }

bool UnrealProperty::operator>(const UnrealProperty &unrealProp) {
  return (Hash() > unrealProp.Hash());
}

bool UnrealProperty::operator<(const UnrealProperty &unrealProp) {
  return (Hash() < unrealProp.Hash());
}

bool UnrealProperty::operator==(const UnrealProperty &unrealProp) {
  return (Hash() == unrealProp.Hash());
}

bool UnrealProperty::operator!=(const UnrealProperty &unrealProp) {
  return (Hash() != unrealProp.Hash());
}

UnrealProperty &UnrealProperty::operator=(const UnrealProperty &unrealProp) {
  Type = unrealProp.Type;
  Property = unrealProp.Property;
  ValidName = unrealProp.ValidName;
  return *this;
}

void GCache::Initialize() {
  // std::cout << "[Initialize] Starting\n";

  if (!m_packages.empty() && !Generator::AreGlobalsValid()) {
    // std::cout << "[Initialize] Skipping, packages not empty or globals
    // invalid\n";
    return;
  }

  auto &objects = *UObject::GObjObjects();
  const size_t totalObjects = objects.size();
  // std::cout << "[Initialize] Total objects: " << totalObjects << "\n";

  for (size_t i = 0; i < totalObjects; ++i) {
    UObject *uObject = objects[i];
    if (!uObject) {
      //   std::cout << "[Initialize] UObject at index " << i << " is null\n";
      continue;
    }

    UnrealObject unrealObj(uObject);
    if (!unrealObj.IsValid()) {
      // std::cout << "[Initialize] UnrealObject(" + unrealObj.FullName + ") at
      // index " << i << " is invalid\n";
      continue;
    }

    //  std::cout << "[Initialize] Caching object " << unrealObj.FullName <<
    //  "\n";
    CacheObject(unrealObj);
  }

  // std::cout << "[Initialize] Done\n";
}

void GCache::ClearCache() {
  m_objects.clear();
  m_consts.clear();
  m_enums.clear();
  m_structs.clear();
  m_classes.clear();
  m_constHashes.clear();
  m_enumHashes.clear();
  m_structHashes.clear();
  m_classHashes.clear();
  m_constants.clear();
  m_constantLookup.clear();
  m_packages.clear();
  m_cachedPackages.clear();
}

std::vector<UnrealObject> *GCache::GetCache(class UObject *packageObj,
                                            EClassTypes type) {
  if (type == EClassTypes::UConst) {
    return &m_consts[packageObj];
  } else if (type == EClassTypes::UEnum) {
    return &m_enums[packageObj];
  } else if (type == EClassTypes::UStruct) {
    return &m_structs[packageObj];
  } else if (type == EClassTypes::UClass) {
    return &m_classes[packageObj];
  }

  return nullptr;
}

std::unordered_map<class UObject *, std::unordered_set<class UObject *>> *
GCache::GetCacheHashes(EClassTypes type) {
  if (type == EClassTypes::UConst) {
    return &m_constHashes;
  } else if (type == EClassTypes::UEnum) {
    return &m_enumHashes;
  } else if (type == EClassTypes::UStruct) {
    return &m_structHashes;
  } else if (type == EClassTypes::UClass) {
    return &m_classHashes;
  }

  return nullptr;
}

std::map<std::string, class UObject *> *GCache::GetConstants() {
  return &m_constants;
}

std::vector<UnrealObject> *GCache::GetPackages() { return &m_packages; }

std::pair<std::string, class UObject *>
GCache::GetConstant(const UnrealObject &unrealObj) {
  if (GConfig::UsingConstants() && unrealObj.IsValid()) {
    auto lookupIt = m_constantLookup.find(unrealObj.Object);

    if (lookupIt != m_constantLookup.end()) {
      return lookupIt->second;
    }
  }

  return {"", nullptr};
}

UnrealObject GCache::GetClass(const std::string &classFullName) {
  for (auto &classPair : m_classes) {
    for (UnrealObject &unrealObj : classPair.second) {
      if (unrealObj.FullName == classFullName) {
        return unrealObj.Object;
      }
    }
  }

  return UnrealObject();
}

void GCache::CacheObject(UnrealObject &unrealObj) {
  if (unrealObj.IsValid()) {
    std::vector<UnrealObject> *cache =
        GetCache(unrealObj.Package, unrealObj.Type);
    std::unordered_map<class UObject *, std::unordered_set<class UObject *>>
        *cacheHashes = GetCacheHashes(unrealObj.Type);

    if (cache && cacheHashes) {
      std::unordered_set<class UObject *> &packageHashes =
          (*cacheHashes)[unrealObj.Package];

      if (packageHashes.insert(unrealObj.Object).second) {
        cache->push_back(unrealObj);
      }

      if (m_cachedPackages.insert(unrealObj.Package).second) {
        m_packages.emplace_back(unrealObj.Package, true);
      }
    }

    CacheConstant(unrealObj);
    CacheCount(unrealObj);
  }
}

void GCache::CacheConstant(UnrealObject &unrealObj) {
  if (GConfig::UsingConstants() && unrealObj.IsValid()) {
    if ((unrealObj.Type == EClassTypes::UClass) ||
        (unrealObj.Type == EClassTypes::UFunction)) {
      std::string constantName;
      constantName.reserve(unrealObj.FullName.length() + 4);
      constantName += "IDX_";

      for (unsigned char c : unrealObj.FullName) {
        if ((c == '.') || (c == ' ')) {
          constantName += '_';
        } else if ((c >= 'a') && (c <= 'z')) {
          constantName += static_cast<char>(c - ('a' - 'A'));
        } else {
          constantName += static_cast<char>(c);
        }
      }

      auto [constantIt, inserted] =
          m_constants.emplace(constantName, unrealObj.Object);

      if (inserted) {
        m_constantLookup[unrealObj.Object] = {constantIt->first,
                                              constantIt->second};
      }
    }
  }
}

void GCache::CacheCount(UnrealObject &unrealObj) {
  if (unrealObj.IsValid() && (unrealObj.Type == EClassTypes::UStruct)) {
    ObjectCountKey objectPair{unrealObj.ValidName, unrealObj.Object->Class};
    m_objects[objectPair]++;
  }
}

bool GLogger::Open() {
#ifndef NO_LOGGING
  if (!m_file.is_open() && GConfig::HasOutputPath()) {
    std::filesystem::path fullDirectory =
        (GConfig::GetOutputPath() / GConfig::GetGameNameShort());
    std::filesystem::create_directory(GConfig::GetOutputPath());
    std::filesystem::create_directory(fullDirectory);

    if (std::filesystem::exists(fullDirectory)) {
      m_file.open(fullDirectory / (GEngine::GetName() + ".log"));
      return true;
    } else {
      Utils::MessageboxError(
          "Error: Failed to create the log file, might not have the right "
          "permissions or your directory is invalid!");
    }
  }
#endif

  return false;
}

void GLogger::Close() {
#ifndef NO_LOGGING
  if (m_file.is_open()) {
    m_file.close();
  }
#endif
}

void GLogger::Flush() {
#ifndef NO_LOGGING
  if (m_file.is_open()) {
    m_file.flush();
  }
#endif
}

void GLogger::Log(const std::string &str, bool bFlush) {
#ifndef NO_LOGGING
  if (m_file.is_open() && !str.empty()) {
    m_file << str << "\n";

    if (bFlush) {
      Flush();
    }
  }
#endif
}

void GLogger::LogObject(const std::string &title,
                        const UnrealObject &unrealObj) {
#ifndef NO_LOGGING
  constexpr bool kVerboseObjectLogging = false;
  if (!kVerboseObjectLogging) {
    return;
  }

  if (m_file.is_open() && !title.empty() && unrealObj.IsValid()) {
    m_file << title;
    Printer::FillRight(m_file, ' ', unrealObj.ValidName.length());
    m_file << unrealObj.ValidName;

    if (unrealObj.ValidName.length() < LOG_FILE_SPACING) {
      Printer::FillRight(
          m_file, ' ',
          (LOG_FILE_SPACING - (unrealObj.ValidName.length() + title.length())));
    }

    m_file << " - Instance: " << Printer::Hex(unrealObj.Object) << "\n";
  }
#endif
}

void GLogger::LogClassSize(class UClass *uClass, size_t localSize) {
#ifndef NO_LOGGING
  if (m_file.is_open() && uClass) {
    m_file << "Error: Incorrect class size detected for \""
           << uClass->GetFullName() << "\"!\n";
    m_file << "Error: Reported size " << localSize << "\n";
    m_file << "Error: Actual size " << uClass->PropertySize << "\n";
  }
#endif
}

void GLogger::LogStructPadding(class UScriptStruct *uScriptStruct,
                               size_t padding) {
#ifndef NO_LOGGING
  if (m_file.is_open() && uScriptStruct) {
    m_file << "Info: Extra padding detected!\n";
    m_file << "Info: Property size "
           << Printer::Hex(uScriptStruct->PropertySize, 1) << "\n";
    m_file << "Info: Min alignment "
           << Printer::Hex(uScriptStruct->MinAlignment, 1) << "\n";
    m_file << "Info: Extra padding " << Printer::Hex(padding, 1) << "\n";
  }
#endif
}

namespace Utils {
void MessageboxExt(const std::string &message, uint32_t flags) {
#if defined(_WIN32) && !defined(NO_POPUPS)
  MessageBoxA(NULL, message.c_str(), GEngine::GetName().c_str(), flags);
#else
  (void)message;
  (void)flags;
#endif
}

void MessageboxInfo(const std::string &message) {
#ifdef _WIN32
  MessageboxExt(message, (MB_OK | MB_ICONINFORMATION));
#endif
}

void MessageboxWarn(const std::string &message) {
#ifdef _WIN32
  MessageboxExt(message, (MB_OK | MB_ICONWARNING));
#endif
}

void MessageboxError(const std::string &message) {
#ifdef _WIN32
  MessageboxExt(message, (MB_OK | MB_ICONERROR));
#endif
}

bool SortProperty(const UnrealProperty &unrealPropA,
                  const UnrealProperty &unrealPropB) {
  if (unrealPropA.Property && unrealPropB.Property) {
    if ((unrealPropA.Property->Offset == unrealPropB.Property->Offset) &&
        (unrealPropA.Type == EPropertyTypes::Bool) &&
        (unrealPropB.Type == EPropertyTypes::Bool)) {
      return (static_cast<UBoolProperty *>(unrealPropA.Property)->BitMask <
              static_cast<UBoolProperty *>(unrealPropB.Property)->BitMask);
    }

    return (unrealPropA.Property->Offset < unrealPropB.Property->Offset);
  }

  return false;
}

bool SortPropertyPair(const std::pair<UnrealProperty, std::string> &pairA,
                      const std::pair<UnrealProperty, std::string> &pairB) {
  return SortProperty(pairA.first, pairB.first);
}
} // namespace Utils

namespace Retrievers {
void GetAllFunctionFlags(std::ostringstream &stream, uint64_t functionFlags) {
  static thread_local std::unordered_map<uint64_t, std::string>
      m_functionFlagCache;
  auto cacheIt = m_functionFlagCache.find(functionFlags);

  if (cacheIt != m_functionFlagCache.end()) {
    stream << cacheIt->second;
    return;
  }

  std::string cachedFlags;
  cachedFlags.reserve(192);

  auto appendFlag = [&cachedFlags](bool enabled, const char *label) {
    if (enabled) {
      cachedFlags += (cachedFlags.empty() ? "(" : " | ");
      cachedFlags += label;
    }
  };

  appendFlag(functionFlags & EFunctionFlags::FUNC_None, "FUNC_None");
  appendFlag(functionFlags & EFunctionFlags::FUNC_Final, "FUNC_Final");
  appendFlag(functionFlags & EFunctionFlags::FUNC_Defined, "FUNC_Defined");
  appendFlag(functionFlags & EFunctionFlags::FUNC_Iterator, "FUNC_Iterator");
  appendFlag(functionFlags & EFunctionFlags::FUNC_Latent, "FUNC_Latent");
  appendFlag(functionFlags & EFunctionFlags::FUNC_PreOperator,
             "FUNC_PreOperator");
  appendFlag(functionFlags & EFunctionFlags::FUNC_Singular, "FUNC_Singular");
  appendFlag(functionFlags & EFunctionFlags::FUNC_Net, "FUNC_Net");
  appendFlag(functionFlags & EFunctionFlags::FUNC_NetReliable,
             "FUNC_NetReliable");
  appendFlag(functionFlags & EFunctionFlags::FUNC_Simulated, "FUNC_Simulated");
  appendFlag(functionFlags & EFunctionFlags::FUNC_Exec, "FUNC_Exec");
  appendFlag(functionFlags & EFunctionFlags::FUNC_Native, "FUNC_Native");
  appendFlag(functionFlags & EFunctionFlags::FUNC_Event, "FUNC_Event");
  appendFlag(functionFlags & EFunctionFlags::FUNC_Operator, "FUNC_Operator");
  appendFlag(functionFlags & EFunctionFlags::FUNC_Static, "FUNC_Static");
  appendFlag(functionFlags & EFunctionFlags::FUNC_NoExport, "FUNC_NoExport");
  appendFlag(functionFlags & EFunctionFlags::FUNC_OptionalParm,
             "FUNC_OptionalParm");
  appendFlag(functionFlags & EFunctionFlags::FUNC_Const, "FUNC_Const");
  appendFlag(functionFlags & EFunctionFlags::FUNC_Invariant, "FUNC_Invariant");
  appendFlag(functionFlags & EFunctionFlags::FUNC_Public, "FUNC_Public");
  appendFlag(functionFlags & EFunctionFlags::FUNC_Private, "FUNC_Private");
  appendFlag(functionFlags & EFunctionFlags::FUNC_Protected, "FUNC_Protected");
  appendFlag(functionFlags & EFunctionFlags::FUNC_Delegate, "FUNC_Delegate");
  appendFlag(functionFlags & EFunctionFlags::FUNC_NetServer, "FUNC_NetServer");
  appendFlag(functionFlags & EFunctionFlags::FUNC_HasOutParms,
             "FUNC_HasOutParms");
  appendFlag(functionFlags & EFunctionFlags::FUNC_HasDefaults,
             "FUNC_HasDefaults");
  appendFlag(functionFlags & EFunctionFlags::FUNC_NetClient, "FUNC_NetClient");
  appendFlag(functionFlags & EFunctionFlags::FUNC_DLLImport, "FUNC_DLLImport");
  appendFlag(functionFlags & EFunctionFlags::FUNC_K2Call, "FUNC_K2Call");
  appendFlag(functionFlags & EFunctionFlags::FUNC_K2Override,
             "FUNC_K2Override");
  appendFlag(functionFlags & EFunctionFlags::FUNC_K2Pure, "FUNC_K2Pure");
  appendFlag(functionFlags & EFunctionFlags::FUNC_EditorOnly,
             "FUNC_EditorOnly");
  appendFlag(functionFlags & EFunctionFlags::FUNC_Lambda, "FUNC_Lambda");
  appendFlag(functionFlags & EFunctionFlags::FUNC_NetValidate,
             "FUNC_NetValidate");
  appendFlag(functionFlags & EFunctionFlags::FUNC_AllFlags, "FUNC_AllFlags");

  if (!cachedFlags.empty()) {
    cachedFlags += ")";
  }

  m_functionFlagCache.emplace(functionFlags, cachedFlags);
  stream << cachedFlags;
}

void GetAllPropertyFlags(std::ostringstream &stream, uint64_t propertyFlags) {
  static thread_local std::unordered_map<uint64_t, std::string>
      m_propertyFlagCache;
  auto cacheIt = m_propertyFlagCache.find(propertyFlags);

  if (cacheIt != m_propertyFlagCache.end()) {
    stream << cacheIt->second;
    return;
  }

  std::string cachedFlags;
  cachedFlags.reserve(320);

  auto appendFlag = [&cachedFlags](bool enabled, const char *label) {
    if (enabled) {
      cachedFlags += (cachedFlags.empty() ? "(" : " | ");
      cachedFlags += label;
    }
  };

  auto appendDeprecated = [&cachedFlags](bool enabled) {
    if (enabled) {
      cachedFlags += (cachedFlags.empty() ? " " : " | ");
      cachedFlags += "CPF_Deprecated";
    }
  };

  appendFlag(propertyFlags & EPropertyFlags::CPF_Edit, "CPF_Edit");
  appendFlag(propertyFlags & EPropertyFlags::CPF_Const, "CPF_Const");
  appendFlag(propertyFlags & EPropertyFlags::CPF_Input, "CPF_Input");
  appendFlag(propertyFlags & EPropertyFlags::CPF_ExportObject,
             "CPF_ExportObject");
  appendFlag(propertyFlags & EPropertyFlags::CPF_OptionalParm,
             "CPF_OptionalParm");
  appendFlag(propertyFlags & EPropertyFlags::CPF_Net, "CPF_Net");
  appendFlag(propertyFlags & EPropertyFlags::CPF_EditFixedSize,
             "CPF_EditFixedSize");
  appendFlag(propertyFlags & EPropertyFlags::CPF_Parm, "CPF_Parm");
  appendFlag(propertyFlags & EPropertyFlags::CPF_OutParm, "CPF_OutParm");
  appendFlag(propertyFlags & EPropertyFlags::CPF_SkipParm, "CPF_SkipParm");
  appendFlag(propertyFlags & EPropertyFlags::CPF_ReturnParm, "CPF_ReturnParm");
  appendFlag(propertyFlags & EPropertyFlags::CPF_CoerceParm, "CPF_CoerceParm");
  appendFlag(propertyFlags & EPropertyFlags::CPF_Native, "CPF_Native");
  appendFlag(propertyFlags & EPropertyFlags::CPF_Transient, "CPF_Transient");
  appendFlag(propertyFlags & EPropertyFlags::CPF_Config, "CPF_Config");
  appendFlag(propertyFlags & EPropertyFlags::CPF_Localized, "CPF_Localized");
  appendFlag(propertyFlags & EPropertyFlags::CPF_Travel, "CPF_Travel");
  appendFlag(propertyFlags & EPropertyFlags::CPF_EditConst, "CPF_EditConst");
  appendFlag(propertyFlags & EPropertyFlags::CPF_GlobalConfig,
             "CPF_GlobalConfig");
  appendFlag(propertyFlags & EPropertyFlags::CPF_Component, "CPF_Component");
  appendFlag(propertyFlags & EPropertyFlags::CPF_NeedCtorLink,
             "CPF_NeedCtorLink");
  appendFlag(propertyFlags & EPropertyFlags::CPF_NoExport, "CPF_NoExport");
  appendFlag(propertyFlags & EPropertyFlags::CPF_NoClear, "CPF_NoClear");
  appendFlag(propertyFlags & EPropertyFlags::CPF_EditInline, "CPF_EditInline");
  appendFlag(propertyFlags & EPropertyFlags::CPF_EditInlineUse,
             "CPF_EditInlineUse");
  appendFlag(propertyFlags & EPropertyFlags::CPF_EditFindable,
             "CPF_EditFindable");
  appendFlag(propertyFlags & EPropertyFlags::CPF_EditInlineUse,
             "CPF_EditInlineUse");
  appendDeprecated(propertyFlags & EPropertyFlags::CPF_Deprecated);
  appendFlag(propertyFlags & EPropertyFlags::CPF_DataBinding,
             "CPF_DataBinding");
  appendFlag(propertyFlags & EPropertyFlags::CPF_SerializeText,
             "CPF_SerializeText");
  appendFlag(propertyFlags & EPropertyFlags::CPF_RepNotify, "CPF_RepNotify");
  appendFlag(propertyFlags & EPropertyFlags::CPF_Interp, "CPF_Interp");
  appendFlag(propertyFlags & EPropertyFlags::CPF_NonTransactional,
             "CPF_NonTransactional");
  appendFlag(propertyFlags & EPropertyFlags::CPF_EditorOnly, "CPF_EditorOnly");
  appendFlag(propertyFlags & EPropertyFlags::CPF_NotForConsole,
             "CPF_NotForConsole");
  appendFlag(propertyFlags & EPropertyFlags::CPF_RepRetry, "CPF_RepRetry");
  appendFlag(propertyFlags & EPropertyFlags::CPF_PrivateWrite,
             "CPF_PrivateWrite");
  appendFlag(propertyFlags & EPropertyFlags::CPF_ProtectedWrite,
             "CPF_ProtectedWrite");
  appendFlag(propertyFlags & EPropertyFlags::CPF_ArchetypeProperty,
             "CPF_ArchetypeProperty");
  appendFlag(propertyFlags & EPropertyFlags::CPF_EditHide, "CPF_EditHide");
  appendFlag(propertyFlags & EPropertyFlags::CPF_EditTextBox,
             "CPF_EditTextBox");
  appendFlag(propertyFlags & EPropertyFlags::CPF_CrossLevelPassive,
      "CPF_CrossLevelPassive");
  appendFlag(propertyFlags & EPropertyFlags::CPF_CrossLevelActive,
      "CPF_CrossLevelActive");

  if (!cachedFlags.empty()) {
    cachedFlags += ")";
  }

  m_propertyFlagCache.emplace(propertyFlags, cachedFlags);
  stream << cachedFlags;
}

uintptr_t GetBaseAddress() {
  return reinterpret_cast<uintptr_t>(GetModuleHandle(NULL));
}

uintptr_t GetOffset(void *pointer) {
  uintptr_t baseAddress = GetBaseAddress();
  uintptr_t address = reinterpret_cast<uintptr_t>(pointer);

  if (address > baseAddress) {
    return (address - baseAddress);
  }

  return NULL;
}

uintptr_t FindPattern(const uint8_t *pattern, const std::string &mask) {
  if (pattern && !mask.empty()) {
    MODULEINFO miInfos;
    ZeroMemory(&miInfos, sizeof(MODULEINFO));

    HMODULE hModule = GetModuleHandle(NULL);
    K32GetModuleInformation(GetCurrentProcess(), hModule, &miInfos,
                            sizeof(MODULEINFO));

    uintptr_t start = reinterpret_cast<uintptr_t>(hModule);
    uintptr_t end = (start + miInfos.SizeOfImage);

    size_t currentPos = 0;
    size_t maskLength = (mask.length() - 1);

    for (uintptr_t retAddress = start; retAddress < end; retAddress++) {
      if (*reinterpret_cast<uint8_t *>(retAddress) == pattern[currentPos] ||
          mask[currentPos] == '?') {
        if (currentPos == maskLength) {
          return (retAddress - maskLength);
        }

        currentPos++;
      } else {
        retAddress -= currentPos;
        currentPos = 0;
      }
    }
  }

  return NULL;
}
} // namespace Retrievers

namespace ConstGenerator {
static std::unordered_map<std::string, class UConst *> m_constNames;

void GenerateConstName(UnrealObject *unrealObj) {
  if (unrealObj) {
    if (unrealObj->ValidName.find("Default__") == std::string::npos) {
      std::string constName = unrealObj->ValidName;

      if (unrealObj->Object->Name.GetInstance()) {
        constName +=
            ("_" + std::to_string(unrealObj->Object->Name.GetInstance()));
      }

      if (constName.find("CONST_") != 0) {
        constName = ("CONST_" + constName);
      }

      if (m_constNames.contains(constName) &&
          (m_constNames[constName] != unrealObj->Object)) {
        uint32_t freeIndex = 0;

        for (uint32_t i = freeIndex; i < UINT32_MAX; i++) {
          std::string freeName = (constName + "_" + std::to_string(i));

          if (!m_constNames.contains(freeName)) {
            constName = freeName;
            break;
          }
        }
      }

      unrealObj->ValidName = constName;
      m_constNames[constName] = static_cast<UConst *>(unrealObj->Object);
    } else {
      unrealObj->ValidName.clear();
    }
  }
}

void GenerateConst(std::ofstream &stream, const UnrealObject &unrealObj) {
  if (unrealObj.IsValid()) {
#ifndef NO_LOGGING
    GLogger::LogObject("Const: ", unrealObj);
#endif

    if (!unrealObj.ValidName.empty()) {
      stream << "#define " << unrealObj.ValidName;

      if (unrealObj.ValidName.length() < GConfig::GetConstSpacing()) {
        Printer::FillLeft(
            stream, ' ',
            (GConfig::GetConstSpacing() - unrealObj.ValidName.length()));
      }

      stream << " " << static_cast<UConst *>(unrealObj.Object)->Value.ToString()
             << "\n";
    }
  }
}

void ProcessConsts(std::ofstream &stream, class UObject *packageObj) {
  if (packageObj) {
    std::vector<UnrealObject> *objCache =
        GCache::GetCache(packageObj, EClassTypes::UConst);

    if (objCache) {
      for (const UnrealObject &unrealObj : *objCache) {
        GenerateConst(stream, unrealObj);
      }
    }
  }
}
} // namespace ConstGenerator

namespace EnumGenerator {
static std::unordered_map<std::string, class UEnum *> m_enumNames;

void GenerateEnumName(UnrealObject *unrealObj) {
  if (unrealObj) {
    if (unrealObj->ValidName.find("Default__") == std::string::npos) {
      std::string enumName = unrealObj->ValidName;

      if (unrealObj->Object->Name.GetInstance()) {
        enumName +=
            ("_" + std::to_string(unrealObj->Object->Name.GetInstance()));
      }

      if (enumName.find("E") != 0) {
        enumName = ("E" + enumName);
      }

      if (m_enumNames.contains(enumName) &&
          (m_enumNames[enumName] != unrealObj->Object)) {
        uint32_t freeIndex = 0;

        for (uint32_t i = freeIndex; i < UINT32_MAX; i++) {
          std::string freeName = (enumName + "_" + std::to_string(i));

          if (!m_enumNames.contains(freeName)) {
            enumName = freeName;
            break;
          }
        }
      }

      unrealObj->ValidName = enumName;
      m_enumNames[enumName] = static_cast<UEnum *>(unrealObj->Object);
    } else {
      unrealObj->ValidName.clear();
    }
  }
}

void GenerateEnum(std::ofstream &file, const UnrealObject &unrealObj) {
  if (unrealObj.IsValid()) {
#ifndef NO_LOGGING
    GLogger::LogObject("Enum: ", unrealObj);
#endif

    std::ostringstream enumStream;
    std::ostringstream propertyStream;
    enumStream << "// " << unrealObj.FullName << "\n";

    if (GConfig::UsingEnumClasses()) {
      enumStream << "enum class " << unrealObj.ValidName << " : "
                 << GConfig::GetEnumClassType() << "\n";
      enumStream << "{" << "\n";
    } else {
      enumStream << "enum " << unrealObj.ValidName << "\n";
      enumStream << "{" << "\n";
    }

    UEnum *uEnum = static_cast<UEnum *>(unrealObj.Object);
    std::map<std::string, size_t> enumValues;

    for (int32_t i = 0; i < uEnum->Names.size(); i++) {
      std::string propertyName =
          UnrealObject::CreateValidName(uEnum->Names[i].ToString());
      size_t maxPos = propertyName.find("_MAX");

      if (maxPos != std::string::npos) {
        propertyName.replace(maxPos, 4, "_END");
      }

      if (!GConfig::UsingEnumClasses()) {
        propertyName = (unrealObj.ValidName + "_" + propertyName);
      }

      if (enumValues.count(propertyName) == 0) {
        enumValues[propertyName] = 1;
        propertyStream << propertyName;
      } else {
        propertyStream << propertyName
                       << Printer::Decimal(enumValues[propertyName],
                                           EWidthTypes::Byte);
        enumValues[propertyName]++;
      }

      enumStream << "\t";
      Printer::FillLeft(enumStream, ' ', GConfig::GetEnumSpacing());
      enumStream << propertyStream.str() << " = " << i;

      if (i != (uEnum->Names.size() - 1)) {
        enumStream << ",";
      }

      enumStream << "\n";
      Printer::Empty(propertyStream);
    }

    enumStream << "};\n\n";
    file << enumStream.str();
  }
}

void ProcessEnums(std::ofstream &stream, class UObject *packageObj) {
  if (packageObj) {
    std::vector<UnrealObject> *objCache =
        GCache::GetCache(packageObj, EClassTypes::UEnum);

    if (objCache) {
      for (const UnrealObject &unrealObj : *objCache) {
        GenerateEnum(stream, unrealObj);
      }
    }
  }
}
} // namespace EnumGenerator

namespace StructGenerator {
static std::unordered_set<std::string> m_generatedStructs;

void GenerateStructMembers(std::ofstream &structStream,
                           EClassTypes structType) {
  if (structType != EClassTypes::Unknown) {
    std::ostringstream propertyStream;

    size_t localSize = 0;
    size_t startOffset = 0;
    size_t missedOffset = 0;
    size_t lastOffset = 0;

    localSize = Member::GetClassSize(structType);
    startOffset = Member::GetClassOffset(structType);
    std::map<size_t, Member *> members = Member::GetRegistered(structType);

    if (!members.empty()) {
      lastOffset = startOffset;
      uint32_t unknownDataIndex = 0;

      for (const auto &memberPair : members) {
        if ((lastOffset + memberPair.second->Size) <
            memberPair.second->Offset) {
          missedOffset = (memberPair.second->Offset - lastOffset);

          if (missedOffset >= GConfig::GetGameAlignment()) {
            propertyStream << "\tuint8_t UnknownData"
                           << Printer::Decimal(unknownDataIndex,
                                               EWidthTypes::Byte);
            propertyStream << "[" << Printer::Hex(missedOffset) << "];";

            structStream << propertyStream.str() << "// "
                         << Printer::Hex(lastOffset, EWidthTypes::Size);
            structStream << " ("
                         << Printer::Hex(missedOffset, EWidthTypes::Size)
                         << ") DYNAMIC FIELD PADDING\n";
            Printer::Empty(propertyStream);

            unknownDataIndex++;
          }
        }

        structStream << "\t" << memberPair.second->Label << " // "
                     << Printer::Hex(memberPair.second->Offset,
                                     EWidthTypes::Size);
        structStream << " ("
                     << Printer::Hex(memberPair.second->Size, EWidthTypes::Size)
                     << ")\n";
        lastOffset = (memberPair.second->Offset + memberPair.second->Size);
      }

      if ((structType != EClassTypes::FNameEntry) && (lastOffset < localSize)) {
        missedOffset = (localSize - lastOffset);

        if (missedOffset >= GConfig::GetGameAlignment()) {
          propertyStream << "\tuint8_t UnknownData"
                         << Printer::Decimal(unknownDataIndex,
                                             EWidthTypes::Byte);
          propertyStream << "[" << Printer::Hex(missedOffset) << "];";

          structStream << propertyStream.str() << "// "
                       << Printer::Hex(lastOffset, EWidthTypes::Size);
          structStream << " (" << Printer::Hex(missedOffset, EWidthTypes::Size)
                       << ") DYNAMIC FIELD PADDING\n";
        }
      }
    } else {
#ifndef NO_LOGGING
      GLogger::Log("Error: No registered members found for struct type \"" +
                   Member::GetName(structType) + "\"!");
#endif
      Utils::MessageboxError(
          "Error: No registered members found for struct type \"" +
          Member::GetName(structType) + "\"!");
    }
  }
}

void GenerateStruct(std::ofstream &file, const UnrealObject &unrealObj) {
  if (unrealObj.IsValid()) {
    std::string structNameCPP =
        UnrealObject::CreateValidName(unrealObj.Object->GetNameCPP());
    std::string structOuterNameCPP =
        UnrealObject::CreateValidName(unrealObj.Object->Outer->GetNameCPP());

    if (GConfig::IsTypeBlacklisted(structNameCPP)) {
      return;
    }

#ifndef NO_LOGGING
    GLogger::LogObject("ScriptStruct: ", unrealObj);
#endif

    std::ostringstream structStream;
    std::ostringstream propertyStream;
    std::ostringstream flagStream;

    structStream << "// " << unrealObj.FullName << "\n";

    if (!GConfig::IsTypeOveridden(structNameCPP)) {
      size_t size = 0;
      size_t lastOffset = 0;
      size_t missedOffset = 0;

      UScriptStruct *scriptStruct =
          static_cast<UScriptStruct *>(unrealObj.Object);
      UScriptStruct *superField =
          static_cast<UScriptStruct *>(scriptStruct->SuperField);
      uint32_t structCount =
          GCache::CountObject<UScriptStruct>(unrealObj.ValidName);

      const std::vector<std::string> alignedStructs = {
          "BoneAtom",        "SHVector", "Vector4",
          "Matrix",          "Plane",    "Quat",
          "SkinMatrix3x4",   "BoneQuat", "EnsurePadding",
          "OctreeNodeBounds"};

      const auto isKnownAlignedStruct = [&alignedStructs](
                                            const std::string &cppName) {
        if (std::find(alignedStructs.begin(), alignedStructs.end(), cppName) !=
            alignedStructs.end()) {
          return true;
        }

        if ((cppName.length() > 1) && (cppName[0] == 'F')) {
          return (std::find(alignedStructs.begin(), alignedStructs.end(),
                            cppName.substr(1)) != alignedStructs.end());
        }

        return false;
      };

      const bool requiresExplicitAlignment = isKnownAlignedStruct(structNameCPP);

      if (superField && (superField != scriptStruct)) {
        size = (scriptStruct->PropertySize - superField->PropertySize);
        lastOffset = superField->PropertySize;

        std::string fieldName =
            UnrealObject::CreateValidName(superField->GetName());
        std::string fieldNameCPP =
            UnrealObject::CreateValidName(superField->GetNameCPP());
        uint32_t fieldStructCount =
            GCache::CountObject<UScriptStruct>(fieldName);

        structStream << "// " << Printer::Hex(size, EWidthTypes::Size);
        structStream << " ("
                     << Printer::Hex(superField->PropertySize,
                                     EWidthTypes::Size);
        structStream << " - "
                     << Printer::Hex(scriptStruct->PropertySize,
                                     EWidthTypes::Size);
        structStream << ")\n";

        if (structCount > 1) {
          structStream << "struct ";

          if (requiresExplicitAlignment) {
            structStream << "alignas(16) ";
          }

          structStream << structOuterNameCPP << "_"
                       << structNameCPP << " : ";
        } else {
          structStream << "struct ";

          if (requiresExplicitAlignment) {
            structStream << "alignas(16) ";
          }

          structStream << structNameCPP << " : ";
        }

        if ((fieldStructCount > 1) && superField->Outer) {
          structStream << UnrealObject::CreateValidName(
                              superField->Outer->GetNameCPP())
                       << "_" << fieldNameCPP << "\n";
        } else {
          structStream << fieldNameCPP << "\n";
        }
      } else {
        size = scriptStruct->PropertySize;
        structStream << "// " << Printer::Hex(size, EWidthTypes::Size) << "\n";

        if (structCount > 1) {
          structStream << "struct ";

          if (requiresExplicitAlignment) {
            structStream << "alignas(16) ";
          }

          structStream << structOuterNameCPP << "_" << structNameCPP << "\n";
        } else {
          structStream << "struct ";

          if (requiresExplicitAlignment) {
            structStream << "alignas(16) ";
          }

          structStream << structNameCPP << "\n";
        }
      }

      structStream << "{\n";

      std::vector<UnrealProperty> structProperties;
      int32_t effectiveAlignment = scriptStruct->MinAlignment;

      if (requiresExplicitAlignment) {
        effectiveAlignment = (std::max<int32_t>)(effectiveAlignment, 16);
      }

      for (UProperty *uProperty =
               static_cast<UProperty *>(scriptStruct->Children);
           uProperty; uProperty = static_cast<UProperty *>(uProperty->Next)) {
        if (uProperty && (uProperty->PropertySize > 0) &&
            !uProperty->IsA<UScriptStruct>()) {
          UnrealProperty unrealProp(uProperty);

          if (unrealProp.IsValid()) {
            structProperties.push_back(uProperty);
          }
        }
      }

      std::sort(structProperties.begin(), structProperties.end(),
                Utils::SortProperty);
      std::unordered_map<std::string, uint32_t> propertyNameMap;
      uint32_t unknownDataIndex = 0;

      for (const UnrealProperty &unrealProp : structProperties) {
        if (unrealProp.IsValid()) {
          if (unrealProp.Type == EPropertyTypes::FStruct) {
            UStructProperty *structProperty =
                static_cast<UStructProperty *>(unrealProp.Property);

            if (structProperty && structProperty->Struct) {
              int32_t memberAlignment = structProperty->Struct->MinAlignment;
              std::string memberStructName = UnrealObject::CreateValidName(
                  structProperty->Struct->GetNameCPP());

              if (isKnownAlignedStruct(memberStructName)) {
                memberAlignment = (std::max<int32_t>)(memberAlignment, 16);
              }

              effectiveAlignment =
                  (std::max)(effectiveAlignment, memberAlignment);
            }
          }

          if (lastOffset < unrealProp.Property->Offset) {
            missedOffset = (unrealProp.Property->Offset - lastOffset);

            if (missedOffset >= GConfig::GetGameAlignment()) {
              std::string missedStr = Printer::Hex(missedOffset);
              propertyStream
                  << "UnknownData"
                  << Printer::Decimal(unknownDataIndex, EWidthTypes::Byte);
              propertyStream << "[" << missedStr << "];";

              Printer::FillLeft(
                  structStream, ' ',
                  (GConfig::GetStructSpacing() + UNKNOWN_DATA_SPACING));
              structStream << "\tuint8_t";
              Printer::FillLeft(
                  structStream, ' ',
                  ((GConfig::GetStructSpacing() - missedStr.length()) +
                   UNKNOWN_DATA_SPACING));

              structStream << propertyStream.str() << "// "
                           << Printer::Hex(lastOffset, EWidthTypes::Size);
              structStream << " ("
                           << Printer::Hex(missedOffset, EWidthTypes::Size)
                           << ") MISSED OFFSET\n";
              Printer::Empty(propertyStream);

              unknownDataIndex++;
            }
          }

          size_t correctPropertySize = unrealProp.GetSize();

          if (propertyNameMap.count(unrealProp.ValidName) == 0) {
            propertyNameMap[unrealProp.ValidName] = 1;
            propertyStream << unrealProp.ValidName;
          } else {
            propertyStream << unrealProp.ValidName
                           << Printer::Decimal(
                                  propertyNameMap[unrealProp.ValidName],
                                  EWidthTypes::Byte);
            propertyNameMap[unrealProp.ValidName]++;
          }

          if (unrealProp.IsAnArray()) {
            if (unrealProp.Type != EPropertyTypes::UInterface) {
              propertyStream << "[" << unrealProp.Property->ArraySize << "]";
            }

            correctPropertySize *= unrealProp.Property->ArraySize;
          }

          if (unrealProp.Type == EPropertyTypes::Bool) {
            propertyStream << " : 1";
          }

          Retrievers::GetAllPropertyFlags(flagStream,
                                          unrealProp.Property->PropertyFlags);
          int32_t offsetError =
              ((unrealProp.Property->PropertySize *
                unrealProp.Property->ArraySize) -
               (correctPropertySize * unrealProp.Property->ArraySize));
          const std::string propertyText = propertyStream.str();

          if (unrealProp.Type == EPropertyTypes::UInterface) {
            size_t interfaceSize = unrealProp.GetSize();

            if (offsetError == interfaceSize) {
              offsetError -= interfaceSize;
            }

            structStream << "\t";
            Printer::FillLeft(structStream, ' ', GConfig::GetStructSpacing());
            structStream << unrealProp.GetTypeForStruct() << " "
                         << propertyText;

            if (unrealProp.IsAnArray()) {
              structStream << "_Object[" << unrealProp.Property->ArraySize
                           << "];";
            } else {
              structStream << "_Object;";
            }

            uint32_t propSpacing =
                static_cast<uint32_t>(propertyText.length() + 8);

            if (propSpacing < GConfig::GetStructSpacing()) {
              Printer::FillRight(structStream, ' ',
                                 (GConfig::GetStructSpacing() - propSpacing));
            }

            structStream << "// "
                         << Printer::Hex(unrealProp.Property->Offset,
                                         EWidthTypes::Size);
            structStream << " ("
                         << Printer::Hex((unrealProp.Property->PropertySize *
                                          unrealProp.Property->ArraySize),
                                         EWidthTypes::Size)
                         << ")";
            structStream << " ["
                         << Printer::Hex(unrealProp.Property->PropertyFlags,
                                         EWidthTypes::PropertyFlags)
                         << "] ";
            structStream << flagStream.str() << "\n";

            structStream << "\t";
            Printer::FillLeft(structStream, ' ', GConfig::GetStructSpacing());
            structStream << unrealProp.GetTypeForStruct() << " "
                         << propertyText;

            if (unrealProp.IsAnArray()) {
              structStream << "_Interface[" << unrealProp.Property->ArraySize
                           << "];";
            } else {
              structStream << "_Interface;";
            }

            uint32_t interfaceSpacing =
                static_cast<uint32_t>(propertyText.length() + 11);

            if (interfaceSpacing < GConfig::GetStructSpacing()) {
              Printer::FillRight(
                  structStream, ' ',
                  (GConfig::GetStructSpacing() - interfaceSpacing));
            }

            structStream << "// "
                         << Printer::Hex(unrealProp.Property->Offset,
                                         EWidthTypes::Size);
            structStream << " ("
                         << Printer::Hex((unrealProp.Property->PropertySize *
                                          unrealProp.Property->ArraySize),
                                         EWidthTypes::Size)
                         << ")";
            structStream << " ["
                         << Printer::Hex(unrealProp.Property->PropertyFlags,
                                         EWidthTypes::PropertyFlags)
                         << "] ";
            Printer::FillLeft(structStream, ' ',
                              static_cast<size_t>(EWidthTypes::FieldWidth));
          } else {
            structStream << "\t";
            Printer::FillLeft(structStream, ' ', GConfig::GetStructSpacing());
            structStream << unrealProp.GetTypeForStruct() << " " << propertyText
                         << ";";

            uint32_t propSpacing =
                static_cast<uint32_t>(propertyText.length() + 1);

            if (propSpacing < GConfig::GetStructSpacing()) {
              Printer::FillRight(structStream, ' ',
                                 (GConfig::GetStructSpacing() - propSpacing));
            }

            structStream << "// "
                         << Printer::Hex(unrealProp.Property->Offset,
                                         EWidthTypes::Size);
            structStream << " ("
                         << Printer::Hex((unrealProp.Property->PropertySize *
                                          unrealProp.Property->ArraySize),
                                         EWidthTypes::Size)
                         << ")";
            structStream << " ["
                         << Printer::Hex(unrealProp.Property->PropertyFlags,
                                         EWidthTypes::PropertyFlags)
                         << "] ";

            if (unrealProp.Type == EPropertyTypes::Bool) {
              structStream << "["
                           << Printer::Hex(static_cast<UBoolProperty *>(
                                               unrealProp.Property)
                                               ->BitMask,
                                           EWidthTypes::BitMask)
                           << "] ";
            } else {
              Printer::FillLeft(structStream, ' ',
                                static_cast<size_t>(EWidthTypes::FieldWidth));
            }
          }

          structStream << flagStream.str() << "\n";

          if (offsetError > 0) {
            std::string missedStr = Printer::Hex(offsetError);
            propertyStream << "UnknownData"
                           << Printer::Decimal(unknownDataIndex,
                                               EWidthTypes::Byte);
            propertyStream << "[" << missedStr << "];";

            Printer::FillLeft(
                structStream, ' ',
                (GConfig::GetStructSpacing() + UNKNOWN_DATA_SPACING));
            structStream << "\tuint8_t";
            Printer::FillLeft(
                structStream, ' ',
                ((GConfig::GetStructSpacing() - missedStr.length()) +
                 UNKNOWN_DATA_SPACING));

            structStream << propertyStream.str() << "// "
                         << Printer::Hex(unrealProp.Property->Offset +
                                             offsetError,
                                         EWidthTypes::Size);
            Printer::Empty(propertyStream);

            structStream << " (" << Printer::Hex(offsetError, EWidthTypes::Size)
                         << ") FIX WRONG SIZE OF PREVIOUS PROPERTY";
            structStream << " [Original: "
                         << Printer::Hex((unrealProp.Property->PropertySize *
                                          unrealProp.Property->ArraySize),
                                         EWidthTypes::Size);
            structStream << ", Missing: "
                         << Printer::Hex(offsetError, EWidthTypes::Size)
                         << "]\n";

            unknownDataIndex++;
          }

          Printer::Empty(propertyStream);
          Printer::Empty(flagStream);

          lastOffset = (unrealProp.Property->Offset +
                        (unrealProp.Property->PropertySize *
                         unrealProp.Property->ArraySize));
        } else if (unrealProp.Property) {
          std::string missedStr =
              Printer::Hex(unrealProp.Property->PropertySize *
                           unrealProp.Property->ArraySize);
          propertyStream << "UnknownData"
                         << Printer::Decimal(unknownDataIndex,
                                             EWidthTypes::Byte);
          propertyStream << "[" << missedStr << "];";

          Printer::FillLeft(
              structStream, ' ',
              (GConfig::GetStructSpacing() + UNKNOWN_DATA_SPACING));
          structStream << "\tuint8_t";
          Printer::FillLeft(
              structStream, ' ',
              ((GConfig::GetStructSpacing() - missedStr.length()) +
               UNKNOWN_DATA_SPACING));

          structStream << propertyStream.str() << "// "
                       << Printer::Hex(unrealProp.Property->Offset,
                                       EWidthTypes::Size);
          structStream << " ("
                       << Printer::Hex((unrealProp.Property->PropertySize *
                                        unrealProp.Property->ArraySize),
                                       EWidthTypes::Size);
          structStream << ") UNKNOWN PROPERTY: "
                       << unrealProp.Property->GetFullName() << "\n";
          Printer::Empty(propertyStream);

          lastOffset = (unrealProp.Property->Offset +
                        (unrealProp.Property->PropertySize *
                         unrealProp.Property->ArraySize));
          unknownDataIndex++;
        }
      }

      if (lastOffset < scriptStruct->PropertySize) {
        missedOffset = (scriptStruct->PropertySize - lastOffset);

        if (missedOffset >= GConfig::GetGameAlignment()) {
          std::string missedStr = Printer::Hex(missedOffset);
          propertyStream << "UnknownData"
                         << Printer::Decimal(unknownDataIndex,
                                             EWidthTypes::Byte);
          propertyStream << "[" << missedStr << "];";

          Printer::FillLeft(
              structStream, ' ',
              (GConfig::GetStructSpacing() + UNKNOWN_DATA_SPACING));
          structStream << "\tuint8_t";
          Printer::FillLeft(
              structStream, ' ',
              ((GConfig::GetStructSpacing() - missedStr.length()) +
               UNKNOWN_DATA_SPACING));

          structStream << propertyStream.str() << "// "
                       << Printer::Hex(lastOffset, EWidthTypes::Size);
          structStream << " (" << Printer::Hex(missedOffset, EWidthTypes::Size)
                       << ") MISSED OFFSET\n";
          Printer::Empty(propertyStream);
        }

        lastOffset += missedOffset;
      }

#ifndef SKIP_MIN_ALIGNMENT
      if (effectiveAlignment > 0) {
        int32_t actualSize = static_cast<int32_t>(lastOffset);
        int32_t remainder = (actualSize % effectiveAlignment);

        if (remainder != 0) {
          actualSize += (effectiveAlignment - remainder);
        }

        if (lastOffset < static_cast<size_t>(actualSize)) {
          int32_t padding = (actualSize - static_cast<int32_t>(lastOffset));
          std::string missedStr = Printer::Hex(padding);
          propertyStream << "MinStructAlignment" << "[" << missedStr << "];";

          Printer::FillLeft(
              structStream, ' ',
              (GConfig::GetStructSpacing() + UNKNOWN_DATA_SPACING));
          structStream << "\tuint8_t";
          Printer::FillLeft(
              structStream, ' ',
              ((GConfig::GetStructSpacing() - missedStr.length()) +
               UNKNOWN_DATA_SPACING));

          structStream << propertyStream.str() << "// "
                       << Printer::Hex(lastOffset, EWidthTypes::Size);
          structStream << " (" << Printer::Hex(padding, EWidthTypes::Size)
                       << ") ADDED PADDING\n";

#ifndef NO_LOGGING
          GLogger::LogStructPadding(scriptStruct, padding);
#endif
        }
      }
#endif
      std::string sName = (structCount > 1)
                              ? (structOuterNameCPP + "_" + structNameCPP)
                              : structNameCPP;

      structStream << "\n";
      structStream << "\tbool operator==(const " << sName
                   << "& other) const { return memcmp(this, &other, sizeof("
                   << sName << ")) == 0; }\n";
      structStream << "\tbool operator!=(const " << sName
                   << "& other) const { return !(*this == other); }\n";
      structStream << "\tbool operator<(const " << sName
                   << "& other) const { return memcmp(this, &other, sizeof("
                   << sName << ")) < 0; }\n";
      structStream << "};\n\n";
    } else {
      structStream << "// (Custom Override)\n";
      structStream << GConfig::GetTypeOverride(structNameCPP);
    }

    file << structStream.str();
  }
}

void GenerateStructPre(std::ofstream &stream, const UnrealObject &unrealObj) {
  if (unrealObj.IsValid() && !m_generatedStructs.contains(unrealObj.FullName)) {
    UScriptStruct *scriptStruct =
        static_cast<UScriptStruct *>(unrealObj.Object);

    if (scriptStruct->SuperField &&
        (scriptStruct->SuperField != scriptStruct)) {
      UScriptStruct *superStruct =
          static_cast<UScriptStruct *>(scriptStruct->SuperField);

      if (!m_generatedStructs.contains(superStruct->GetFullName())) {
        GenerateStructPre(stream, superStruct);
      }
    }

    for (UProperty *structChild =
             static_cast<UProperty *>(scriptStruct->Children);
         structChild;
         structChild = static_cast<UProperty *>(structChild->Next)) {
      UnrealProperty structProp(structChild);

      if (structProp.IsValid()) {
        if (structProp.Type == EPropertyTypes::FStruct) {
          UScriptStruct *propertyStruct = static_cast<UScriptStruct *>(
              static_cast<UStructProperty *>(structProp.Property)->Struct);

          if (propertyStruct && (propertyStruct != scriptStruct) &&
              !m_generatedStructs.contains(propertyStruct->GetFullName())) {
            GenerateStructPre(stream, propertyStruct);
          }
        } else if (structProp.Type == EPropertyTypes::TArray) {
          UScriptStruct *propertyStruct = static_cast<UScriptStruct *>(
              static_cast<UStructProperty *>(
                  static_cast<UArrayProperty *>(structProp.Property)->Inner)
                  ->Struct);

          if (propertyStruct && (propertyStruct != scriptStruct)) {
            UnrealProperty innerProp(
                static_cast<UArrayProperty *>(structProp.Property)->Inner);

            if (innerProp.IsValid() &&
                (innerProp.Type == EPropertyTypes::FStruct) &&
                !m_generatedStructs.contains(
                    innerProp.Property->GetFullName())) {
              GenerateStructPre(stream, propertyStruct);
            }
          }
        }
      }
    }

    GenerateStruct(stream, unrealObj);
    m_generatedStructs.insert(unrealObj.FullName);
  }
}

void ProcessStructs(std::ofstream &stream, class UObject *packageObj) {
  if (packageObj) {
    std::vector<UnrealObject> *objCache =
        GCache::GetCache(packageObj, EClassTypes::UStruct);

    if (objCache) {
      for (const UnrealObject &unrealObj : *objCache) {
        GenerateStructPre(stream, unrealObj);
      }
    }
  }
}
} // namespace StructGenerator

namespace ClassGenerator {
static std::unordered_set<std::string> m_generatedClasses;

void GenerateClassMembers(std::ostringstream &classStream, class UClass *uClass,
                          EClassTypes classType) {
  if (uClass && (classType != EClassTypes::Unknown)) {
    std::ostringstream propertyStream;

    size_t localSize = 0;
    size_t startOffset = 0;
    size_t missedOffset = 0;
    size_t lastOffset = 0;

    localSize = Member::GetClassSize(classType);
    startOffset = Member::GetClassOffset(classType);
    std::map<size_t, Member *> members = Member::GetRegistered(classType);

    if (!members.empty()) {
      if (uClass->PropertySize == localSize) {
        lastOffset = startOffset;
        uint32_t unknownDataIndex = 0;

        for (const auto &memberPair : members) {
          if ((lastOffset + memberPair.second->Size) <
              memberPair.second->Offset) {
            missedOffset = (memberPair.second->Offset - lastOffset);

            if (missedOffset >= GConfig::GetGameAlignment()) {
              propertyStream
                  << "\tuint8_t UnknownData"
                  << Printer::Decimal(unknownDataIndex, EWidthTypes::Byte);
              propertyStream << "[" << Printer::Hex(missedOffset) << "];";

              classStream << propertyStream.str() << "// "
                          << Printer::Hex(lastOffset, EWidthTypes::Size);
              classStream << " ("
                          << Printer::Hex(missedOffset, EWidthTypes::Size)
                          << ") DYNAMIC FIELD PADDING\n";
              Printer::Empty(propertyStream);

              unknownDataIndex++;
            }
          }

          classStream << "\t" << memberPair.second->Label << " // "
                      << Printer::Hex(memberPair.second->Offset,
                                      EWidthTypes::Size);
          classStream << " ("
                      << Printer::Hex(memberPair.second->Size,
                                      EWidthTypes::Size)
                      << ")\n";

          lastOffset = (memberPair.second->Offset + memberPair.second->Size);
        }

        if (lastOffset < uClass->PropertySize) {
          missedOffset = (uClass->PropertySize - lastOffset);

          if (missedOffset >= GConfig::GetGameAlignment()) {
            propertyStream << "\tuint8_t UnknownData"
                           << Printer::Decimal(unknownDataIndex,
                                               EWidthTypes::Byte);
            propertyStream << "[" << Printer::Hex(missedOffset) << "];";

            classStream << propertyStream.str() << "// "
                        << Printer::Hex(lastOffset, EWidthTypes::Size);
            classStream << " (" << Printer::Hex(missedOffset, EWidthTypes::Size)
                        << ") DYNAMIC FIELD PADDING\n";
          }
        }
      } else {
#ifndef NO_LOGGING
        GLogger::LogClassSize(uClass, localSize);
#endif
        Utils::MessageboxError("Error: Incorrect class size detected for \"" +
                               Member::GetName(classType) +
                               "\", check the log file for more details!");
      }
    } else {
#ifndef NO_LOGGING
      GLogger::Log("Error: No registered members found for class \"" +
                   uClass->GetName() + "\"!");
#endif
      Utils::MessageboxError("Error: No registered members found for \"" +
                             Member::GetName(classType) + "\"!");
    }
  }
}

void GenerateClass(std::ofstream &file, const UnrealObject &unrealObj) {
  if (unrealObj.IsValid()) {
    UClass *uClass = static_cast<UClass *>(unrealObj.Object);
    UClass *uSuperClass = static_cast<UClass *>(uClass->SuperField);
    std::string classNameCPP =
        UnrealObject::CreateValidName(uClass->GetNameCPP());

    if (GConfig::IsTypeBlacklisted(classNameCPP)) {
      return;
    }

#ifndef NO_LOGGING
    GLogger::LogObject("Class: ", unrealObj);
#endif

    std::ostringstream classStream;
    std::ostringstream propertyStream;
    std::ostringstream flagStream;
    const std::string classConstant =
        (GConfig::UsingConstants() ? GCache::GetConstant(unrealObj).first
                                   : std::string());

    size_t size = 0;
    size_t lastOffset = 0;
    size_t missedOffset = 0;

    classStream << "// " << unrealObj.FullName << "\n";

    if (!GConfig::IsTypeOveridden(classNameCPP)) {
      std::vector<UnrealProperty> classProperties;

      for (UProperty *uProperty = static_cast<UProperty *>(uClass->Children);
           uProperty; uProperty = static_cast<UProperty *>(uProperty->Next)) {
        if (uProperty && (uProperty->PropertySize > 0) &&
            !uProperty->IsA<UFunction>() && !uProperty->IsA<UConst>() &&
            !uProperty->IsA<UEnum>() && !uProperty->IsA<UScriptStruct>() &&
            (!uSuperClass ||
             (uSuperClass && (uSuperClass != uClass) &&
              (uProperty->Offset >= uSuperClass->PropertySize)))) {
          UnrealProperty unrealProp(uProperty);

          if (unrealProp.IsValid()) {
            classProperties.push_back(unrealProp);
          }
        }
      }

      std::sort(classProperties.begin(), classProperties.end(),
                Utils::SortProperty);

      if (uSuperClass && (uSuperClass != uClass)) {
        size = (uClass->PropertySize - uSuperClass->PropertySize);
        lastOffset = uSuperClass->PropertySize;

        classStream << "// " << Printer::Hex(size, EWidthTypes::Size);
        classStream << " ("
                    << Printer::Hex(uSuperClass->PropertySize,
                                    EWidthTypes::Size);
        classStream << " - "
                    << Printer::Hex(uClass->PropertySize, EWidthTypes::Size);
        classStream << ")\n"
                    << "class " << classNameCPP << " : public "
                    << UnrealObject::CreateValidName(uSuperClass->GetNameCPP());
      } else {
        size = uClass->PropertySize;

        classStream << "// " << Printer::Hex(size, EWidthTypes::Size) << "\n";
        classStream << "class " << classNameCPP;
      }

      classStream << "\n{\npublic:\n";

      if (uClass == UField::StaticClass()) {
        GenerateClassMembers(classStream, uClass, EClassTypes::UField);
      } else if (uClass == UEnum::StaticClass()) {
        GenerateClassMembers(classStream, uClass, EClassTypes::UEnum);
      } else if (uClass == UConst::StaticClass()) {
        GenerateClassMembers(classStream, uClass, EClassTypes::UConst);
      } else if (uClass == UProperty::StaticClass()) {
        GenerateClassMembers(classStream, uClass, EClassTypes::UProperty);
      } else if (uClass == UStruct::StaticClass()) {
        GenerateClassMembers(classStream, uClass, EClassTypes::UStruct);
      } else if (uClass == UFunction::StaticClass()) {
        GenerateClassMembers(classStream, uClass, EClassTypes::UFunction);
      } else if (uClass == UDelegateProperty::StaticClass()) {
        GenerateClassMembers(classStream, uClass,
                             EClassTypes::UDelegateProperty);
      } else if (uClass == UClass::StaticClass()) {
        GenerateClassMembers(classStream, uClass, EClassTypes::UClass);
      } else if (uClass == UStructProperty::StaticClass()) {
        GenerateClassMembers(classStream, uClass, EClassTypes::UStructProperty);
      } else if (uClass == UClassProperty::StaticClass()) {
        GenerateClassMembers(classStream, uClass, EClassTypes::UClassProperty);
      } else if (uClass == UObjectProperty::StaticClass()) {
        GenerateClassMembers(classStream, uClass, EClassTypes::UObjectProperty);
      } else if (uClass == UMapProperty::StaticClass()) {
        GenerateClassMembers(classStream, uClass, EClassTypes::UMapProperty);
      } else if (uClass == UInterfaceProperty::StaticClass()) {
        GenerateClassMembers(classStream, uClass,
                             EClassTypes::UInterfaceProperty);
      } else if (uClass == UByteProperty::StaticClass()) {
        GenerateClassMembers(classStream, uClass, EClassTypes::UByteProperty);
      } else if (uClass == UBoolProperty::StaticClass()) {
        GenerateClassMembers(classStream, uClass, EClassTypes::UBoolProperty);
      } else if (uClass == UArrayProperty::StaticClass()) {
        GenerateClassMembers(classStream, uClass, EClassTypes::UArrayProperty);
      } else {
        std::unordered_map<std::string, uint32_t> propertyNameMap;
        uint32_t unknownDataIndex = 0;

        for (const UnrealProperty &unrealProp : classProperties) {
          if (unrealProp.IsValid()) {
            if (uClass == UObject::StaticClass()) {
              if (!GConfig::UsingCallFunctionIndex() &&
                  (unrealProp.ValidName.find("VfTable") != std::string::npos)) {
                lastOffset = (unrealProp.Property->Offset +
                              (unrealProp.Property->PropertySize *
                               unrealProp.Property->ArraySize));
                continue;
              }
            }

            if (lastOffset < unrealProp.Property->Offset) {
              missedOffset = (unrealProp.Property->Offset - lastOffset);

              if (missedOffset >= GConfig::GetGameAlignment()) {
                std::string missedStr = Printer::Hex(missedOffset);
                propertyStream
                    << "UnknownData"
                    << Printer::Decimal(unknownDataIndex, EWidthTypes::Byte);
                propertyStream << "[" << missedStr << "];";

                Printer::FillLeft(
                    classStream, ' ',
                    (GConfig::GetClassSpacing() + UNKNOWN_DATA_SPACING));
                classStream << "\tuint8_t";
                Printer::FillLeft(
                    classStream, ' ',
                    ((GConfig::GetClassSpacing() - missedStr.length()) +
                     UNKNOWN_DATA_SPACING));

                classStream << propertyStream.str() << "// "
                            << Printer::Hex(lastOffset, EWidthTypes::Size);
                classStream << " ("
                            << Printer::Hex(missedOffset, EWidthTypes::Size)
                            << ") MISSED OFFSET\n";
                Printer::Empty(propertyStream);

                unknownDataIndex++;
              }
            }

            size_t correctPropertySize = unrealProp.GetSize();

            if (propertyNameMap.count(unrealProp.ValidName) == 0) {
              propertyNameMap[unrealProp.ValidName] = 1;
              propertyStream << unrealProp.ValidName;
            } else {
              propertyStream
                  << unrealProp.ValidName
                  << Printer::Decimal(propertyNameMap[unrealProp.ValidName],
                                      EWidthTypes::Byte);
              propertyNameMap[unrealProp.ValidName]++;
            }

            if (unrealProp.IsAnArray()) {
              if (unrealProp.Type != EPropertyTypes::UInterface) {
                propertyStream << "[" << unrealProp.Property->ArraySize << "]";
              }

              correctPropertySize *= unrealProp.Property->ArraySize;
            }

            if (unrealProp.Type == EPropertyTypes::Bool) {
              propertyStream << " : 1";
            }

            Retrievers::GetAllPropertyFlags(flagStream,
                                            unrealProp.Property->PropertyFlags);
            int32_t offsetError =
                ((unrealProp.Property->PropertySize *
                  unrealProp.Property->ArraySize) -
                 (correctPropertySize * unrealProp.Property->ArraySize));
            const std::string propertyText = propertyStream.str();

            if (unrealProp.Type == EPropertyTypes::UInterface) {
              size_t interfaceSize = unrealProp.GetSize();

              if (offsetError == interfaceSize) {
                offsetError -= interfaceSize;
              }

              classStream << "\t";
              Printer::FillLeft(classStream, ' ', GConfig::GetClassSpacing());
              classStream << unrealProp.GetTypeForClass() << " "
                          << propertyText;

              if (unrealProp.IsAnArray()) {
                classStream << "_Object[" << unrealProp.Property->ArraySize
                            << "];";
                correctPropertySize *= unrealProp.Property->ArraySize;
              } else {
                classStream << "_Object;";
              }

              uint32_t propSpacing =
                  static_cast<uint32_t>(propertyText.length() + 8);

              if (propSpacing < GConfig::GetClassSpacing()) {
                Printer::FillRight(classStream, ' ',
                                   (GConfig::GetClassSpacing() - propSpacing));
              }

              classStream << "// "
                          << Printer::Hex(unrealProp.Property->Offset,
                                          EWidthTypes::Size);
              classStream << " ("
                          << Printer::Hex(((unrealProp.Property->PropertySize *
                                            unrealProp.Property->ArraySize) -
                                           interfaceSize),
                                          EWidthTypes::Size)
                          << ")";
              classStream << " ["
                          << Printer::Hex(unrealProp.Property->PropertyFlags,
                                          EWidthTypes::PropertyFlags)
                          << "] ";
              classStream << flagStream.str() << "\n";

              classStream << "\t";
              Printer::FillLeft(classStream, ' ', GConfig::GetClassSpacing());
              classStream << unrealProp.GetTypeForClass() << " "
                          << propertyText;

              if (unrealProp.IsAnArray()) {
                classStream << "_Interface[" << unrealProp.Property->ArraySize
                            << "];";
                correctPropertySize *= unrealProp.Property->ArraySize;
              } else {
                classStream << "_Interface;";
              }

              uint32_t interfaceSpacing =
                  static_cast<uint32_t>(propertyText.length() + 11);

              if (interfaceSpacing < GConfig::GetClassSpacing()) {
                Printer::FillRight(
                    classStream, ' ',
                    (GConfig::GetClassSpacing() - interfaceSpacing));
              }

              classStream << "// "
                          << Printer::Hex(unrealProp.Property->Offset +
                                              interfaceSize,
                                          EWidthTypes::Size);
              classStream << " ("
                          << Printer::Hex(((unrealProp.Property->PropertySize *
                                            unrealProp.Property->ArraySize) -
                                           interfaceSize),
                                          EWidthTypes::Size)
                          << ")";
              classStream << " ["
                          << Printer::Hex(unrealProp.Property->PropertyFlags,
                                          EWidthTypes::PropertyFlags)
                          << "] ";
              Printer::FillLeft(classStream, ' ',
                                static_cast<size_t>(EWidthTypes::FieldWidth));
            } else {
              classStream << "\t";
              Printer::FillLeft(classStream, ' ', GConfig::GetClassSpacing());
              classStream << unrealProp.GetTypeForClass() << " " << propertyText
                          << ";";

              uint32_t propSpacing =
                  static_cast<uint32_t>(propertyText.length() + 1);

              if (propSpacing < GConfig::GetClassSpacing()) {
                Printer::FillRight(classStream, ' ',
                                   (GConfig::GetClassSpacing() - propSpacing));
              }

              classStream << "// "
                          << Printer::Hex(unrealProp.Property->Offset,
                                          EWidthTypes::Size);
              classStream << " ("
                          << Printer::Hex((unrealProp.Property->PropertySize *
                                           unrealProp.Property->ArraySize),
                                          EWidthTypes::Size)
                          << ")";
              classStream << " ["
                          << Printer::Hex(unrealProp.Property->PropertyFlags,
                                          EWidthTypes::PropertyFlags)
                          << "] ";

              if (unrealProp.Type == EPropertyTypes::Bool) {
                classStream << "["
                            << Printer::Hex(static_cast<UBoolProperty *>(
                                                unrealProp.Property)
                                                ->BitMask,
                                            EWidthTypes::BitMask)
                            << "] ";
              } else {
                Printer::FillLeft(classStream, ' ',
                                  static_cast<size_t>(EWidthTypes::FieldWidth));
              }
            }

            classStream << flagStream.str() << "\n";

            if (offsetError > 0) {
              std::string missedStr = Printer::Hex(offsetError);
              propertyStream
                  << "_UnknownData"
                  << Printer::Decimal(unknownDataIndex, EWidthTypes::Byte);
              propertyStream << "[" << missedStr << "];";

              Printer::FillLeft(
                  classStream, ' ',
                  (GConfig::GetClassSpacing() + UNKNOWN_DATA_SPACING));
              classStream << "\tuint8_t";
              Printer::FillLeft(
                  classStream, ' ',
                  ((GConfig::GetClassSpacing() - missedStr.length()) +
                   UNKNOWN_DATA_SPACING));

              classStream << propertyStream.str() << "// "
                          << Printer::Hex(
                                 (unrealProp.Property->Offset + offsetError),
                                 EWidthTypes::Size);
              classStream << " ("
                          << Printer::Hex(offsetError, EWidthTypes::Size)
                          << ") FIX WRONG SIZE OF PREVIOUS PROPERTY ";
              classStream << " [Original:"
                          << Printer::Hex((unrealProp.Property->PropertySize *
                                           unrealProp.Property->ArraySize),
                                          EWidthTypes::Size);
              classStream << ", Missing: "
                          << Printer::Hex(offsetError, EWidthTypes::Size)
                          << "]\n";
              Printer::Empty(propertyStream);

              unknownDataIndex++;
            }

            Printer::Empty(propertyStream);
            Printer::Empty(flagStream);

            lastOffset = (unrealProp.Property->Offset +
                          (unrealProp.Property->PropertySize *
                           unrealProp.Property->ArraySize));
          } else if (unrealProp.Property) {
            std::string missedStr =
                Printer::Hex(unrealProp.Property->PropertySize *
                             unrealProp.Property->ArraySize);
            propertyStream << "UnknownData"
                           << Printer::Decimal(unknownDataIndex,
                                               EWidthTypes::Byte);
            propertyStream << "[" << missedStr << "];";

            Printer::FillLeft(classStream, ' ', GConfig::GetClassSpacing());
            classStream << "\tuint8_t";
            Printer::FillLeft(
                classStream, ' ',
                ((GConfig::GetClassSpacing() - missedStr.length()) +
                 UNKNOWN_DATA_SPACING));

            classStream << propertyStream.str() << "// "
                        << Printer::Hex(unrealProp.Property->Offset,
                                        EWidthTypes::Size);
            classStream << " ("
                        << Printer::Hex((unrealProp.Property->PropertySize *
                                         unrealProp.Property->ArraySize),
                                        EWidthTypes::Size);
            classStream << ") UNKNOWN PROPERTY: "
                        << unrealProp.Property->GetFullName() << "\n";
            Printer::Empty(propertyStream);

            lastOffset = (unrealProp.Property->Offset +
                          (unrealProp.Property->PropertySize *
                           unrealProp.Property->ArraySize));
            unknownDataIndex++;
          }
        }

        if (lastOffset < uClass->PropertySize) {
          missedOffset = (uClass->PropertySize - lastOffset);

          if (missedOffset >= GConfig::GetGameAlignment()) {
            std::string missedStr = Printer::Hex(missedOffset);
            propertyStream << "UnknownData"
                           << Printer::Decimal(unknownDataIndex,
                                               EWidthTypes::Byte);
            propertyStream << "[" << missedStr << "];";

            Printer::FillLeft(
                classStream, ' ',
                (GConfig::GetClassSpacing() + UNKNOWN_DATA_SPACING));
            classStream << "\tuint8_t";
            Printer::FillLeft(
                classStream, ' ',
                ((GConfig::GetClassSpacing() - missedStr.length()) +
                 UNKNOWN_DATA_SPACING));

            classStream << propertyStream.str() << "// "
                        << Printer::Hex(lastOffset, EWidthTypes::Size);
            classStream << " (" << Printer::Hex(missedOffset, EWidthTypes::Size)
                        << ") MISSED OFFSET\n";
            Printer::Empty(propertyStream);
          }
        }
      }

      classStream << "\npublic:\n";
      classStream << "\tstatic UClass* StaticClass()\n";
      classStream << "\t{\n";
      classStream << "\t\tstatic UClass* uClassPointer = nullptr;\n\n";
      classStream << "\t\tif (!uClassPointer)\n";
      classStream << "\t\t{\n";

      if (GConfig::UsingConstants()) {
        classStream << "\t\t\tuClassPointer = "
                       "reinterpret_cast<UClass*>(UObject::GObjObjects()->at("
                    << classConstant << "));\n";
      } else {
        classStream << "\t\t\tuClassPointer = UObject::FindClass(\""
                    << unrealObj.FullName << "\");\n";
      }

      classStream << "\t\t}\n\n";
      classStream << "\t\treturn uClassPointer;\n";
      classStream << "\t};\n\n";

      if (uClass == UObject::StaticClass()) {
        classStream << PiecesOfCode::UObject_FunctionDescriptions;
      }

      file << classStream.str();
      Printer::Empty(classStream);

      FunctionGenerator::GenerateFunctionParameters(file, unrealObj);

      if (uClass == UObject::StaticClass()) {
        if (GConfig::UsingCallFunctionIndex() &&
            GConfig::UsingProcessEventIndex()) {
          classStream << "\tvoid CallFunction(struct FFrame& Stack, "
                         "RESULT_DECL, class UFunction* function);\n";
          classStream << "\tvoid ProcessEvent(class UFunction* uFunction, "
                         "void* uParams, void* uResult = {});\n";

        } else if (GConfig::GetCallFunctionIndex() != -1) {
          FunctionGenerator::GenerateVirtualFunctions(file);
        }
      } else if (uClass == UFunction::StaticClass()) {
        classStream << "\tstatic UFunction* FindFunction(const std::string& "
                       "functionFullName);\n";
      }

      classStream << "};\n\n";
    } else {
      classStream << "// (Custom Override)\n";
      classStream << GConfig::GetTypeOverride(classNameCPP);
    }

    file << classStream.str();
  }
}

void GenerateClassPre(std::ofstream &file, const UnrealObject &unrealObj,
                      class UObject *packageObj) {
  if (unrealObj.IsValid() && !m_generatedClasses.contains(unrealObj.FullName)) {
    if (unrealObj.Package != packageObj) {
      return;
    }

    UClass *uClass = static_cast<UClass *>(unrealObj.Object);

    if (uClass->SuperField && (uClass->SuperField != uClass)) {
      if (!m_generatedClasses.contains(uClass->SuperField->GetFullName())) {
        GenerateClassPre(file, static_cast<UClass *>(uClass->SuperField),
                         packageObj);
      }
    }

    GenerateClass(file, uClass);
    m_generatedClasses.insert(unrealObj.FullName);
  }
}

void ProcessClasses(std::ofstream &stream, class UObject *packageObj) {
  if (packageObj) {
    std::vector<UnrealObject> *objCache =
        GCache::GetCache(packageObj, EClassTypes::UClass);

    if (objCache) {
      for (const UnrealObject &unrealObj : *objCache) {
        GenerateClassPre(stream, unrealObj, packageObj);

        if (unrealObj.FullName == "Class Core.SubscriptionCollection_ORS") {
          UnrealObject subObj =
              GCache::GetClass("Class Engine.ScriptGroup_ORS");

          if (subObj.IsValid()) {
            GenerateClassPre(stream, subObj, subObj.Package);
          }
        } else if (unrealObj.FullName == "Class Core.Field") {
          UnrealObject subObj = GCache::GetClass("Class Core.Enum");

          if (subObj.IsValid()) {
            GenerateClassPre(stream, subObj, subObj.Package);
          }
        } else if (unrealObj.FullName == "Class Core.Function") {
          UnrealObject subObj = GCache::GetClass("Class Core.DelegateProperty");

          if (subObj.IsValid()) {
            GenerateClassPre(stream, subObj, subObj.Package);
          }
        }
      }
    }
  }
}
} // namespace ClassGenerator

namespace ParameterGenerator {
void GenerateParameter(std::ofstream &file, const UnrealObject &unrealObj) {
  std::ostringstream parameterStream;
  std::ostringstream propertyStream;
  std::ostringstream flagStream;

  UClass *uClass = static_cast<UClass *>(unrealObj.Object);
  std::vector<UnrealObject> classFunctions;

  for (UProperty *uProperty = static_cast<UProperty *>(uClass->Children);
       uProperty; uProperty = static_cast<UProperty *>(uProperty->Next)) {
    if (uProperty && uProperty->IsA<UFunction>()) {
      classFunctions.push_back(UnrealObject(uProperty));
    }
  }

  std::string classNameCPP =
      UnrealObject::CreateValidName(uClass->GetNameCPP());

  for (UnrealObject &functionObj : classFunctions) {
    if (functionObj.IsValid()) {
      UFunction *uFunction = static_cast<UFunction *>(functionObj.Object);
      parameterStream << "// " << functionObj.FullName << "\n";
      parameterStream << "// ["
                      << Printer::Hex(uFunction->FunctionFlags,
                                      EWidthTypes::FunctionFlags)
                      << "] ";

      if ((uFunction->FunctionFlags & EFunctionFlags::FUNC_Native) &&
          uFunction->iNative) {
        parameterStream << " (iNative[" << uFunction->iNative << "])";
      }

      if (uFunction->FunctionFlags & EFunctionFlags::FUNC_Exec) {
        propertyStream << "exec";
      } else if (uFunction->FunctionFlags & EFunctionFlags::FUNC_Event) {
        propertyStream << "event";
      } else {
        propertyStream << "exec";
      }

      parameterStream << "\nstruct " << classNameCPP << "_"
                      << propertyStream.str() << functionObj.ValidName
                      << "_Params\n"
                      << "{\n";
      Printer::Empty(propertyStream);

      std::vector<UnrealProperty> funcProperties;

      for (UProperty *uProperty = static_cast<UProperty *>(uFunction->Children);
           uProperty; uProperty = static_cast<UProperty *>(uProperty->Next)) {
        if (uProperty && (uProperty->PropertySize > 0)) {
          funcProperties.push_back(UnrealProperty(uProperty));
        }
      }

      std::sort(funcProperties.begin(), funcProperties.end(),
                Utils::SortProperty);
      std::unordered_map<std::string, uint32_t> propertyNameMap;

      for (UnrealProperty &unrealProp : funcProperties) {
        if (unrealProp.IsValid()) {
          std::string propertyType = unrealProp.GetTypeForStruct();

          if (propertyNameMap.count(unrealProp.ValidName) == 0) {
            propertyNameMap[unrealProp.ValidName] = 1;
            propertyStream << unrealProp.ValidName;
          } else {
            propertyStream << unrealProp.ValidName
                           << Printer::Decimal(
                                  propertyNameMap[unrealProp.ValidName],
                                  EWidthTypes::Byte);
            propertyNameMap[unrealProp.ValidName]++;
          }

          if (unrealProp.Property->ArraySize > 1) {
            propertyStream << "[" << unrealProp.Property->ArraySize << "]";
          }

#ifdef ROCKET_LEAGUE
          if ((unrealProp.Type == EPropertyTypes::Bool) &&
              !unrealProp.IsThickBool()) {
            propertyStream << " : 1";
          }
#else
          if (unrealProp.Type == EPropertyTypes::Bool) {
            propertyStream << " : 1";
          }
#endif

          propertyStream << ";";
          Retrievers::GetAllPropertyFlags(flagStream,
                                          unrealProp.Property->PropertyFlags);

          parameterStream << "\t";
          int32_t spacingSub = 0;

          if (!unrealProp.IsParameter()) // Properties that arent parameters are
                                         // usually used in some way in the
                                         // function related to unreal script.
          {
            parameterStream << "// ";
            spacingSub = 3;
          }

          Printer::FillLeft(parameterStream, ' ',
                            (GConfig::GetFunctionSpacing() - spacingSub));
          parameterStream << propertyType << " ";
          Printer::FillLeft(parameterStream, ' ',
                            GConfig::GetFunctionSpacing());

          parameterStream << propertyStream.str() << "// "
                          << Printer::Hex(unrealProp.Property->Offset,
                                          EWidthTypes::Size);
          parameterStream << " ("
                          << Printer::Hex((unrealProp.Property->PropertySize *
                                           unrealProp.Property->ArraySize),
                                          EWidthTypes::Size)
                          << ")";
          parameterStream << " ["
                          << Printer::Hex(unrealProp.Property->PropertyFlags,
                                          EWidthTypes::PropertyFlags)
                          << "] ";

          if (unrealProp.Type == EPropertyTypes::Bool) {
            parameterStream << "["
                            << Printer::Hex(static_cast<UBoolProperty *>(
                                                unrealProp.Property)
                                                ->BitMask,
                                            EWidthTypes::BitMask)
                            << "] ";
          } else {
            Printer::FillLeft(parameterStream, ' ',
                              static_cast<size_t>(EWidthTypes::FieldWidth));
          }

          parameterStream << flagStream.str() << "\n";
          Printer::Empty(propertyStream);
          Printer::Empty(flagStream);
        } else if (unrealProp.Property) {
          parameterStream << "\t// UNKNOWN PROPERTY: "
                          << unrealProp.Property->GetFullName() << "\n";
        }
      }

      parameterStream << "};\n\n";
    }
  }

  file << parameterStream.str();
}

void ProcessParameters(std::ofstream &stream, UObject *packageObj) {
  if (packageObj) {
    std::vector<UnrealObject> *objCache =
        GCache::GetCache(packageObj, EClassTypes::UClass);

    if (objCache) {
      for (const UnrealObject &unrealObj : *objCache) {
        GenerateParameter(stream, unrealObj);
      }
    }
  }
}
} // namespace ParameterGenerator

namespace FunctionGenerator {
namespace {
void WriteFunctionParameter(std::ostream &stream,
                            const UnrealProperty &property,
                            const std::string &name, bool isOutParameter,
                            bool ignoreConst = false) {
  if (isOutParameter && property.IsAnArray()) {
    stream << property.GetTypeForParameter(ignoreConst) << " (&" << name
           << ")[" << property.Property->ArraySize << "]";
    return;
  }

  stream << property.GetTypeForParameter(ignoreConst);

  if (isOutParameter) {
    stream << "& " << name;
    return;
  }

  stream << " " << name;

  if (property.IsAnArray()) {
    stream << "[" << property.Property->ArraySize << "]";
  }
}
} // namespace

void GenerateVirtualFunctions(std::ofstream &stream) {
  uintptr_t processEventAddress = 0;

  if (!GConfig::UsingProcessEventIndex()) {
    processEventAddress = Retrievers::FindPattern(
        GConfig::GetProcessEventPattern(), GConfig::GetProcessEventMask());
  } else if (GConfig::GetCallFunctionIndex() != -1) {
    processEventAddress = reinterpret_cast<uintptr_t *>(
        UObject::StaticClass()
            ->VfTableObject.Dummy)[GConfig::GetProcessEventIndex()];
  } else {
    stream << "\n\t// FIX PROCESSEVENT IN CONFIGURATION.CPP, INVALID INDEX";
    Utils::MessageboxWarn(
        "Warning: ProcessEvent is not configured correctly in "
        "\"Configuration.cpp\", you set \"UsingIndex\" to true yet you did not "
        "provide a valid index for process event!");
  }

  if (processEventAddress) {
    stream << "\n\t// Virtual Functions\n\n";

    for (uint32_t index = 0; index < 1024;
         index++) // 1024 is just for infinite loop protection.
    {
      uintptr_t virtualFunction = reinterpret_cast<uintptr_t *>(
          UObject::StaticClass()->VfTableObject.Dummy)[index];

      if (virtualFunction == processEventAddress) {
        stream << "\tvirtual void ProcessEvent(class UFunction* uFunction, "
                  "void* uParams, void* uResult);";
        stream << "\t// "
               << Printer::Hex(virtualFunction, sizeof(virtualFunction))
               << "\n";
        break;
      } else {
        stream << "\tvirtual void VirtualFunction"
               << Printer::Decimal(index, EWidthTypes::Byte) << "();";
        stream << "\t// "
               << Printer::Hex(virtualFunction, sizeof(virtualFunction))
               << "\n";
      }
    }
  } else {
    stream << "\n\t// FIX PROCESSEVENT IN CONFIGURATION.CPP, INVALID ADDRESS";
    Utils::MessageboxWarn(
        "Warning: ProcessEvent is not configured correctly in "
        "\"Configuration.cpp\", failed to find a valid address!");
  }
}

void GenerateFunctionCode(std::ofstream &stream,
                          const UnrealObject &unrealObj) {
  if (unrealObj.IsValid()) {
    UClass *uClass = static_cast<UClass *>(unrealObj.Object);
    std::string classNameCPP =
        UnrealObject::CreateValidName(uClass->GetNameCPP());
    std::ostringstream codeStream;
    std::ostringstream functionStream;
    const std::string classConstant =
        (GConfig::UsingConstants() ? GCache::GetConstant(unrealObj).first
                                   : std::string());

    if (uClass == UObject::StaticClass()) {
      codeStream << PiecesOfCode::UObject_Functions;

      if (GConfig::UsingCallFunctionIndex()) {
        codeStream << "template<typename T> T GetVirtualFunction(const void* "
                      "instance, size_t index)\n";
        codeStream << "{\n";
        codeStream << "\tauto vtable = *static_cast<const "
                      "void***>(const_cast<void*>(instance));\n";
        codeStream << "\treturn reinterpret_cast<T>(vtable[index]);\n";
        codeStream << "}\n\n";

        codeStream << "void UObject::CallFunction( FFrame& Stack, RESULT_DECL, "
                      "UFunction* Function )\n";
        codeStream << "{\n";
        codeStream << "\tGetVirtualFunction<void(*)(class UObject*, FFrame&, "
                      "RESULT_DECL, UFunction*)>(this, "
                   << GConfig::GetCallFunctionIndex()
                   << ")(this, Stack, Result, Function);\n";
        codeStream << "}\n\n";
      }

      if (GConfig::UsingProcessEventIndex()) {
        codeStream << "void UObject::ProcessEvent(class UFunction* uFunction, "
                      "void* uParams, void* uResult)\n";
        codeStream << "{\n";
        codeStream << "\tGetVirtualFunction<void(*)(class UObject*, class "
                      "UFunction*, void*)>(this, "
                   << GConfig::GetProcessEventIndex()
                   << ")(this, uFunction, uParams);\n";
        codeStream << "}\n\n";
      }
    }

    if (uClass == UFunction::StaticClass()) {
      codeStream << PiecesOfCode::UFunction_Functions;
    }

    std::vector<UnrealObject> classFunctions;

    for (UField *uField = uClass->Children; uField; uField = uField->Next) {
      if (uField && uField->IsA<UFunction>()) {
        classFunctions.push_back(UnrealObject(uField));
      }
    }

    for (UnrealObject &functionObj : classFunctions) {
      if (functionObj.IsValid()) {
        UFunction *uFunction = static_cast<UFunction *>(functionObj.Object);
        Retrievers::GetAllFunctionFlags(functionStream,
                                        uFunction->FunctionFlags);
        codeStream << "// " << functionObj.FullName << "\n";
        codeStream << "// ["
                   << Printer::Hex(uFunction->FunctionFlags,
                                   EWidthTypes::FunctionFlags)
                   << "] " << functionStream.str();
        Printer::Empty(functionStream);

        if ((uFunction->FunctionFlags & EFunctionFlags::FUNC_Native) &&
            uFunction->iNative) {
          codeStream << " (iNative[" << uFunction->iNative << "])";
        }

        std::vector<std::pair<UnrealProperty, std::string>> propertyParams;
        std::vector<std::pair<UnrealProperty, std::string>> propertyOutParams;
        std::vector<std::pair<UnrealProperty, std::string>> propertyBothParams;
        std::pair<UnrealProperty, std::string> returnParam;
        std::unordered_map<std::string, uint32_t> propertyNameMap;

        for (UProperty *uProperty =
                 static_cast<UProperty *>(uFunction->Children);
             uProperty; uProperty = static_cast<UProperty *>(uProperty->Next)) {
          UnrealProperty unrealProp(uProperty);

          if (unrealProp.IsValid()) {
            std::string propertyNameUnique;

            if (propertyNameMap.count(unrealProp.ValidName) == 0) {
              propertyNameMap[unrealProp.ValidName] = 1;
              propertyNameUnique = unrealProp.ValidName;
            } else {
              functionStream
                  << unrealProp.ValidName
                  << Printer::Decimal(propertyNameMap[unrealProp.ValidName],
                                      EWidthTypes::Byte);
              propertyNameUnique = functionStream.str();
              Printer::Empty(functionStream);
              propertyNameMap[unrealProp.ValidName]++;
            }

            if (unrealProp.IsReturnParameter()) {
              returnParam = {uProperty, propertyNameUnique};
            } else if (unrealProp.IsParameter()) {
              if (unrealProp.IsOutParameter()) {
                propertyOutParams.push_back({unrealProp, propertyNameUnique});
                propertyBothParams.push_back({unrealProp, propertyNameUnique});
              } else {
                propertyParams.push_back({unrealProp, propertyNameUnique});
              }
            }
          }
        }

        std::sort(propertyParams.begin(), propertyParams.end(),
                  Utils::SortPropertyPair);
        std::sort(propertyOutParams.begin(), propertyOutParams.end(),
                  Utils::SortPropertyPair);
        std::sort(propertyBothParams.begin(), propertyBothParams.end(),
                  Utils::SortPropertyPair);

        codeStream << "\n// Parameter Info:\n";

        if (returnParam.first.IsValid()) {
          Retrievers::GetAllPropertyFlags(
              functionStream, returnParam.first.Property->PropertyFlags);
          codeStream << "// ";
          Printer::FillLeft(codeStream, ' ', GConfig::GetCommentSpacing());
          codeStream << returnParam.first.GetTypeForParameter() << " ";
          Printer::FillLeft(codeStream, ' ', GConfig::GetCommentSpacing());
          codeStream << returnParam.second << " " << functionStream.str()
                     << "\n";
          Printer::Empty(functionStream);
        }

        for (const auto &propertyPair : propertyParams) {
          if (propertyPair.first.IsValid()) {
            Retrievers::GetAllPropertyFlags(
                functionStream, propertyPair.first.Property->PropertyFlags);
            codeStream << "// ";
            Printer::FillLeft(codeStream, ' ', GConfig::GetCommentSpacing());
            codeStream << propertyPair.first.GetTypeForClass() << " ";
            Printer::FillLeft(codeStream, ' ', GConfig::GetCommentSpacing());
            codeStream << propertyPair.second << " " << functionStream.str()
                       << "\n";
            Printer::Empty(functionStream);
          }
        }

        for (const auto &propertyPair : propertyOutParams) {
          if (propertyPair.first.IsValid()) {
            Retrievers::GetAllPropertyFlags(
                functionStream, propertyPair.first.Property->PropertyFlags);
            codeStream << "// ";
            Printer::FillLeft(codeStream, ' ', GConfig::GetCommentSpacing());
            codeStream << propertyPair.first.GetTypeForClass() << " ";
            Printer::FillLeft(codeStream, ' ', GConfig::GetCommentSpacing());
            codeStream << propertyPair.second << " " << functionStream.str()
                       << "\n";
            Printer::Empty(functionStream);
          }
        }

        if (returnParam.first.IsValid()) {
          codeStream << "\n" << returnParam.first.GetTypeForClass();
        } else {
          codeStream << "\nvoid";
        }

        if (uFunction->FunctionFlags & EFunctionFlags::FUNC_Exec) {
          codeStream << " " << classNameCPP << "::" << functionObj.ValidName
                     << "(";
        } else if (uFunction->FunctionFlags & EFunctionFlags::FUNC_Event) {
          codeStream << " " << classNameCPP << "::event"
                     << functionObj.ValidName << "(";
        } else {
          codeStream << " " << classNameCPP << "::" << functionObj.ValidName
                     << "(";
        }

        bool printComma = false;

        for (const auto &propertyPair : propertyParams) {
          if (propertyPair.first.IsValid()) {
            if (printComma) {
              codeStream << ", ";
            }

            WriteFunctionParameter(codeStream, propertyPair.first,
                                   propertyPair.second, false);

            printComma = true;
          }
        }

        for (const auto &propertyPair : propertyOutParams) {
          if (propertyPair.first.IsValid()) {
            if (printComma) {
              codeStream << ", ";
            }

            WriteFunctionParameter(codeStream, propertyPair.first,
                                   propertyPair.second, true, true);
            printComma = true;
          }
        }

        codeStream << ")\n";
        codeStream << "{\n";
        codeStream << "\tstatic UFunction* uFn" << functionObj.ValidName
                   << " = nullptr;\n\n";
        codeStream << "\tif (!uFn" << functionObj.ValidName << ")\n";
        codeStream << "\t{\n";

        if (GConfig::UsingConstants()) {
          codeStream
              << "\t\tuFn" << functionObj.ValidName
              << " = reinterpret_cast<UFunction*>(UObject::GObjObjects()->at("
              << classConstant << "));\n";
        } else {
          codeStream << "\t\tuFn" << functionObj.ValidName
                     << " = UFunction::FindFunction(\"" << functionObj.FullName
                     << "\");\n";
        }

        codeStream << "\t}\n\n";
        codeStream << "\t" << classNameCPP << "_";

        if (uFunction->FunctionFlags & EFunctionFlags::FUNC_Exec) {
          codeStream << "exec";
        } else if (uFunction->FunctionFlags & EFunctionFlags::FUNC_Event) {
          codeStream << "event";
        } else {
          codeStream << "exec";
        }

        codeStream << functionObj.ValidName << "_Params "
                   << functionObj.ValidName << "_Params;\n";
        codeStream << "\tmemset(&" << functionObj.ValidName
                   << "_Params, 0, sizeof(" << functionObj.ValidName
                   << "_Params));\n";

        for (const auto &propertyPair : propertyParams) {
          if (propertyPair.first.IsValid()) {
            if (!propertyPair.first.CantMemcpy()) {
              codeStream << "\tmemcpy_s(&" << functionObj.ValidName
                         << "_Params." << propertyPair.second << ", sizeof("
                         << functionObj.ValidName << "_Params."
                         << propertyPair.second << ")";

              // if (propertyPair.first.IsAnArray())
              //{
              //     codeStream << " * " <<
              //     propertyPair.first.Property->ArraySize;
              // }

              codeStream << ", &" << propertyPair.second << ", sizeof("
                         << propertyPair.second << ")";

              // if (propertyPair.first.IsAnArray())
              //{
              //     codeStream << " * " <<
              //     propertyPair.first.Property->ArraySize;
              // }

              codeStream << ");\n";
            } else if (!propertyPair.first.IsContainer()) {
              codeStream << "\t" << functionObj.ValidName << "_Params."
                         << propertyPair.second << " = " << propertyPair.second
                         << ";\n";
            }
          }
        }

        for (const auto &propertyPair : propertyBothParams) {
          if (propertyPair.first.IsValid()) {
            if (!propertyPair.first.CantMemcpy()) {
              codeStream << "\tmemcpy_s(&" << functionObj.ValidName
                         << "_Params." << propertyPair.second << ", sizeof("
                         << functionObj.ValidName << "_Params."
                         << propertyPair.second << ")";

              // if (propertyPair.first.IsAnArray())
              //{
              //       codeStream << " * " <<
              //       propertyPair.first.Property->ArraySize;
              // }

              codeStream << ", &" << propertyPair.second << ", sizeof("
                         << propertyPair.second << ")";

              // if (propertyPair.first.IsAnArray())
              //{
              //       codeStream << " * " <<
              //       propertyPair.first.Property->ArraySize;
              // }

              codeStream << ");\n";
            } else if (!propertyPair.first.IsContainer()) {
              codeStream << "\t" << functionObj.ValidName << "_Params."
                         << propertyPair.second << " = " << propertyPair.second
                         << ";\n";
            }
          }
        }

        bool hasNativeIndex = (uFunction->iNative ? true : false);
        bool hasNativeFlags =
            (uFunction->FunctionFlags & EFunctionFlags::FUNC_Native);
        bool isStatic =
            (uFunction->FunctionFlags & EFunctionFlags::FUNC_Static);

        if (hasNativeFlags && hasNativeIndex && GConfig::RemoveNativeIndex()) {
          codeStream << "\n\tuFn" << functionObj.ValidName << "->iNative = 0;";
        }

        if (GConfig::RemoveNativeFlags() && hasNativeFlags) {
          codeStream << "\n\tuFn" << functionObj.ValidName
                     << "->FunctionFlags &= ~"
                     << Printer::Hex(EFunctionFlags::FUNC_Native) << ";";
        }

        if ((uFunction->FunctionFlags & EFunctionFlags::FUNC_Static) &&
            (uFunction->FunctionFlags != EFunctionFlags::FUNC_AllFlags)) {
          codeStream << "\n\t" << classNameCPP
                     << "::StaticClass()->ProcessEvent(" << "uFn"
                     << functionObj.ValidName << ", &" << functionObj.ValidName
                     << "_Params, nullptr);\n";
        } else {
          codeStream << "\n\tthis->ProcessEvent(uFn" << functionObj.ValidName
                     << ", &" << functionObj.ValidName
                     << "_Params, nullptr);\n";
        }

        if (hasNativeFlags && GConfig::RemoveNativeFlags()) {
          codeStream << "\tuFn" << functionObj.ValidName
                     << "->FunctionFlags |= "
                     << Printer::Hex(EFunctionFlags::FUNC_Native) << ";\n";
        }

        if (hasNativeFlags && hasNativeIndex && GConfig::RemoveNativeIndex()) {
          codeStream << "\tuFn" << functionObj.ValidName
                     << "->iNative = " << uFunction->iNative << ";\n";
        }

        if (!propertyOutParams.empty()) {
          codeStream << "\n";

          for (const auto &propertyPair : propertyOutParams) {
            if (propertyPair.first.IsValid()) {
              if (!propertyPair.first.CantMemcpy()) {
                codeStream << "\tmemcpy_s(&" << propertyPair.second
                           << ", sizeof(" << propertyPair.second << ")";

                // if (propertyPair.first.IsAnArray())
                //{
                //      codeStream << " * " <<
                //      propertyPair.first.Property->ArraySize;
                // }

                codeStream << ", &" << functionObj.ValidName << "_Params."
                           << propertyPair.second << ", sizeof("
                           << functionObj.ValidName << "_Params."
                           << propertyPair.second << ")";

                // if (propertyPair.first.IsAnArray())
                //{
                //     codeStream << " * " <<
                //     propertyPair.first.Property->ArraySize;
                // }

                codeStream << ");\n";
              } else if (!propertyPair.first.IsContainer()) {
                codeStream << "\t" << propertyPair.second << " = "
                           << functionObj.ValidName << "_Params."
                           << propertyPair.second << ";\n";
              }
            }
          }
        }

        if (returnParam.first.IsValid()) {
          codeStream << "\n\treturn ";

          if (GConfig::UsingEnumClasses() &&
              (returnParam.first.Type == EPropertyTypes::UInt8)) {
            std::string returnType = returnParam.first.GetTypeForClass();

            if (returnType != "uint8_t") {
              codeStream << "static_cast<" << returnType << ">("
                         << functionObj.ValidName << "_Params."
                         << returnParam.second << ");\n";
            } else {
              codeStream << functionObj.ValidName << "_Params."
                         << returnParam.second << ";\n";
            }
          } else {
            codeStream << functionObj.ValidName << "_Params."
                       << returnParam.second << ";\n";
          }
        }

        codeStream << "};\n\n";
      }
    }

    stream << codeStream.str();
  }
}

void GenerateFunctionParameters(std::ofstream &stream,
                                const UnrealObject &unrealObj) {
  if (unrealObj.IsValid()) {
    UClass *uClass = static_cast<UClass *>(unrealObj.Object);
    std::ostringstream functionStream;
    std::ostringstream propertyStream;
    std::vector<UnrealObject> classFunctions;

    for (UField *uField = uClass->Children; uField; uField = uField->Next) {
      if (uField && uField->IsA<UFunction>()) {
        classFunctions.push_back(UnrealObject(uField));
      }
    }

    for (UnrealObject &functionObj : classFunctions) {
      if (functionObj.IsValid()) {
        UFunction *uFunction = static_cast<UFunction *>(functionObj.Object);
        std::vector<std::pair<UnrealProperty, std::string>> funcParams;
        std::vector<std::pair<UnrealProperty, std::string>> outParams;
        std::pair<UnrealProperty, std::string> returnParam;
        std::unordered_map<std::string, uint32_t> propertyNameMap;

        for (UProperty *uProperty =
                 static_cast<UProperty *>(uFunction->Children);
             uProperty; uProperty = static_cast<UProperty *>(uProperty->Next)) {
          UnrealProperty unrealProp(uProperty);

          if (unrealProp.IsValid()) {
            std::string propertyNameUnique;

            if (propertyNameMap.count(unrealProp.ValidName) == 0) {
              propertyNameMap[unrealProp.ValidName] = 1;
              propertyNameUnique = unrealProp.ValidName;
            } else {
              propertyStream
                  << unrealProp.ValidName
                  << Printer::Decimal(propertyNameMap[unrealProp.ValidName],
                                      EWidthTypes::Byte);
              propertyNameUnique = propertyStream.str();
              Printer::Empty(propertyStream);
              propertyNameMap[unrealProp.ValidName]++;
            }

            if (unrealProp.IsReturnParameter()) {
              returnParam = {unrealProp, propertyNameUnique};
            } else if (unrealProp.IsOutParameter()) {
              propertyNameUnique[0] = std::toupper(propertyNameUnique[0]);
              outParams.push_back({unrealProp, ("out" + propertyNameUnique)});
            } else if (unrealProp.IsParameter()) {
              if (unrealProp.IsOptionalParameter()) {
                propertyNameUnique[0] = std::toupper(propertyNameUnique[0]);
                funcParams.push_back(
                    {unrealProp, ("optional" + propertyNameUnique)});
              } else {
                funcParams.push_back({unrealProp, propertyNameUnique});
              }
            }
          }
        }

        std::sort(funcParams.begin(), funcParams.end(),
                  Utils::SortPropertyPair);
        std::sort(outParams.begin(), outParams.end(), Utils::SortPropertyPair);

        functionStream << "\t";

        bool isStaticFunc =
            (uFunction->FunctionFlags & EFunctionFlags::FUNC_Static) &&
            (uFunction->FunctionFlags != EFunctionFlags::FUNC_AllFlags);

        if (isStaticFunc) {
          functionStream << "static ";
        }

        if (returnParam.first.IsValid()) {
          functionStream << returnParam.first.GetTypeForParameter();
        } else {
          functionStream << "void";
        }

        if (uFunction->FunctionFlags & EFunctionFlags::FUNC_Exec) {
          functionStream << " " << functionObj.ValidName << "(";
        } else if (uFunction->FunctionFlags & EFunctionFlags::FUNC_Event) {
          functionStream << " event" << functionObj.ValidName << "(";
        } else {
          functionStream << " " << functionObj.ValidName << "(";
        }

        bool printComma = false;

        // Find the start of the trailing optional-only suffix.
        // Only params from this index onward (all of which are optional) can
        // safely receive a " = {}" default without breaking C++ syntax.
        // Requirements for = {} to be valid:
        //   1. No out-params exist - they always come after funcParams with no
        //      default, which would make any earlier default invalid (C2548).
        //   2. The param is not a struct type - structs may be forward-declared
        //      only in the class header (cross-package), causing C2440.
        size_t defaultSuffixStart = funcParams.size();
        if (outParams.empty()) {
          for (size_t i = funcParams.size(); i-- > 0;) {
            if (!funcParams[i].first.IsOptionalParameter() ||
                funcParams[i].first.Type == EPropertyTypes::FStruct) {
              break;
            }
            defaultSuffixStart = i;
          }
        }

        for (size_t i = 0; i < funcParams.size(); i++) {
          const auto &propertyPair = funcParams[i];
          if (propertyPair.first.IsValid()) {
            if (printComma) {
              functionStream << ", ";
            }

            WriteFunctionParameter(functionStream, propertyPair.first,
                                   propertyPair.second, false);

            if (i >= defaultSuffixStart &&
                propertyPair.first.IsOptionalParameter() &&
                propertyPair.first.Type != EPropertyTypes::FStruct) {
              functionStream << " = {}";
            }

            printComma = true;
          }
        }

        for (const auto &propertyPair : outParams) {
          if (propertyPair.first.IsValid() &&
              propertyPair.first.IsParameter()) {
            if (printComma) {
              functionStream << ", ";
            }

            WriteFunctionParameter(functionStream, propertyPair.first,
                                   propertyPair.second, true);
            printComma = true;
          }
        }

        functionStream << ");\n";

        bool singleUClassParam =
            (funcParams.size() == 1) &&
            outParams.empty() &&
            (funcParams[0].first.Type == EPropertyTypes::UObject) &&
            (funcParams[0].first.GetTypeForClass() == "class UClass*");

        bool returnsObjectPtr =
            returnParam.first.IsValid() &&
            (returnParam.first.Type == EPropertyTypes::UObject ||
                returnParam.first.Type == EPropertyTypes::UClass ||
                returnParam.first.Type == EPropertyTypes::UInterface);

        if (!isStaticFunc && singleUClassParam && returnsObjectPtr)
        {
            std::string funcCallName = functionObj.ValidName;

            if (uFunction->FunctionFlags & EFunctionFlags::FUNC_Event)
            {
                funcCallName = "event" + funcCallName;
            }

            functionStream << "\ttemplate<typename C>\n";
            functionStream << "\ttypename C* " << funcCallName << "()\n";
            functionStream << "\t{\n";
            functionStream << "\t\tUClass* staticClass = C::StaticClass();\n\n";
            functionStream << "\t\tif (staticClass)\n";
            functionStream << "\t\t{\n";
            functionStream << "\t\t\treturn (C*)" << funcCallName << "(staticClass);\n";
            functionStream << "\t\t}\n\n";
            functionStream << "\t\treturn nullptr;\n";
            functionStream << "\t}\n\n";
        }
      }
    }

    stream << functionStream.str();
  }
}

void ProcessFunctions(std::ofstream &stream, class UObject *packageObj) {
  if (packageObj) {
    std::vector<UnrealObject> *objCache =
        GCache::GetCache(packageObj, EClassTypes::UClass);

    if (objCache) {
      for (const UnrealObject &unrealObj : *objCache) {
        GenerateFunctionCode(stream, unrealObj);
      }
    }
  }
}
} // namespace FunctionGenerator

namespace Generator {
static constexpr size_t FILE_BUFFER_SIZE = (1u << 20);

static bool CopyDirectoryTree(const std::filesystem::path &sourceDirectory,
                              const std::filesystem::path &destinationDirectory,
                              bool useHardLinks = false) {
  if (!std::filesystem::exists(sourceDirectory)) {
    return false;
  }

  std::filesystem::create_directories(destinationDirectory);

  std::vector<std::pair<std::filesystem::path, std::filesystem::path>>
      filesToCopy;

  for (const auto &entry :
       std::filesystem::recursive_directory_iterator(sourceDirectory)) {
    const std::filesystem::path relativePath =
        std::filesystem::relative(entry.path(), sourceDirectory);
    const std::filesystem::path destinationPath =
        (destinationDirectory / relativePath);

    if (entry.is_directory()) {
      std::filesystem::create_directories(destinationPath);
    } else if (entry.is_regular_file()) {
      std::filesystem::create_directories(destinationPath.parent_path());

      filesToCopy.emplace_back(entry.path(), destinationPath);
    }
  }

  if (useHardLinks) {
    size_t threadCount = std::thread::hardware_concurrency();

    if (threadCount == 0) {
      threadCount = 4;
    }

    if (threadCount > 8) {
      threadCount = 8;
    }

    if (threadCount > filesToCopy.size()) {
      threadCount = filesToCopy.size();
    }

    if (threadCount == 0) {
      return true;
    }

    std::vector<std::thread> workers;
    workers.reserve(threadCount);

    for (size_t threadIndex = 0; threadIndex < threadCount; ++threadIndex) {
      workers.emplace_back([threadIndex, threadCount, &filesToCopy]() {
        for (size_t fileIndex = threadIndex; fileIndex < filesToCopy.size();
             fileIndex += threadCount) {
          const std::filesystem::path &sourcePath =
              filesToCopy[fileIndex].first;
          const std::filesystem::path &destinationPath =
              filesToCopy[fileIndex].second;

          std::error_code removeError;
          std::filesystem::remove(destinationPath, removeError);

#ifdef _WIN32
          bool wroteFile =
              (CreateHardLinkW(destinationPath.wstring().c_str(),
                               sourcePath.wstring().c_str(), nullptr) != 0);
#else
          bool wroteFile = false;
#endif

          if (!wroteFile) {
            std::filesystem::copy_file(
                sourcePath, destinationPath,
                std::filesystem::copy_options::overwrite_existing);
          }
        }
      });
    }

    for (std::thread &worker : workers) {
      if (worker.joinable()) {
        worker.join();
      }
    }
  } else {
    for (const auto &filePair : filesToCopy) {
      std::filesystem::copy_file(
          filePair.first, filePair.second,
          std::filesystem::copy_options::overwrite_existing);
    }
  }

  return true;
}

static std::filesystem::path GetCacheRoot() {
  return (GConfig::GetOutputPath() / ".sdk_cache" /
          GConfig::GetGameNameShort());
}

static bool
HasCachedSdkOutputs(const std::filesystem::path &cacheHeaderDirectory,
                    const std::filesystem::path &cacheSourceDirectory) {
  if (!std::filesystem::exists(cacheHeaderDirectory / "SdkHeaders.hpp") ||
      !std::filesystem::exists(cacheHeaderDirectory / "GameDefines.hpp") ||
      !std::filesystem::exists(cacheSourceDirectory / "GameDefines.cpp")) {
    return false;
  }

  if (GConfig::UsingConstants() &&
      !std::filesystem::exists(cacheHeaderDirectory / "SdkConstants.hpp")) {
    return false;
  }

  return true;
}

static bool
RegenerateOutputsFromCache(const std::filesystem::path &headerDirectory,
                           const std::filesystem::path &sourceDirectory,
                           const std::filesystem::path &cacheHeaderDirectory,
                           const std::filesystem::path &cacheSourceDirectory) {
  if (!HasCachedSdkOutputs(cacheHeaderDirectory, cacheSourceDirectory)) {
    return false;
  }

  if (!CopyDirectoryTree(cacheHeaderDirectory, headerDirectory, true)) {
    return false;
  }

  if (!CopyDirectoryTree(cacheSourceDirectory, sourceDirectory, true)) {
    return false;
  }

  return true;
}

static void UpdateSdkCache(const std::filesystem::path &headerDirectory,
                           const std::filesystem::path &sourceDirectory) {
  const std::filesystem::path cacheRoot = GetCacheRoot();
  const std::filesystem::path cacheHeaderDirectory =
      (cacheRoot / "SDK_HEADERS");
  const std::filesystem::path cacheSourceDirectory = (cacheRoot / "SDK_SOURCE");

  std::filesystem::remove_all(cacheHeaderDirectory);
  std::filesystem::remove_all(cacheSourceDirectory);
  CopyDirectoryTree(headerDirectory, cacheHeaderDirectory);
  CopyDirectoryTree(sourceDirectory, cacheSourceDirectory);
}

class BoundedTaskQueue {
public:
  explicit BoundedTaskQueue(size_t workerCount) {
    if (workerCount == 0) {
      workerCount = 1;
    }

    m_workers.reserve(workerCount);

    for (size_t workerIndex = 0; workerIndex < workerCount; ++workerIndex) {
      m_workers.emplace_back([this]() { WorkerLoop(); });
    }
  }

  BoundedTaskQueue(const BoundedTaskQueue &) = delete;
  BoundedTaskQueue &operator=(const BoundedTaskQueue &) = delete;

  ~BoundedTaskQueue() { Shutdown(); }

  template <typename TTask> void Enqueue(TTask &&task) {
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_tasks.emplace_back(std::forward<TTask>(task));
    }

    m_workEvent.notify_one();
  }

  void Wait() {
    std::unique_lock<std::mutex> lock(m_mutex);

    m_doneEvent.wait(
        lock, [this]() { return (m_tasks.empty() && (m_activeWorkers == 0)); });

    if (m_firstException) {
      std::rethrow_exception(m_firstException);
    }
  }

private:
  void WorkerLoop() {
    for (;;) {
      std::function<void()> task;

      {
        std::unique_lock<std::mutex> lock(m_mutex);

        m_workEvent.wait(lock,
                         [this]() { return (m_stopping || !m_tasks.empty()); });

        if (m_stopping && m_tasks.empty()) {
          return;
        }

        task = std::move(m_tasks.front());
        m_tasks.pop_front();
        ++m_activeWorkers;
      }

      try {
        task();
      } catch (...) {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_firstException) {
          m_firstException = std::current_exception();
        }
      }

      {
        std::lock_guard<std::mutex> lock(m_mutex);
        --m_activeWorkers;

        if (m_tasks.empty() && (m_activeWorkers == 0)) {
          m_doneEvent.notify_all();
        }
      }
    }
  }

  void Shutdown() {
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_stopping = true;
    }

    m_workEvent.notify_all();

    for (std::thread &worker : m_workers) {
      if (worker.joinable()) {
        worker.join();
      }
    }

    m_workers.clear();
  }

private:
  std::mutex m_mutex;
  std::condition_variable m_workEvent;
  std::condition_variable m_doneEvent;
  std::deque<std::function<void()>> m_tasks;
  std::vector<std::thread> m_workers;
  std::exception_ptr m_firstException;
  size_t m_activeWorkers{0};
  bool m_stopping{false};
};

static void ConfigureFileBuffer(std::ofstream &file) {
  static thread_local std::vector<char> fileBuffer(FILE_BUFFER_SIZE);
  file.rdbuf()->pubsetbuf(fileBuffer.data(),
                          static_cast<std::streamsize>(fileBuffer.size()));
}

void GenerateConstants() {
  if (GConfig::UsingConstants()) {
    std::ofstream constantsFile;
    ConfigureFileBuffer(constantsFile);
    constantsFile.open(std::filesystem::path(GConfig::GetOutputPath()) /
                       GConfig::GetGameNameShort() / "SDK_HEADERS" /
                       "SdkConstants.hpp");
    constantsFile << "#pragma once\n";

    std::map<std::string, class UObject *> *constants = GCache::GetConstants();

    for (const auto &constantPair : *constants) {
      if (constantPair.second) {
        constantsFile << "#define " << constantPair.first;
        Printer::FillRight(constantsFile, ' ',
                           (CONST_VALUE_SPACING - constantPair.first.length()));
        constantsFile << constantPair.second->ObjectInternalInteger << "\n";
      }
    }

    constantsFile.close();
  }
}

void GenerateHeaders() {
  std::ofstream headersFile;
  ConfigureFileBuffer(headersFile);
  headersFile.open(std::filesystem::path(GConfig::GetOutputPath()) /
                   GConfig::GetGameNameShort() / "SDK_HEADERS" /
                   "SdkHeaders.hpp");

  Printer::Header(headersFile, "SdkHeaders", "hpp", false);
  headersFile << "#pragma once\n";
  Printer::Section(headersFile, "Includes");

  headersFile << "#include \"GameDefines.hpp\"\n";
  std::vector<UnrealObject> *packages = GCache::GetPackages();

  for (const UnrealObject &packageObj : *packages) {
    if (packageObj.IsValid()) {
      headersFile << "#include \"" << packageObj.ValidName << "/"
                  << packageObj.ValidName << "_constants.hpp\"\n";
      headersFile << "#include \"" << packageObj.ValidName << "/"
                  << packageObj.ValidName << "_enums.hpp\"\n";
      headersFile << "#include \"" << packageObj.ValidName << "/"
                  << packageObj.ValidName << "_structs.hpp\"\n";
      headersFile << "#include \"" << packageObj.ValidName << "/"
                  << packageObj.ValidName << "_classes.hpp\"\n";
      headersFile << "#include \"" << packageObj.ValidName << "/"
                  << packageObj.ValidName << "_parameters.hpp\"\n";
    }
  }

  headersFile << "\n";

  Printer::Footer(headersFile, false);
  headersFile.close();
}

void GenerateDefines() {
  std::ofstream definesFile;
  ConfigureFileBuffer(definesFile);
  definesFile.open(GConfig::GetOutputPath() / GConfig::GetGameNameShort() /
                   "SDK_HEADERS" / "GameDefines.hpp");

  Printer::Header(definesFile, "GameDefines", "hpp", false);

  definesFile << "#pragma once\n";

  if (GConfig::UsingWindows()) {
    definesFile << "#include <windows.h>\n";
  }

  definesFile << "#include <algorithm>\n";
  definesFile << "#include <locale>\n";
  definesFile << "#include <stdlib.h>\n";
  definesFile << "#include <xlocale>\n";
  definesFile << "#include <ctype.h>\n";
  definesFile << "#include <chrono>\n";
  definesFile << "#include <thread>\n";
  definesFile << "#include <vector>\n";
  definesFile << "#include <string>\n";
  definesFile << "#include <map>\n";

  if (GConfig::PrintEnumFlags()) {
    Printer::Section(definesFile, "Flags");
    definesFile << PiecesOfCode::EEnumFlags;
  }

  Printer::Section(definesFile, "Globals");

  if (GConfig::UsingOffsets()) {
    definesFile << "// GObjects\n";
    definesFile << "#define GObjects_Offset\t\t(uintptr_t)"
                << Printer::Hex(GConfig::GetGObjectOffset(), sizeof(uintptr_t))
                << "\n";

    definesFile << "// GNames\n";
    definesFile << "#define GNames_Offset\t\t(uintptr_t)"
                << Printer::Hex(GConfig::GetGNameOffset(), sizeof(uintptr_t))
                << "\n";
  } else {
    definesFile << "// GObjects\n";
    definesFile << "#define GObjects_Pattern\t\t(const uint8_t*)\""
                << GConfig::GetGObjectStr() + "\"\n";
    definesFile << "#define GObjects_Mask\t\t\t(const char*)\""
                << GConfig::GetGObjectMask() + "\"\n";

    definesFile << "// GNames\n";
    definesFile << "#define GNames_Pattern\t\t\t(const uint8_t*)\""
                << GConfig::GetGNameStr() + "\"\n";
    definesFile << "#define GNames_Mask\t\t\t\t(const char*)\""
                << GConfig::GetGNameMask() + "\"\n";

    if (GConfig::GetCallFunctionIndex() == -1) {
      definesFile << "// Process Event\n";
      definesFile << "#define CallFunction_Pattern\t(const uint8_t*)\""
                  << GConfig::GetCallFunctionStr() << "\"\n";
      definesFile << "#define CallFunction_Mask\t\t(const char*)\""
                  << GConfig::GetCallFunctionMask() << "\"\n";
    }
  }

  definesFile << "#define RESULT_DECL void*const Result\n";

  Printer::Section(definesFile, "Classes");
  definesFile << PiecesOfCode::TArray_Iterator << "\n";
  definesFile << PiecesOfCode::TArray_Class << "\n";
  definesFile << PiecesOfCode::TMap_Class << "\n";

  definesFile << "extern class TArray<class UObject*>* GObjects;\n";
  definesFile << "extern class TArray<class FNameEntry*>* GNames;\n";

  Printer::Section(definesFile, "Structs");
  definesFile << PiecesOfCode::FNameEntry_Struct << "\n";
  StructGenerator::GenerateStructMembers(definesFile, EClassTypes::FNameEntry);

#ifdef UTF16
  definesFile << PiecesOfCode::FNameEntry_Struct_UTF16 << "\n";
  definesFile << PiecesOfCode::FName_Struct_UTF16 << "\n";
#else
  definesFile << PiecesOfCode::FNameEntry_Struct_UTF8 << "\n";
  definesFile << PiecesOfCode::FName_Struct_UTF8 << "\n";
#endif

#ifdef UTF16_FSTRING
  definesFile << PiecesOfCode::FString_Class_UTF16 << "\n";
#else
  definesFile << PiecesOfCode::FString_Class_UTF8 << "\n";
#endif

  definesFile << PiecesOfCode::FScriptDelegate_Struct << "\n";
  definesFile << PiecesOfCode::FRepRecord_Struct << "\n";
  definesFile << PiecesOfCode::FImplementedInterface_Struct << "\n";

  definesFile << PiecesOfCode::FPointer_Struct << "\n";
  definesFile << PiecesOfCode::FQWord_Struct << "\n";

  definesFile << PiecesOfCode::FOutputDevice_Struct << "\n";
  definesFile << PiecesOfCode::FOutParamRec_Struct << "\n";
  definesFile << PiecesOfCode::FFrame_Struct << "\n";

  Printer::Footer(definesFile, false);
  definesFile.close();

  ConfigureFileBuffer(definesFile);
  definesFile.open(GConfig::GetOutputPath() / GConfig::GetGameNameShort() /
                   "SDK_SOURCE" / "GameDefines.cpp");

  Printer::Header(definesFile, "GameDefines", "cpp", false);

  definesFile << "#include \"../SDK_HEADERS/GameDefines.hpp\"\n";
  Printer::Section(definesFile, "Initialize Globals");
  definesFile << "class TArray<class UObject*>* GObjects{};\n";
  definesFile << "class TArray<class FNameEntry*>* GNames{};\n\n";

  Printer::Footer(definesFile, false);
  definesFile.close();
}

void ProcessPackages(const std::filesystem::path &headersDirectory,
                     const std::filesystem::path &sourceDirectory) {
  if (std::filesystem::exists(headersDirectory) &&
      std::filesystem::exists(sourceDirectory)) {
    std::vector<UnrealObject> *packages = GCache::GetPackages();
    unsigned int hardwareThreads = std::thread::hardware_concurrency();
    size_t workerCount =
        (hardwareThreads == 0) ? 4 : static_cast<size_t>(hardwareThreads);

    if (workerCount > 8) {
      workerCount = 8;
    } else if (workerCount < 2) {
      workerCount = 2;
    }

    BoundedTaskQueue taskQueue(workerCount);

#ifndef NO_LOGGING
    constexpr bool kVerbosePackageLogging = false;
#endif

    for (const UnrealObject &packageObj : *packages) {
      if (packageObj.IsValid()) {
#ifndef NO_LOGGING
        if (kVerbosePackageLogging) {
          GLogger::Log("\nProcessing Package: " + packageObj.ValidName + "\n");
        }
#endif

        std::ofstream file;

        std::filesystem::create_directories(headersDirectory /
                                            packageObj.ValidName);
        std::filesystem::create_directories(sourceDirectory /
                                            packageObj.ValidName);

        const UnrealObject packageCopy = packageObj;
        const std::filesystem::path headersPath = headersDirectory;
        const std::filesystem::path sourcePath = sourceDirectory;

        taskQueue.Enqueue([headersPath, packageCopy]() {
          std::ofstream file;
          ConfigureFileBuffer(file);
          file.open(headersPath / (packageCopy.ValidName + "\\" +
                                   packageCopy.ValidName + "_constants.hpp"));
          Printer::Header(file,
                          (packageCopy.ValidName + "\\" +
                           packageCopy.ValidName + "_constants"),
                          "hpp", true);
          Printer::Section(file, "Constants");
          ConstGenerator::ProcessConsts(file, packageCopy.Object);
          Printer::Footer(file, true);
          file.close();
        });

        taskQueue.Enqueue([headersPath, packageCopy]() {
          std::ofstream file;
          ConfigureFileBuffer(file);
          file.open(headersPath / (packageCopy.ValidName + "\\" +
                                   packageCopy.ValidName + "_enums.hpp"));
          Printer::Header(
              file,
              (packageCopy.ValidName + "\\" + packageCopy.ValidName + "_enums"),
              "hpp", true);
          Printer::Section(file, "Enums");
          EnumGenerator::ProcessEnums(file, packageCopy.Object);
          Printer::Footer(file, true);
          file.close();
        });

        // Structs
        ConfigureFileBuffer(file);
        file.open(headersDirectory / (packageObj.ValidName + "\\" +
                                      packageObj.ValidName + "_structs.hpp"));
        Printer::Header(
            file,
            (packageObj.ValidName + "\\" + packageObj.ValidName + "_structs"),
            "hpp", true);
        Printer::Section(file, "Structs");
        StructGenerator::ProcessStructs(file, packageObj.Object);
        Printer::Footer(file, true);
        file.close();

        // Classes
        ConfigureFileBuffer(file);
        file.open(headersDirectory / (packageObj.ValidName + "\\" +
                                      packageObj.ValidName + "_classes.hpp"));
        Printer::Header(
            file,
            (packageObj.ValidName + "\\" + packageObj.ValidName + "_classes"),
            "hpp", true);
        Printer::Section(file, "Classes");
        ClassGenerator::ProcessClasses(file, packageObj.Object);
        Printer::Footer(file, true);
        file.close();

        taskQueue.Enqueue([headersPath, packageCopy]() {
          std::ofstream file;
          ConfigureFileBuffer(file);
          file.open(headersPath / (packageCopy.ValidName + "\\" +
                                   packageCopy.ValidName + "_parameters.hpp"));
          Printer::Header(file,
                          (packageCopy.ValidName + "\\" +
                           packageCopy.ValidName + "_parameters"),
                          "hpp", true);
          Printer::Section(file, "Parameters");
          ParameterGenerator::ProcessParameters(file, packageCopy.Object);
          Printer::Footer(file, true);
          file.close();
        });

        taskQueue.Enqueue([sourcePath, packageCopy]() {
          std::ofstream file;
          ConfigureFileBuffer(file);
          file.open(sourcePath / (packageCopy.ValidName + "\\" +
                                  packageCopy.ValidName + "_classes.cpp"));
          Printer::Header(file,
                          (packageCopy.ValidName + "\\" +
                           packageCopy.ValidName + "_classes"),
                          "cpp", true);
          Printer::Section(file, "Functions");
          FunctionGenerator::ProcessFunctions(file, packageCopy.Object);
          Printer::Footer(file, true);
          file.close();
        });
      }
    }

    taskQueue.Wait();
  } else {
    Utils::MessageboxError(
        "Failed locate the given directory, cannot generate an SDK!");
  }
}

void GenerateSDK() {
  std::filesystem::path fullDirectory =
      (GConfig::GetOutputPath() / GConfig::GetGameNameShort());
  std::filesystem::path headerDirectory = (fullDirectory / "SDK_HEADERS");
  std::filesystem::path sourceDirectory = (fullDirectory / "SDK_SOURCE");
  std::filesystem::path cacheRoot = GetCacheRoot();
  std::filesystem::path cacheHeaderDirectory = (cacheRoot / "SDK_HEADERS");
  std::filesystem::path cacheSourceDirectory = (cacheRoot / "SDK_SOURCE");

  std::filesystem::create_directory(GConfig::GetOutputPath());
  std::filesystem::create_directory(fullDirectory);
  std::filesystem::create_directory(headerDirectory);
  std::filesystem::create_directory(sourceDirectory);

  if (std::filesystem::exists(headerDirectory) &&
      std::filesystem::exists(headerDirectory)) {
    if (Initialize(true)) {
      std::chrono::time_point startTime = std::chrono::steady_clock::now();
      bool generatedFromCache = false;

#ifdef ULTRA_FAST_FILE_REGEN
      generatedFromCache = RegenerateOutputsFromCache(
          headerDirectory, sourceDirectory, cacheHeaderDirectory,
          cacheSourceDirectory);
#endif

      if (!generatedFromCache) {
        std::future<void> packagesTask = std::async(
            std::launch::async, [headerDirectory, sourceDirectory]() {
              ProcessPackages(headerDirectory, sourceDirectory);
            });

        std::future<void> headersTask =
            std::async(std::launch::async, []() { GenerateHeaders(); });

        std::future<void> definesTask =
            std::async(std::launch::async, []() { GenerateDefines(); });

        std::future<void> constantsTask =
            std::async(std::launch::async, []() { GenerateConstants(); });

        packagesTask.get();
        headersTask.get();
        definesTask.get();
        constantsTask.get();

#ifdef ULTRA_FAST_FILE_REGEN
        UpdateSdkCache(headerDirectory, sourceDirectory);
#endif
      }

      if (!generatedFromCache) {
        GCache::ClearCache();
      }

      std::chrono::time_point endTime = std::chrono::steady_clock::now();
      std::string formattedTime = Printer::Precision(
          std::chrono::duration<float>(endTime - startTime).count(), 4);

#ifndef NO_LOGGING
      GLogger::Log("\n" + GConfig::GetGameNameShort() + " generated in " +
                   formattedTime + " seconds.");
      GLogger::Close();
#endif

      Utils::MessageboxInfo("SDK generation complete, finished in " +
                            formattedTime + " seconds!");
    }
  } else {
    Utils::MessageboxError("Failed to create the source or headers directory, "
                           "cannot generate an SDK!");
  }
}

bool Initialize(bool bCreateLog) {
  if (!GConfig::HasOutputPath()) {
    Utils::MessageboxError(
        "Looks like you forgot to set an output path for the generator! Please "
        "edit the output path in \"Configuration.cpp\" and recompile.");
    return false;
  }

  if (!AreGlobalsValid()) {
    if (GConfig::UsingOffsets()) {
      GObjects = reinterpret_cast<TArray<UObject *> *>(
          Retrievers::GetBaseAddress() + GConfig::GetGObjectOffset());
      GNames = reinterpret_cast<TArray<FNameEntry *> *>(
          Retrievers::GetBaseAddress() + GConfig::GetGNameOffset());
    } else {
      uintptr_t GObject = Retrievers::FindPattern(GConfig::GetGObjectPattern(),
                                                  GConfig::GetGObjectMask());
      uintptr_t GName = Retrievers::FindPattern(GConfig::GetGNamePattern(),
                                                GConfig::GetGNameMask());

      if (GObject && GName) {
        DWORD ObjectAddr = *(DWORD *)(GObject + 0x3);
        auto GObjectsAddress = (GObject + (uintptr_t)ObjectAddr) + 0x7;

        DWORD NameAddr = *(DWORD *)(GName + 0x3);
        auto GNamesAddress = (GName + (uintptr_t)NameAddr) + 0x7;

        GObjects = reinterpret_cast<TArray<UObject *> *>(GObjectsAddress);
        GNames = reinterpret_cast<TArray<FNameEntry *> *>(GNamesAddress);
      }

      if (!AreGlobalsValid()) {
        GObjects = reinterpret_cast<TArray<UObject *> *>(
            Retrievers::GetBaseAddress() + GConfig::GetGObjectOffset());
        GNames = reinterpret_cast<TArray<FNameEntry *> *>(
            Retrievers::GetBaseAddress() + GConfig::GetGNameOffset());
      }
    }

    if (AreGlobalsValid()) {
      // Here is where that "REGISTER_MEMBER" macro is used, these functions
      // calculate offsets for each class member. There might be a better and
      // automated way of doing this, so maybe I'll change this in the future
      // when I'm less lazy.
      //
      // Structs
      FNameEntry::Register_HashNext();
      FNameEntry::Register_Index();
      FNameEntry::Register_Flags();
      FNameEntry::Register_Name();

      // Objects
      UObject::Register_VfTableObject();
      UObject::Register_ObjectInternalInteger();
      UObject::Register_HashNext();
      UObject::Register_ObjectFlags();
      UObject::Register_HashOuterNext();
      UObject::Register_StateFrame();
      UObject::Register_Linker();
      UObject::Register_LinkerIndex();
      UObject::Register_NetIndex();
      UObject::Register_Outer();
      UObject::Register_Name();
      UObject::Register_Class();
      UObject::Register_ObjectArchetype();

      UField::Register_Next();
      UField::Register_UniqueFeatureValue();

      UEnum::Register_Names();
      UConst::Register_Value();
      UProperty::Register_ArraySize();
      UProperty::Register_PropertySize();
      UProperty::Register_PropertyFlags();
      UProperty::Register_RepOffset();
      UProperty::Register_RepIndex();
      UProperty::Register_Category();
      UProperty::Register_ArraySizeEnum();
      UProperty::Register_Offset();
      UProperty::Register_PropertyLinkNext();
      UProperty::Register_ConstructorLinkNext();
      UProperty::Register_NextRef();
      UProperty::Register_Alias();

      UStruct::Register_SuperField();
      UStruct::Register_Children();
      UStruct::Register_PropertySize();
      UStruct::Register_MinAlignment();
      UFunction::Register_FunctionFlags();
      UFunction::Register_iNative();
      UFunction::Register_RepOffset();
      UFunction::Register_FriendlyName();
      UFunction::Register_OperPrecedence();
      UFunction::Register_NumParms();
      UFunction::Register_ParmsSize();
      UFunction::Register_ReturnValueOffset();
      UFunction::Register_Func();

      UDelegateProperty::Register_SuccessCallback();
      UDelegateProperty::Register_FailureCallback();

      // Class
      UClass::Register_ClassFlags();
      UClass::Register_ClassUnique();
      UClass::Register_ClassDelegates();
      UClass::Register_ClassCastFlags();
      UClass::Register_ClassWithin();
      UClass::Register_Category();
      UClass::Register_ClassReps();
      UClass::Register_NetFields();
      UClass::Register_HideCategories();
      UClass::Register_AutoExpandCategories();
      UClass::Register_AutoCollapseCategories();
      UClass::Register_DontSortCategories();
      UClass::Register_DependentOn();
      UClass::Register_ClassGroupNames();
      UClass::Register_bForceScriptOrder();
      UClass::Register_ClassHeaderFilename();
      UClass::Register_ManualPad();
      UClass::Register_ClassDefaultObject();
      UClass::Register_ClassConstructor();
      UClass::Register_ClassStaticConstructor();
      UClass::Register_ClassStaticInitializer();
      UClass::Register_ComponentNameToDefaultObjectMap();
      UClass::Register_Interfaces();
      UClass::Register_DefaultPropText();
      UClass::Register_bNeedsPropertiesLinked();
      UClass::Register_ReferenceTokenStream();
      UClass::Register_ClassStates();

      UStructProperty::Register_Struct();
      UObjectProperty::Register_PropertyClass();
      UClassProperty::Register_MetaClass();
      UMapProperty::Register_Key();
      UMapProperty::Register_Value();
      UInterfaceProperty::Register_InterfaceClass();
      UByteProperty::Register_Enum();
      UBoolProperty::Register_BitMask();
      UArrayProperty::Register_Inner();

#ifndef NO_LOGGING
      std::chrono::time_point startTime = std::chrono::system_clock::now();
#endif

      GCache::Initialize(); // Cache all object instances needed for generation.
#ifndef NO_LOGGING
      std::chrono::time_point endTime = std::chrono::system_clock::now();
      std::string formattedTime = Printer::Precision(
          std::chrono::duration<float>(endTime - startTime).count(), 4);

      if (bCreateLog && GLogger::Open()) {
        GLogger::Log("Base: " + Printer::Hex(Retrievers::GetBaseAddress(),
                                             sizeof(uintptr_t)));
        GLogger::Log("GObjects: " + Printer::Hex(GObjects));
        GLogger::Log("GNames: " + Printer::Hex(GNames));

        GLogger::Log("\n" + GConfig::GetGameNameShort() +
                     " objects cached in " + formattedTime + " seconds.");
      }
#endif
    } else {
      Utils::MessageboxError(
          "Failed to validate GObject & GNames, please make sure you have them "
          "configured properly in \"Configuration.cpp\"!");
      return false;
    }
  }

  if (AreGlobalsValid()) {
#ifndef NO_LOGGING
    if (bCreateLog &&
        GLogger::Open()) // Will return false if the file is already open.
    {
      GLogger::Log("Base: " + Printer::Hex(Retrievers::GetBaseAddress(),
                                           sizeof(uintptr_t)));
      GLogger::Log("GObjects: " + Printer::Hex(GObjects));
      GLogger::Log("GNames: " + Printer::Hex(GNames));
    }
#endif
    return true;
  }

  return false;
}

void DumpInstances(bool bNames, bool bObjects) {
  if (Initialize(false)) {
    if (bNames) {
      DumpGNames();
    }

    if (bObjects) {
      DumpGObjects();
    }

    Utils::MessageboxInfo("Finished dumping instances!");
  }
}

void DumpGNames() {
  if (Initialize(false) && AreGlobalsValid()) {
    std::filesystem::path fullDirectory =
        (GConfig::GetOutputPath() / GConfig::GetGameNameShort());
    std::filesystem::create_directory(GConfig::GetOutputPath());
    std::filesystem::create_directory(fullDirectory);

    if (std::filesystem::exists(fullDirectory)) {
      std::ofstream file(fullDirectory / "NameDump.txt");
      file << "Base: "
           << Printer::Hex(Retrievers::GetBaseAddress(), sizeof(uintptr_t))
           << "\n";
      file << "GNames: " << Printer::Hex(GNames) << "\n";
      file << "Offset: "
           << Printer::Hex(Retrievers::GetOffset(GNames), sizeof(uintptr_t))
           << "\n\n";

      for (int32_t i = 0; i < FName::Names()->size(); i++) {
        FNameEntry *nameEntry = FName::Names()->at(i);

        if (nameEntry) {
          std::string name = nameEntry->ToString();

          if (!name.empty()) {
            file << "Name[";
            Printer::FillRight(file, '0', 6);
            file << nameEntry->GetIndex() << "] " << name << " ";

            if (name.length() < INSTANCE_DUMP_SPACING) {
              Printer::FillRight(file, ' ',
                                 (INSTANCE_DUMP_SPACING - name.length()));
            }

            file << Printer::Hex(nameEntry) << "\n";
          }
        }
      }

      file.close();
    }
  }
}

void DumpGObjects() {
  if (Initialize(false) && AreGlobalsValid()) {
    std::filesystem::path fullDirectory =
        (GConfig::GetOutputPath() / GConfig::GetGameNameShort());
    std::filesystem::create_directory(GConfig::GetOutputPath());
    std::filesystem::create_directory(fullDirectory);

    if (std::filesystem::exists(fullDirectory)) {
      std::ofstream file(fullDirectory / "ObjectDump.txt");
      file << "Base: "
           << Printer::Hex(Retrievers::GetBaseAddress(), sizeof(uintptr_t))
           << "\n";
      file << "GObjects: " << Printer::Hex(GObjects) << "\n";
      file << "Offset: "
           << Printer::Hex(Retrievers::GetOffset(GObjects), sizeof(uintptr_t))
           << "\n\n";

      for (int32_t i = 0; i < (UObject::GObjObjects()->size() - 1); i++) {
        UObject *uObject = UObject::GObjObjects()->at(i);

        if (uObject) {
          std::string name = uObject->GetFullName();

          if (!name.empty()) {

            std::stringstream ss;
            ss << "0x" << std::hex << std::uppercase
               << reinterpret_cast<uintptr_t>(uObject); // cast to integer type

            std::string hex_address = ss.str();

            file << "UObject[";
            Printer::FillRight(file, '0', 6);
            file << hex_address << "] " << name << " ";

            if (name.length() < INSTANCE_DUMP_SPACING) {
              Printer::FillRight(file, ' ',
                                 (INSTANCE_DUMP_SPACING - name.length()));
            }

            file << Printer::Hex(uObject) << "\n";
          }
        }
      }

      file.close();
    }
  }
}

bool AreGObjectsValid() {
  if (GObjects && !UObject::GObjObjects()->empty() &&
      (UObject::GObjObjects()->capacity() > UObject::GObjObjects()->size())) {
    return true;
  }

  return false;
}

bool AreGNamesValid() {
  if (GNames && !FName::Names()->empty() &&
      (FName::Names()->capacity() > FName::Names()->size())) {
    return true;
  }

  return false;
}

bool AreGlobalsValid() {
  static bool globalsValid = false;

  if (!globalsValid) {
    globalsValid = (AreGObjectsValid() && AreGNamesValid());
  }

  return globalsValid;
}
} // namespace Generator

DWORD WINAPI OnAttach(LPVOID lpParameter) {
  HMODULE hModule = reinterpret_cast<HMODULE>(lpParameter);

  try {
    Generator::GenerateSDK();
#ifndef SKIP_INSTANCE_DUMPS
    Generator::DumpInstances(true, true);
#endif
  } catch (const std::exception &exception) {
    Utils::MessageboxError(std::string("SDK generation failed: ") +
                           exception.what());
  } catch (...) {
    Utils::MessageboxError("SDK generation failed due to an unknown error.");
  }

  if (hModule) {
    FreeLibraryAndExitThread(hModule, 0);
  }

  return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call,
                      LPVOID lpReserved) {
  switch (ul_reason_for_call) {
  case DLL_PROCESS_ATTACH: {
    DisableThreadLibraryCalls(hModule);

    HANDLE threadHandle =
        CreateThread(nullptr, 0, OnAttach, hModule, 0, nullptr);

    if (!threadHandle) {
      return FALSE;
    }

    CloseHandle(threadHandle);
    break;
  }
  case DLL_THREAD_ATTACH:
  case DLL_THREAD_DETACH:
  case DLL_PROCESS_DETACH:
    break;
  }
  return TRUE;
}
