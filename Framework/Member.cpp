#include "../Engine/Engine.hpp"

/*
# ========================================================================================= #
# Members
# ========================================================================================= #
*/

Member::Member() : Type(EMemberTypes::Unknown), Label(GetLabel(Type)), Offset(0), Size(1) {}

Member::Member(EMemberTypes type, size_t size) : Type(type), Label(GetLabel(type)), Offset(GetOffset(type)), Size(size) {}

Member::Member(const Member& member) : Type(member.Type), Label(member.Label), Offset(member.Offset), Size(member.Size) {}

Member::~Member() {}

std::string Member::GetName(EClassTypes type)
{
	switch (type)
	{
	case EClassTypes::FNameEntry:
		return "FNameEntry";
	case EClassTypes::UObject:
		return "UObject";
	case EClassTypes::UClass:
		return "UClass";
	case EClassTypes::UField:
		return "UField";
	case EClassTypes::UEnum:
		return "UEnum";
	case EClassTypes::UConst:
		return "UConst";
	case EClassTypes::UProperty:
		return "UProperty";
	case EClassTypes::UStruct:
		return "UStruct";
	case EClassTypes::UFunction:
		return "UFunction";
	case EClassTypes::UDelegateProperty:
		return "UDelegateProperty";
	case EClassTypes::UStructProperty:
		return "UStructProperty";
	case EClassTypes::UObjectProperty:
		return "UObjectProperty";
	case EClassTypes::UClassProperty:
		return "UClassProperty";
	case EClassTypes::UMapProperty:
		return "UMapProperty";
	case EClassTypes::UInterfaceProperty:
		return "UInterfaceProperty";
	case EClassTypes::UByteProperty:
		return "UByteProperty";
	case EClassTypes::UBoolProperty:
		return "UBoolProperty";
	case EClassTypes::UArrayProperty:
		return "UArrayProperty";
	default:
		return "Unknown";
	}
}

