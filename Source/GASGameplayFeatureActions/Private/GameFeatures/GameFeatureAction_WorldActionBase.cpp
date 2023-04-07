// Fill out your copyright notice in the Description page of Project Settings.


#include "GameFeatures/GameFeatureAction_WorldActionBase.h"

void UGameFeatureAction_WorldActionBase::OnGameFeatureActivating()
{
	GameInstanceStartHandle = FWorldDelegates::OnStartGameInstance.AddUObject(this, &UGameFeatureAction_WorldActionBase::HandleGameInstanceStart);

	// Add to any worlds with associated game instances that have already been initialized
	for (const auto WorldContext : GEngine->GetWorldContexts())
	{
		AddToWorld(WorldContext);
	}
}

void UGameFeatureAction_WorldActionBase::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	FWorldDelegates::OnStartGameInstance.Remove(GameInstanceStartHandle);
}

void UGameFeatureAction_WorldActionBase::HandleGameInstanceStart(UGameInstance* GameInstance)
{
	if (const auto WorldContext = GameInstance->GetWorldContext())
	{
		AddToWorld(*WorldContext);
	}
}
