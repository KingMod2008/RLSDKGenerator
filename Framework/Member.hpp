#pragma once
#include <map>
#include <vector>
#include <string>

// Class Types
enum class EClassTypes : uint8_t
{
	Unknown,
	FNameEntry,
	UObject,
	UClass,
	UField,
	UEnum,
	UConst,
	UProperty,
	UStruct,
	UFunction,
	UStructProperty,
	UObjectProperty,
	UClassProperty,
	UMapProperty,
	UInterfaceProperty,
	UByteProperty,
	UBoolProperty,
	UArrayProperty,
	UDelegateProperty,
	UChannel
};

// Member Types
enum class EMemberTypes : uint8_t
{
	Unknown,
	FNameEntry_Index,
	FNameEntry_Flags,
	FNameEntry_HashNext,
	FNameEntry_Name,
	UObject_VfTable,
	UObject_HashNext,
	UObject_ObjectFlags,
	UObject_HashOuterNext,
	UObject_StateFrame,
	UObject_Linker,
	UObject_LinkerIndex,
	UObject_Integer,
	UObject_NetIndex,
	UObject_Outer,
	UObject_Name,
	UObject_Class,
	UObject_Archetype,
	UField_Next,
	UField_UniqueFeatureValue,
	UField_SuperField, // Not needed if SuperField is in the UStruct class, leave "SUPERFIELDS_IN_UFIELD" in your "GameDefines.hpp" file!
	UEnum_Names,
	UConst_Value,
	UProperty_ArrayDim,
	UProperty_PropertySize,
	UProperty_PropertyFlags,
	UProperty_RepOffset,
	UProperty_RepIndex,
	UProperty_Category, 
	UProperty_ArraySizeEnum,
	UProperty_Offset,
	UProperty_PropertyLinkNext,
	UProperty_ConstructorLinkNext,
	UProperty_NextRef,
	UProperty_Alias,
	UStruct_SuperField, // Not needed if SuperField is in the UField class, comment out "SUPERFIELDS_IN_UFIELD" in your "GameDefines.hpp" file!
	UStruct_Children,
	UStruct_Size,
	UStruct_Alignment,
	UFunction_Flags,
	UFunction_Native,
	UFunction_RepOffset,
	UFunction_FriendlyName,
	UFunction_OperPrecedence,
	UFunction_NumParms,
	UFunction_ParmsSize,
	UFunction_ReturnValueOffset,
	UFunction_Func,
	UStructProperty_Struct,
	UObjectProperty_Class,
	UClassProperty_Meta,
	UMapProperty_Key,
	UMapProperty_Value,
	UInterfaceProperty_Class,
	UByteProperty_Enum,
	UBoolProperty_BitMask,
	UArrayProperty_Inner,
	UChannel_Connection,
	UChannel_OpenAcked,
	UChannel_Closing,
	UChannel_OpenTemporary,
	UChannel_Broken,
	UChannel_bTornOff,
	UChannel_ChIndex,
	UChannel_OpenedLocally,
	UChannel_OpenPacketId,
	UChannel_ChType,
	UChannel_NumInRec,
	UChannel_NumOutRec,
	UChannel_NegotiatedVer,
	UChannel_InRec,
	UChannel_OutRec,
	UChannel_LastUnqueueTime,
	UClass_ClassFlags,
	UClass_ClassUnique,
	UClass_ClassDelegates,
	UClass_ClassCastFlags,
	UClass_ClassWithin,
	UClass_Category,
	UClass_ClassReps,
	UClass_NetFields,
	UClass_HideCategories,
	UClass_AutoExpandCategories,
	UClass_AutoCollapseCategories,
	UClass_DontSortCategories,
	UClass_DependentOn,
	UClass_ClassGroupNames,
	UClass_bForceScriptOrder,
	UClass_ClassHeaderFilename,
	UClass_ManualPad,
	UClass_ClassDefaultObject,
	UClass_ClassConstructor,
	UClass_ClassStaticConstructor,
	UClass_ClassStaticInitializer,
	UClass_ComponentNameToDefaultObjectMap,
	UClass_Interfaces,
	UClass_DefaultPropText,
	UClass_bNeedsPropertiesLinked,
	UClass_ReferenceTokenStream,
	UClass_ClassStates,
	UDelegateProperty_SuccessCallback,
	UDelegateProperty_FailureCallback
};

// Property Types
enum class EPropertyTypes : uint8_t
{
	Unknown,
	Int32,
	UInt8,
	UInt32,
	UInt64,
	Double,
	Float,
	Bool,
	FName,
	FString,
	FScriptDelegate,
	FStruct,
	UObject,
	UClass,
	UInterface,
	TArray,
	TMap
};

/*
# ========================================================================================= #
# Members
# ========================================================================================= #
*/

class Member
{
public:
	EMemberTypes Type;		// Internal id used to tell which member this class is.
	std::string Label;		// String label used for printing the member inside the class.
	size_t Offset;			// Member offset from the base of the class.
	size_t Size;			// Size of the member at the given offset.

public:
	Member();
	Member(EMemberTypes type, size_t size);
	Member(const Member& member);
	~Member();

public: // Global Utils
	static std::string GetName(EClassTypes type);		// Returns the string name of the enum used for logging and messageboxes.
	static std::string GetLabel(EMemberTypes type);		// Returns the string version of the member used for printing in the generated sdk.
	static uintptr_t GetOffset(EMemberTypes type);		// Returns the members offset in its defined class.
	static size_t GetClassSize(EClassTypes type);		// Returns the "sizeof" for the given class type.
	static size_t GetClassOffset(EClassTypes type);		// Returns the start offset of the given type, taking into account its inherited classes.

public:
	static void Register(EMemberTypes type, size_t size);				// This should only be called by the "REGISTER_MEMBER" macro!
	static std::map<size_t, Member*> GetRegistered(EClassTypes type);	// Returns registered members for the given class type, sorted by their offsets.
	template<typename T>
	static void Register(EMemberTypes type)
	{
		Register(type, sizeof(T));
	}
private:
	static void AddRegistered(std::map<size_t, Member*>& members, EMemberTypes type);
	static std::map<EClassTypes, std::vector<EMemberTypes>> m_classMembers;
	static inline std::map<EMemberTypes, Member> m_registeredMembers;

public:
	Member& operator=(const Member& member);
};

#define REGISTER_MEMBER(memberVariable, memberName, memberType) static void Register_##memberName(){ Member::Register(memberType, sizeof(memberVariable)); }

/*
# ========================================================================================= #
#
# ========================================================================================= #
*/