std::string Member::GetLabel(EMemberTypes type)
{
	switch (type)
	{
	case EMemberTypes::FNameEntry_HashNext:
		return "class FNameEntry* HashNext;";
	case EMemberTypes::FNameEntry_Index:
		return "int32_t Index;";
	case EMemberTypes::FNameEntry_Flags:
		return "uint64_t Flags;";
	case EMemberTypes::FNameEntry_Name:
#ifdef UTF16
		return "wchar_t Name[0x400];";
#else
		return "char Name[0x400];";
#endif
	case EMemberTypes::UObject_VfTable:
		return "struct FPointer VfTableObject;";
	case EMemberTypes::UObject_HashNext:
		return "struct FPointer HashNext;";
	case EMemberTypes::UObject_ObjectFlags:
		return "uint64_t ObjectFlags;";
	case EMemberTypes::UObject_HashOuterNext:
		return "struct FPointer HashOuterNext;";
	case EMemberTypes::UObject_StateFrame:
		return "struct FPointer StateFrame;";
	case EMemberTypes::UObject_Linker:
		return "class UObject* Linker;";
	case EMemberTypes::UObject_LinkerIndex:
		return "struct FPointer LinkerIndex;";
	case EMemberTypes::UObject_Integer:
		return "int32_t ObjectInternalInteger;";
	case EMemberTypes::UObject_NetIndex:
		return "int32_t NetIndex;";
	case EMemberTypes::UObject_Outer:
		return "class UObject* Outer;";
	case EMemberTypes::UObject_Name:
		return "class FName Name;";
	case EMemberTypes::UObject_Class:
		return "class UClass* Class;";
	case EMemberTypes::UObject_Archetype:
		return "class UObject* ObjectArchetype;";
	case EMemberTypes::UField_Next:
		return "class UField* Next;";
	case EMemberTypes::UField_UniqueFeatureValue:
		return "uint64_t UniqueFeatureValue;";
	case EMemberTypes::UEnum_Names:
		return "class TArray<class FName> Names;";
	case EMemberTypes::UConst_Value:
		return "class FString Value;";
	case EMemberTypes::UProperty_ArrayDim:
		return "int32_t ArraySize;";

	case EMemberTypes::UProperty_PropertySize:
		return "int32_t PropertySize;";

	case EMemberTypes::UProperty_PropertyFlags:
		return "uint64_t PropertyFlags;";

	case EMemberTypes::UProperty_RepOffset:
		return "uint16_t RepOffset;";

	case EMemberTypes::UProperty_RepIndex:
		return "uint16_t RepIndex;";

	case EMemberTypes::UProperty_Category:
		return "FName Category;";

	case EMemberTypes::UProperty_ArraySizeEnum:
		return "UEnum* ArraySizeEnum;";

	case EMemberTypes::UProperty_Offset:
		return "uint32_t Offset;";

	case EMemberTypes::UProperty_PropertyLinkNext:
		return "UProperty* PropertyLinkNext;";

	case EMemberTypes::UProperty_ConstructorLinkNext:
		return "UProperty* ConstructorLinkNext;";

	case EMemberTypes::UProperty_NextRef:
		return "UProperty* NextRef;";

	case EMemberTypes::UProperty_Alias:
		return "FString Alias;";
	case EMemberTypes::UStruct_SuperField:
		return "class UField* SuperField;";
	case EMemberTypes::UStruct_Children:
		return "class UField* Children;";
	case EMemberTypes::UStruct_Size:
		return "int32_t PropertySize;";
	case EMemberTypes::UStruct_Alignment:
		return "int32_t MinAlignment;";
	case EMemberTypes::UFunction_Flags:
		return "uint64_t FunctionFlags;";
	case EMemberTypes::UFunction_Native:
		return "uint16_t iNative;";
	case EMemberTypes::UFunction_RepOffset:
		return "uint16_t RepOffset;";
	case EMemberTypes::UFunction_FriendlyName:
		return "class FName FriendlyName;";
	case EMemberTypes::UFunction_OperPrecedence:
		return "uint8_t OperPrecedence;";
	case EMemberTypes::UFunction_NumParms:
		return "uint8_t NumParms;";
	case EMemberTypes::UFunction_ParmsSize:
		return "uint16_t ParmsSize;";
	case EMemberTypes::UFunction_ReturnValueOffset:
		return "uint16_t ReturnValueOffset;";
	case EMemberTypes::UFunction_Func:
		return "FPointer Func;";
	case EMemberTypes::UStructProperty_Struct:
		return "class UStruct* Struct;";
	case EMemberTypes::UObjectProperty_Class:
		return "class UClass* PropertyClass;";
	case EMemberTypes::UClassProperty_Meta:
		return "class UClass* MetaClass;";
	case EMemberTypes::UMapProperty_Key:
		return "class UProperty* Key;";
	case EMemberTypes::UMapProperty_Value:
		return "class UProperty* Value;";
	case EMemberTypes::UInterfaceProperty_Class:
		return "class UClass* InterfaceClass;";
	case EMemberTypes::UByteProperty_Enum:
		return "class UEnum* Enum;";
	case EMemberTypes::UBoolProperty_BitMask:
		return "uint32_t BitMask;";
	case EMemberTypes::UArrayProperty_Inner:
		return "class UProperty* Inner;";
	case EMemberTypes::UClass_ClassFlags:
		return "uint32_t ClassFlags;";
	case EMemberTypes::UClass_ClassUnique:
		return "uint32_t ClassUnique;";
	case EMemberTypes::UClass_ClassDelegates:
		return "TMap<FName, UDelegateProperty*> ClassDelegates;";
	case EMemberTypes::UClass_ClassCastFlags:
		return "uint32_t ClassCastFlags;";
	case EMemberTypes::UClass_ClassWithin:
		return "UClass* ClassWithin;";
	case EMemberTypes::UClass_Category:
		return "FName Category;";
	case EMemberTypes::UClass_ClassReps:
		return "TArray<FRepRecord> ClassReps;";
	case EMemberTypes::UClass_NetFields:
		return "TArray<UField*> NetFields;";
	case EMemberTypes::UClass_HideCategories:
		return "TArray<FName> HideCategories;";
	case EMemberTypes::UClass_AutoExpandCategories:
		return "TArray<FName> AutoExpandCategories;";
	case EMemberTypes::UClass_AutoCollapseCategories:
		return "TArray<FName> AutoCollapseCategories;";
	case EMemberTypes::UClass_DontSortCategories:
		return "TArray<FName> DontSortCategories;";
	case EMemberTypes::UClass_DependentOn:
		return "TArray<FName> DependentOn;";
	case EMemberTypes::UClass_ClassGroupNames:
		return "TArray<FName> ClassGroupNames;";
	case EMemberTypes::UClass_bForceScriptOrder:
		return "uint32_t bForceScriptOrder;";
	case EMemberTypes::UClass_ClassHeaderFilename:
		return "FString ClassHeaderFilename;";
	case EMemberTypes::UClass_ClassDefaultObject:
		return "UObject* ClassDefaultObject;";
	case EMemberTypes::UClass_ClassConstructor:
		return "void* ClassConstructor;";
	case EMemberTypes::UClass_ClassStaticConstructor:
		return "void* ClassStaticConstructor;";
	case EMemberTypes::UClass_ClassStaticInitializer:
		return "void* ClassStaticInitializer;";
	case EMemberTypes::UClass_ComponentNameToDefaultObjectMap:
		return "TMap<FName, UObject*> ComponentNameToDefaultObjectMap;";
	case EMemberTypes::UClass_Interfaces:
		return "TArray<FImplementedInterface> Interfaces;";
	case EMemberTypes::UClass_DefaultPropText:
		return "FString DefaultPropText;";
	case EMemberTypes::UClass_bNeedsPropertiesLinked:
		return "uint32_t bNeedsPropertiesLinked;";
	case EMemberTypes::UClass_ReferenceTokenStream:
		return "TArray<uint32_t> ReferenceTokenStream;";
	case EMemberTypes::UClass_ClassStates:
		return "TMap<FName, UState*> ClassStates;";
	case EMemberTypes::UClass_ManualPad:
		return "uint8_t ManualPad[40];";
	case EMemberTypes::UDelegateProperty_SuccessCallback:
		return "UFunction* SuccessCallback;";
	case EMemberTypes::UDelegateProperty_FailureCallback:
		return "UFunction* FailureCallback;";
	default:
		return "uint8_t UnknownMemberType[0x1];";
	}
}

