// Fill out your copyright notice in the Description page of Project Settings.


#include "GameFeatures/GameFeatureAction_AddAbilities.h"

#include "Abilities/GameplayAbility.h"
#include "Input/AbilityInputBindingComponent.h"
#include "AttributeSets/GFAttributeset.h"
#include "AbilitySystemComponent.h"
#include "AssetRegistry/AssetBundleData.h"
#include "AttributeSet.h"
#include "ComponentInstanceDataCache.h"
#include "Components/ActorComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Containers/UnrealString.h"
#include "Delegates/Delegate.h"
#include "Engine/AssetManager.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFeaturesSubsystemSettings.h"
#include "GameFramework/Actor.h"
#include "HAL/PlatformCrt.h"
#include "InputAction.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Text.h"
#include "Logging/LogCategory.h"
#include "Logging/LogMacros.h"
#include "Misc/AssertionMacros.h"
#include "Templates/ChooseClass.h"
#include "Templates/SubclassOf.h"
#include "Trace/Detail/Channel.h"
#include "UObject/Object.h"
#include "UObject/ObjectPtr.h"
#include "UObject/SoftObjectPath.h"
#include "AbilitySystemComponent.h"
#include "GameFeaturesSubsystem.h"
#include "AttributeSet.h"
#include "GameFeaturesSubsystemSettings.h"
#include "Internationalization/Internationalization.h"
#include "GameplayAbilitySpec.h"
#include "GASGameplayFeatureActions.h"
#include "Engine/AssetManager.h"
#include "Input/AbilityInputBindingComponent.h"

#define LOCTEXT_NAMESPACE "GASGameplayFeatureActions"

void UGameFeatureAction_AddAbilities::OnGameFeatureActivating()
{
	if (ensureAlways(ActiveExtensions.IsEmpty()) || ensureAlways(ComponentRequests.IsEmpty()))
	{
		Reset();
	}
	
	Super::OnGameFeatureActivating();
}

void UGameFeatureAction_AddAbilities::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	Super::OnGameFeatureDeactivating(Context);
	Reset();
}

void UGameFeatureAction_AddAbilities::AddAdditionalAssetBundleData(FAssetBundleData& AssetBundleData)
{
	if (UAssetManager::IsValid())
	{
		auto AddBundleAsset = [&AssetBundleData](const FTopLevelAssetPath& Path)
		{
			AssetBundleData.AddBundleAsset(UGameFeaturesSubsystemSettings::LoadStateClient, Path);
			AssetBundleData.AddBundleAsset(UGameFeaturesSubsystemSettings::LoadStateServer, Path);
		};

		for (const FGameFeatureAbilitiesEntry& GameFeatureAbilitiesEntry : AbilitiesList)
		{
			for (const auto  Ability : GameFeatureAbilitiesEntry.Abilities)
			{
				AddBundleAsset(Ability.AbilityType.ToSoftObjectPath().GetAssetPath());
				if (!Ability.AbilityType.IsNull())
				{
					AddBundleAsset(Ability.AbilityType.ToSoftObjectPath().GetAssetPath());
				}
			}
			for (auto& Attributes : GameFeatureAbilitiesEntry.Attributes)
			{
				AddBundleAsset(Attributes.AttributeSetType.ToSoftObjectPath().GetAssetPath());
				if (!Attributes.InitializationData.IsNull())
				{
					AddBundleAsset(Attributes.InitializationData.ToSoftObjectPath().GetAssetPath());
				}
			}
		}
	}
}

/**
 * Validate the data for this action, returning a result indicating whether the data is valid or not.
 *
 *  @param ValidationErrors		An array of validation errors that will be appended to if the data is invalid.
 *
 *  @return EDataValidationResult::Valid if the data is valid, EDataValidationResult::Invalid otherwise.
 */
