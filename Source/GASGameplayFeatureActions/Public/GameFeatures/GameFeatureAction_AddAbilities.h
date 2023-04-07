// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFeatureAction.h"
#include "GameFeatureAction_WorldActionBase.h"
#include "CoreTypes.h"
#include "GameplayAbilitySpec.h"

#include "GameFeatureAction_AddAbilities.generated.h"

class AActor;
class FText;
class UActorComponent;
class UGFAttributeSet;
class UAttributeSet;
class UClass;
class UDataTable;
class UGameplayAbility;
class UInputAction;
class UObject;
struct FAssetBundleData;
struct FComponentRequestHandle;
struct FGameFeatureDeactivatingContext;
struct FWorldContext;

USTRUCT(BlueprintType)
struct FGameAbilityMapping 
{
	GENERATED_BODY()

	// What Type of Ability to grant
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftClassPtr<UGameplayAbility> AbilityType;

	// InputAction to bind the Ability to, if any (can be left unset)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UInputAction> InputAction;
};

USTRUCT(BlueprintType)
struct FAttributesMapping 
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftClassPtr<UGFAttributeSet> AttributeSetType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UDataTable> InitializationData;
};

USTRUCT()
struct FGameFeatureAbilitiesEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Abilities")
	TSoftClassPtr<AActor> ActorClass;

	UPROPERTY(EditAnywhere, Category="Abilities")
	TArray<FGameAbilityMapping> Abilities;

	UPROPERTY(EditAnywhere, Category="Attributes")
	TArray<FAttributesMapping> Attributes;
};

UCLASS(meta=(DisplayName="Add Abilities"))
class UGameFeatureAction_AddAbilities final : public UGameFeatureAction_WorldActionBase
{
	GENERATED_BODY()

public:
	virtual void OnGameFeatureActivating() override;
	virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;

#if WITH_EDITORONLY_DATA
	virtual void AddAdditionalAssetBundleData(FAssetBundleData& AssetBundleData) override;
#endif

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(TArray<FText>& ValidationErrors) override;
#endif

private:
	// List of abilities to grant to actors of the specified class
	UPROPERTY(EditAnywhere, Category="Abilities", meta=(TitleProperty="ActorClass"))
	TArray<FGameFeatureAbilitiesEntry> AbilitiesList;

	// UGameFeatureAction_WorldActionBase interface
	virtual void AddToWorld(const FWorldContext& WorldContext) override;
	// End of UGameFeatureAction_WorldActionBase interface

	void Reset();
	void HandleActorExtension(AActor* Actor, FName EventName, int32 EntryIndex);
	void AddActorAbilities(AActor* Actor, const FGameFeatureAbilitiesEntry& Entry);
	void RemoveActorAbilities(AActor* Actor);

	template<class ComponentType>
	ComponentType* FindOrAddComponentForActor(AActor* Actor, const FGameFeatureAbilitiesEntry& Entry)
	{
		return Cast<ComponentType>(FindOrAddComponentForActor(ComponentType::StaticClass(), Actor, Entry));
	}

	UActorComponent* FindOrAddComponentForActor(UClass* ComponentType, AActor* Actor, const FGameFeatureAbilitiesEntry& Entry);

	struct FActorExtensions
	{
		TArray<FGameplayAbilitySpecHandle> AbilitiesEntries;
		TArray<UAttributeSet*> AttributesEntries;
	};

	TMap<AActor*, FActorExtensions> ActiveExtensions;

	TArray<TSharedPtr<FComponentRequestHandle>> ComponentRequests;
};