uintptr_t Member::GetOffset(EMemberTypes type)
{
	switch (type)
	{
	case EMemberTypes::FNameEntry_HashNext:
		return offsetof(FNameEntry, HashNext);
	case EMemberTypes::FNameEntry_Index:
		return offsetof(FNameEntry, Index);
	case EMemberTypes::FNameEntry_Flags:
		return offsetof(FNameEntry, Flags);
	case EMemberTypes::FNameEntry_Name:
		return offsetof(FNameEntry, Name);
	case EMemberTypes::UObject_VfTable:
		return offsetof(UObject, VfTableObject);
	case EMemberTypes::UObject_HashNext:
		return offsetof(UObject, HashNext);
	case EMemberTypes::UObject_ObjectFlags:
		return offsetof(UObject, ObjectFlags);
	case EMemberTypes::UObject_HashOuterNext:
		return offsetof(UObject, HashOuterNext);
	case EMemberTypes::UObject_StateFrame:
		return offsetof(UObject, StateFrame);
	case EMemberTypes::UObject_Linker:
		return offsetof(UObject, Linker);
	case EMemberTypes::UObject_LinkerIndex:
		return offsetof(UObject, LinkerIndex);
	case EMemberTypes::UObject_Integer:
		return offsetof(UObject, ObjectInternalInteger);
	case EMemberTypes::UObject_NetIndex:
		return offsetof(UObject, NetIndex);
	case EMemberTypes::UObject_Outer:
		return offsetof(UObject, Outer);
	case EMemberTypes::UObject_Name:
		return offsetof(UObject, Name);
	case EMemberTypes::UObject_Class:
		return offsetof(UObject, Class);
	case EMemberTypes::UObject_Archetype:
		return offsetof(UObject, ObjectArchetype);
	case EMemberTypes::UField_Next:
		return offsetof(UField, Next);
	case EMemberTypes::UField_UniqueFeatureValue:
		return offsetof(UField, UniqueFeatureValue);
	//case EMemberTypes::UField_SuperField:
	//	return offsetof(UField, SuperField);
	case EMemberTypes::UEnum_Names:
		return offsetof(UEnum, Names);
	case EMemberTypes::UConst_Value:
		return offsetof(UConst, Value);
	case EMemberTypes::UProperty_ArrayDim:
		return offsetof(UProperty, ArraySize);
	case EMemberTypes::UProperty_PropertySize:
		return offsetof(UProperty, PropertySize);
	case EMemberTypes::UProperty_PropertyFlags:
		return offsetof(UProperty, PropertyFlags);
	case EMemberTypes::UProperty_RepOffset:
		return offsetof(UProperty, RepOffset);
	case EMemberTypes::UProperty_RepIndex:
		return offsetof(UProperty, RepIndex);
	case EMemberTypes::UProperty_Category:
		return offsetof(UProperty, Category);
	case EMemberTypes::UProperty_ArraySizeEnum:
		return offsetof(UProperty, ArraySizeEnum);
	case EMemberTypes::UProperty_Offset:
		return offsetof(UProperty, Offset);
	case EMemberTypes::UProperty_PropertyLinkNext:
		return offsetof(UProperty, PropertyLinkNext);
	case EMemberTypes::UProperty_ConstructorLinkNext:
		return offsetof(UProperty, ConstructorLinkNext);
	case EMemberTypes::UProperty_NextRef:
		return offsetof(UProperty, NextRef);
	case EMemberTypes::UProperty_Alias:
		return offsetof(UProperty, Alias);
	case EMemberTypes::UStruct_SuperField:
		return offsetof(UStruct, SuperField);
	case EMemberTypes::UStruct_Children:
		return offsetof(UStruct, Children);
	case EMemberTypes::UStruct_Size:
		return offsetof(UStruct, PropertySize);
	case EMemberTypes::UStruct_Alignment:
		return offsetof(UStruct, MinAlignment);
	case EMemberTypes::UFunction_Flags:
		return offsetof(UFunction, FunctionFlags);
	case EMemberTypes::UFunction_Native:
		return offsetof(UFunction, iNative);
	case EMemberTypes::UFunction_RepOffset:
		return offsetof(UFunction, RepOffset);
	case EMemberTypes::UFunction_FriendlyName:
		return offsetof(UFunction, FriendlyName);
	case EMemberTypes::UFunction_OperPrecedence:
		return offsetof(UFunction, OperPrecedence);
	case EMemberTypes::UFunction_NumParms:
		return offsetof(UFunction, NumParms);
	case EMemberTypes::UFunction_ParmsSize:
		return offsetof(UFunction, ParmsSize);
	case EMemberTypes::UFunction_ReturnValueOffset:
		return offsetof(UFunction, ReturnValueOffset);
	case EMemberTypes::UFunction_Func:
		return offsetof(UFunction, Func);
	case EMemberTypes::UStructProperty_Struct:
		return offsetof(UStructProperty, Struct);
	case EMemberTypes::UObjectProperty_Class:
		return offsetof(UObjectProperty, PropertyClass);
	case EMemberTypes::UClassProperty_Meta:
		return offsetof(UClassProperty, MetaClass);
	case EMemberTypes::UMapProperty_Key:
		return offsetof(UMapProperty, Key);
	case EMemberTypes::UMapProperty_Value:
		return offsetof(UMapProperty, Value);
	case EMemberTypes::UInterfaceProperty_Class:
		return offsetof(UInterfaceProperty, InterfaceClass);
	case EMemberTypes::UByteProperty_Enum:
		return offsetof(UByteProperty, Enum);
	case EMemberTypes::UBoolProperty_BitMask:
		return offsetof(UBoolProperty, BitMask);
	case EMemberTypes::UArrayProperty_Inner:
		return offsetof(UArrayProperty, Inner);
	case EMemberTypes::UClass_ClassFlags:
		return offsetof(UClass, ClassFlags);
	case EMemberTypes::UClass_ClassUnique:
		return offsetof(UClass, ClassUnique);
	case EMemberTypes::UClass_ClassDelegates:
		return offsetof(UClass, ClassDelegates);
	case EMemberTypes::UClass_ClassCastFlags:
		return offsetof(UClass, ClassCastFlags);
	case EMemberTypes::UClass_ClassWithin:
		return offsetof(UClass, ClassWithin);
	case EMemberTypes::UClass_Category:
		return offsetof(UClass, Category);
	case EMemberTypes::UClass_ClassReps:
		return offsetof(UClass, ClassReps);
	case EMemberTypes::UClass_ManualPad:
		return offsetof(UClass, ManualPad);
	case EMemberTypes::UClass_NetFields:
		return offsetof(UClass, NetFields);
	case EMemberTypes::UClass_HideCategories:
		return offsetof(UClass, HideCategories);
	case EMemberTypes::UClass_AutoExpandCategories:
		return offsetof(UClass, AutoExpandCategories);
	case EMemberTypes::UClass_AutoCollapseCategories:
		return offsetof(UClass, AutoCollapseCategories);
	case EMemberTypes::UClass_DontSortCategories:
		return offsetof(UClass, DontSortCategories);
	case EMemberTypes::UClass_DependentOn:
		return offsetof(UClass, DependentOn);
	case EMemberTypes::UClass_ClassGroupNames:
		return offsetof(UClass, ClassGroupNames);
	case EMemberTypes::UClass_bForceScriptOrder:
		return offsetof(UClass, bForceScriptOrder);
	case EMemberTypes::UClass_ClassHeaderFilename:
		return offsetof(UClass, ClassHeaderFilename);
	case EMemberTypes::UClass_ClassDefaultObject:
		return offsetof(UClass, ClassDefaultObject);
	case EMemberTypes::UClass_ClassConstructor:
		return offsetof(UClass, ClassConstructor);
	case EMemberTypes::UClass_ClassStaticConstructor:
		return offsetof(UClass, ClassStaticConstructor);
	case EMemberTypes::UClass_ClassStaticInitializer:
		return offsetof(UClass, ClassStaticInitializer);
	case EMemberTypes::UClass_ComponentNameToDefaultObjectMap:
		return offsetof(UClass, ComponentNameToDefaultObjectMap);
	case EMemberTypes::UClass_Interfaces:
		return offsetof(UClass, Interfaces);
	case EMemberTypes::UClass_DefaultPropText:
		return offsetof(UClass, DefaultPropText);
	case EMemberTypes::UClass_bNeedsPropertiesLinked:
		return offsetof(UClass, bNeedsPropertiesLinked);
	case EMemberTypes::UClass_ReferenceTokenStream:
		return offsetof(UClass, ReferenceTokenStream);
	case EMemberTypes::UClass_ClassStates:
		return offsetof(UClass, ClassStates);
	case EMemberTypes::UDelegateProperty_SuccessCallback:
		return offsetof(UDelegateProperty, SuccessCallback);
	case EMemberTypes::UDelegateProperty_FailureCallback:
		return offsetof(UDelegateProperty, FailureCallback);

	default:
		return 0;
	}
}