EDataValidationResult UGameFeatureAction_AddAbilities::IsDataValid(TArray<FText>& ValidationErrors)
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(ValidationErrors), EDataValidationResult::Valid);

	int32 EntryIndex = 0;
	for (const auto& Entry : AbilitiesList)
	{
		if (Entry.ActorClass.IsNull())
		{
			Result = EDataValidationResult::Invalid;
			ValidationErrors.Add(FText::Format(
				LOCTEXT("EntryHasNullActor", "Null ActorClass at index {0} in AbilitiesList"),
				FText::AsNumber(EntryIndex)));
		}

		if (Entry.Abilities.IsEmpty() && Entry.Attributes.IsEmpty())
		{
			Result = EDataValidationResult::Invalid;
			ValidationErrors.Add(FText::Format(
				LOCTEXT("EntryHasNoAbilitiesOrAttributes", "Entry at index {0} in AbilitiesList has no Abilities or Attributes"),
				FText::AsNumber(EntryIndex)));
		}

		int32 AbilityIndex = 0;
		for (const auto& Ability : Entry.Abilities)
		{
			if (Ability.AbilityType.IsNull())
			{
				Result = EDataValidationResult::Invalid;
				ValidationErrors.Add(FText::Format(
					LOCTEXT("EntryHasNullAbility", "Null AbilityType at index {0} in AbilitiesList[{1}].Abilities"),
					FText::AsNumber(AbilityIndex),
					FText::AsNumber(EntryIndex)));
			}
			++AbilityIndex;
		}

		int32 AttributeIndex = 0;
		for (const auto& Attribute : Entry.Attributes)
		{
			if (Attribute.AttributeSetType.IsNull())
			{
				Result = EDataValidationResult::Invalid;
				ValidationErrors.Add(FText::Format(
					LOCTEXT("EntryHasNullAttribute", "Null AttributeSetType at index {0} in AbilitiesList[{1}].Attributes"),
					FText::AsNumber(AttributeIndex),
					FText::AsNumber(EntryIndex)));
			}
			++AttributeIndex;
		}
	}
	return Result;
}

void UGameFeatureAction_AddAbilities::AddToWorld(const FWorldContext& WorldContext)
{
	UWorld* World = WorldContext.World();
	UGameInstance* GameInstance = WorldContext.OwningGameInstance;

	if ((GameInstance != nullptr) && (World != nullptr) && World->IsGameWorld())
	{
		UGameFrameworkComponentManager* ComponentManager = UGameInstance::GetSubsystem<UGameFrameworkComponentManager>(GameInstance);
		if (ComponentManager != nullptr)
		{
			int32 EntryIndex = 0;
			for (const auto&  Entry : AbilitiesList)
			{
				UGameFrameworkComponentManager::FExtensionHandlerDelegate AddAbilitesDelegate =
					UGameFrameworkComponentManager::FExtensionHandlerDelegate::CreateUObject(
						this, &UGameFeatureAction_AddAbilities::HandleActorExtension, EntryIndex);

				TSharedPtr<FComponentRequestHandle> ExtensionRequestHandle = ComponentManager->AddExtensionHandler(
					Entry.ActorClass, AddAbilitesDelegate);

				ComponentRequests.Add(ExtensionRequestHandle);
			}
		}
	}
}

void UGameFeatureAction_AddAbilities::Reset()
{
	while (!ActiveExtensions.IsEmpty())
	{
		auto ExtensionIT = ActiveExtensions.CreateIterator();
		RemoveActorAbilities(ExtensionIT->Key);
	}
	ComponentRequests.Empty();
}

void UGameFeatureAction_AddAbilities::HandleActorExtension(AActor* Actor, FName EventName, int32 EntryIndex)
{
	if (AbilitiesList.IsValidIndex(EntryIndex))
	{
		const FGameFeatureAbilitiesEntry& Entry = AbilitiesList[EntryIndex];
		if (EventName == UGameFrameworkComponentManager::NAME_ExtensionRemoved || EventName == UGameFrameworkComponentManager::NAME_ReceiverRemoved)
		{
			RemoveActorAbilities(Actor);
		}
		else if (EventName == UGameFrameworkComponentManager::NAME_ExtensionAdded || EventName == UGameFrameworkComponentManager::NAME_GameActorReady)
		{
			AddActorAbilities(Actor, Entry);
		}
	}
}