size_t Member::GetClassSize(EClassTypes type)
{
	switch (type)
	{
	case EClassTypes::FNameEntry:
		return sizeof(FNameEntry);
	case EClassTypes::UClass:
		return sizeof(UClass);
	case EClassTypes::UObject:
		return sizeof(UObject);
	case EClassTypes::UField:
		return sizeof(UField);
	case EClassTypes::UEnum:
		return sizeof(UEnum);
	case EClassTypes::UConst:
		return sizeof(UConst);
	case EClassTypes::UProperty:
		return  sizeof(UProperty);
	case EClassTypes::UStruct:
		return sizeof(UStruct);
	case EClassTypes::UFunction:
		return sizeof(UFunction);
	case EClassTypes::UStructProperty:
		return sizeof(UStructProperty);
	case EClassTypes::UDelegateProperty:
		return sizeof(UDelegateProperty);
	case EClassTypes::UObjectProperty:
		return sizeof(UObjectProperty);
	case EClassTypes::UClassProperty:
		return sizeof(UClassProperty);
	case EClassTypes::UMapProperty:
		return sizeof(UMapProperty);
	case EClassTypes::UInterfaceProperty:
		return sizeof(UInterfaceProperty);
	case EClassTypes::UByteProperty:
		return sizeof(UByteProperty);
	case EClassTypes::UBoolProperty:
		return sizeof(UBoolProperty);
	case EClassTypes::UArrayProperty:
		return sizeof(UArrayProperty);
	default:
		return 0;
	}
}

size_t Member::GetClassOffset(EClassTypes type)
{
	switch (type)
	{
	case EClassTypes::FNameEntry:
		return 0;
	case EClassTypes::UObject:
		return 0;
	case EClassTypes::UField:
		return sizeof(UObject);
	case EClassTypes::UEnum:
		return sizeof(UField);
	case EClassTypes::UClass:
		return sizeof(UState);
	case EClassTypes::UConst:
		return sizeof(UField);
	case EClassTypes::UProperty:
		return sizeof(UField);
	case EClassTypes::UStruct:
		return sizeof(UField);
	case EClassTypes::UFunction:
		return sizeof(UStruct);
	case EClassTypes::UStructProperty:
		return sizeof(UProperty);
	case EClassTypes::UObjectProperty:
		return sizeof(UProperty);
	case EClassTypes::UClassProperty:
		return sizeof(UObjectProperty);
	case EClassTypes::UDelegateProperty:
		return sizeof(UProperty);
	case EClassTypes::UMapProperty:
		return sizeof(UProperty);
	case EClassTypes::UInterfaceProperty:
		return sizeof(UProperty);
	case EClassTypes::UByteProperty:
		return sizeof(UProperty);
	case EClassTypes::UBoolProperty:
		return sizeof(UProperty);
	case EClassTypes::UArrayProperty:
		return sizeof(UProperty);
	default:
		return 0;
	}
}