void UGameFeatureAction_AddAbilities::AddActorAbilities(AActor* Actor, const FGameFeatureAbilitiesEntry& Entry)
{
	if (UAbilitySystemComponent* AbilitySystemComponent = FindOrAddComponentForActor<
		UAbilitySystemComponent>(Actor, Entry))
	{
		FActorExtensions AddExtensions;
		AddExtensions.AbilitiesEntries.Reserve(Entry.Abilities.Num());
		AddExtensions.AttributesEntries.Reserve(Entry.Attributes.Num());

		for (const auto& Ability : Entry.Abilities)
		{
			FGameplayAbilitySpec NewAbilitySpec(Ability.AbilityType.LoadSynchronous());
			FGameplayAbilitySpecHandle AbilityHandle = AbilitySystemComponent->GiveAbility(NewAbilitySpec);

			if (!Ability.InputAction.IsNull())
			{
				UAbilityInputBindingComponent* InputComponent = FindOrAddComponentForActor<
					UAbilityInputBindingComponent>(Actor, Entry);
				if (InputComponent)
				{
					InputComponent->SetupInputBinding(Ability.InputAction.LoadSynchronous(), AbilityHandle);
				}
				else
				{
					UE_LOG(LogGASGameplayFeatureActions, Error,
					       TEXT("Failed to find/add an AbilityInputBindingComponent to Actor %s"),
					       *Actor->GetPathName());
				}
			}
			AddExtensions.AbilitiesEntries.Add(AbilityHandle);
		}

		for (const auto& Attributes : Entry.Attributes)
		{
			if (!Attributes.AttributeSetType.IsNull())
			{
				TSubclassOf<UGFAttributeSet> SetType = Attributes.AttributeSetType.LoadSynchronous();
				if (SetType)
				{
					UAttributeSet* NewSet = NewObject<UAttributeSet>(Actor, SetType);
					if (!Attributes.InitializationData.IsNull())
					{
						UDataTable* InitialData = Attributes.InitializationData.LoadSynchronous();
						if (InitialData)
						{
							NewSet->InitFromMetaDataTable(InitialData);
						}
					}
					AddExtensions.AttributesEntries.Add(NewSet);
					AbilitySystemComponent->AddAttributeSetSubobject(NewSet);
				}
			}
		}
		ActiveExtensions.Add(Actor, AddExtensions);
	}
	else
	{
		UE_LOG(LogGASGameplayFeatureActions, Error,
		       TEXT("Failed to find/add an AbilitySystemComponent to Actor %s"),
		       *Actor->GetPathName());
	}
}

void UGameFeatureAction_AddAbilities::RemoveActorAbilities(AActor* Actor)
{
	if (FActorExtensions* ActorExtensions = ActiveExtensions.Find(Actor))
	{
		if (UAbilitySystemComponent* AbilitySystemComponent = Actor->FindComponentByClass<UAbilitySystemComponent>())
		{
			for (UAttributeSet* AttribSetInstance : ActorExtensions->AttributesEntries)
			{
				AbilitySystemComponent->RemoveSpawnedAttribute(AttribSetInstance);
			}

			UAbilityInputBindingComponent* InputComponent = Actor->FindComponentByClass<UAbilityInputBindingComponent>();
			for (FGameplayAbilitySpecHandle AbilityHandle : ActorExtensions->AbilitiesEntries)
			{
				if (InputComponent)
				{
					InputComponent->ClearInputBinding(AbilityHandle);
				}
				AbilitySystemComponent->SetRemoveAbilityOnEnd(AbilityHandle);
			}
		}

		ActiveExtensions.Remove(Actor);
	}
}

UActorComponent* UGameFeatureAction_AddAbilities::FindOrAddComponentForActor(UClass* ComponentType, AActor* Actor,
	const FGameFeatureAbilitiesEntry& Entry)
{
	 UActorComponent* Component = Actor->FindComponentByClass(ComponentType);

	bool bMakeComponentRequest = Component == nullptr;

	 if (Component)
	 {
	 	/**
	 	 * Check to see if this Component was created from a different UGameFrameworkComponentManager request
	 	 * Native is what CreationMethod defaults to for dynamically added components
	 	 */
		 if (Component->CreationMethod == EComponentCreationMethod::Native)
		 {
			// Attempt to tell the difference between a true native component and one created by the GameFrameworkComponent system.
			// If it is from the UGameFrameworkComponentManager, then we need to make another request (requests are ref counted).
			 UObject* ComponentArchetype = Component->GetArchetype();
		 	bMakeComponentRequest = ComponentArchetype->HasAnyFlags(RF_ClassDefaultObject);
		 }
	 }

	 if (bMakeComponentRequest)
	 {
		 UWorld* World = Actor->GetWorld();
	 	UGameInstance* GameInstance = World->GetGameInstance();
	 	UGameFrameworkComponentManager* ComponentManager = UGameInstance::GetSubsystem<UGameFrameworkComponentManager>(GameInstance);

		 if (ComponentManager)
		 {
			 const TSharedPtr<FComponentRequestHandle> RequestHandle = ComponentManager->AddComponentRequest(Entry.ActorClass, ComponentType);
		 	ComponentRequests.Add(RequestHandle);
		 }

		 if (!Component)
		 {
			 Component = Actor->FindComponentByClass(ComponentType);
		 	ensureAlways(Component);
		 }
	 }
	
	return Component;
}