void Member::Register(EMemberTypes type, size_t size)
{
	m_registeredMembers[type] = Member(type, size);
}

std::map<size_t, Member*> Member::GetRegistered(EClassTypes type)
{
	std::map<size_t, Member*> members;

	if (m_classMembers.contains(type))
	{
		for (EMemberTypes member : m_classMembers[type])
		{
			AddRegistered(members, member);
		}
	}

	return members;
}

void Member::AddRegistered(std::map<size_t, Member*>& members, EMemberTypes type)
{
	if (m_registeredMembers.contains(type))
	{
		members[m_registeredMembers[type].Offset] = &m_registeredMembers[type];
	}
}

std::map<EClassTypes, std::vector<EMemberTypes>> Member::m_classMembers = {
	// Core Objects

	{ EClassTypes::FNameEntry, {
		EMemberTypes::FNameEntry_HashNext,
		EMemberTypes::FNameEntry_Index,
		EMemberTypes::FNameEntry_Flags,
		EMemberTypes::FNameEntry_Name
	} },

	{ EClassTypes::UObject, {
		EMemberTypes::UObject_VfTable,
		EMemberTypes::UObject_Integer,
		EMemberTypes::UObject_HashNext,
		EMemberTypes::UObject_ObjectFlags,
		EMemberTypes::UObject_HashOuterNext,
		EMemberTypes::UObject_StateFrame,
		EMemberTypes::UObject_Linker,
		EMemberTypes::UObject_LinkerIndex,
		EMemberTypes::UObject_NetIndex,
		EMemberTypes::UObject_Outer,
		EMemberTypes::UObject_Name,
		EMemberTypes::UObject_Class,
		EMemberTypes::UObject_Archetype
	} },

	{ EClassTypes::UClass, {
		EMemberTypes::UClass_ClassFlags,
		EMemberTypes::UClass_ClassUnique,
		EMemberTypes::UClass_ClassDelegates,
		EMemberTypes::UClass_ClassCastFlags,
		EMemberTypes::UClass_ClassWithin,
		EMemberTypes::UClass_Category,
		EMemberTypes::UClass_ClassReps,
		EMemberTypes::UClass_NetFields,
		EMemberTypes::UClass_HideCategories,
		EMemberTypes::UClass_AutoExpandCategories,
		EMemberTypes::UClass_AutoCollapseCategories,
		EMemberTypes::UClass_DontSortCategories,
		EMemberTypes::UClass_DependentOn,
		EMemberTypes::UClass_ClassGroupNames,
		EMemberTypes::UClass_bForceScriptOrder,
		EMemberTypes::UClass_ClassHeaderFilename,
		EMemberTypes::UClass_ManualPad,
		EMemberTypes::UClass_ClassDefaultObject,
		EMemberTypes::UClass_ClassConstructor,
		EMemberTypes::UClass_ClassStaticConstructor,
		EMemberTypes::UClass_ClassStaticInitializer,
		EMemberTypes::UClass_ComponentNameToDefaultObjectMap,
		EMemberTypes::UClass_Interfaces,
		EMemberTypes::UClass_DefaultPropText,
		EMemberTypes::UClass_bNeedsPropertiesLinked,
		EMemberTypes::UClass_ReferenceTokenStream,
		EMemberTypes::UClass_ClassStates,
	} },


	{ EClassTypes::UField, {
		EMemberTypes::UField_Next,
		EMemberTypes::UField_UniqueFeatureValue
	} },

	{ EClassTypes::UEnum, {
		EMemberTypes::UEnum_Names
	} },

	{ EClassTypes::UConst, {
		EMemberTypes::UConst_Value
	} },

	{ EClassTypes::UProperty, {
		EMemberTypes::UProperty_ArrayDim,
	EMemberTypes::UProperty_PropertySize,
	EMemberTypes::UProperty_PropertyFlags,
	EMemberTypes::UProperty_RepOffset,
	EMemberTypes::UProperty_RepIndex,
	EMemberTypes::UProperty_Category,
	EMemberTypes::UProperty_ArraySizeEnum,
	EMemberTypes::UProperty_Offset,
	EMemberTypes::UProperty_PropertyLinkNext,
	EMemberTypes::UProperty_ConstructorLinkNext,
	EMemberTypes::UProperty_NextRef,
	EMemberTypes::UProperty_Alias,
	} },

	{ EClassTypes::UStruct, {
		EMemberTypes::UStruct_SuperField,
		EMemberTypes::UStruct_Children,
		EMemberTypes::UStruct_Size,
		EMemberTypes::UStruct_Alignment
	} },

	{ EClassTypes::UFunction, {
		EMemberTypes::UFunction_Flags,
		EMemberTypes::UFunction_Native,
		EMemberTypes::UFunction_RepOffset,
		EMemberTypes::UFunction_FriendlyName,
		EMemberTypes::UFunction_OperPrecedence,
		EMemberTypes::UFunction_NumParms,
		EMemberTypes::UFunction_ParmsSize,
		EMemberTypes::UFunction_ReturnValueOffset,
		EMemberTypes::UFunction_Func
	} },

	{ EClassTypes::UDelegateProperty, {
	EMemberTypes::UDelegateProperty_SuccessCallback,
	EMemberTypes::UDelegateProperty_FailureCallback
	} },

	// Property Objects

	{ EClassTypes::UStructProperty, {
		EMemberTypes::UStructProperty_Struct
	} },

	{ EClassTypes::UObjectProperty, {
		EMemberTypes::UObjectProperty_Class
	} },

	{ EClassTypes::UClassProperty, {
		EMemberTypes::UClassProperty_Meta
	} },

	{ EClassTypes::UMapProperty, {
		EMemberTypes::UMapProperty_Key,
		EMemberTypes::UMapProperty_Value,
	} },

	{ EClassTypes::UInterfaceProperty, {
		EMemberTypes::UInterfaceProperty_Class
	} },

	{ EClassTypes::UByteProperty, {
		EMemberTypes::UByteProperty_Enum
	} },

	{ EClassTypes::UBoolProperty, {
		EMemberTypes::UBoolProperty_BitMask
	} },

	{ EClassTypes::UArrayProperty, {
		EMemberTypes::UArrayProperty_Inner
	} },
};

Member& Member::operator=(const Member& member)
{
	Type = member.Type;
	Label = member.Label;
	Offset = member.Offset;
	Size = member.Size;
	return *this;
}

/*
# ========================================================================================= #
#
# ========================================================================================= #
*/