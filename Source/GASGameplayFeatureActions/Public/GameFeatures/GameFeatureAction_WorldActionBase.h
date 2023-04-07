// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFeatureAction.h"
#include "GameFeatureAction_WorldActionBase.generated.h"

UCLASS(Abstract)
class UGameFeatureAction_WorldActionBase : public UGameFeatureAction
{
	GENERATED_BODY()

public:
	virtual void OnGameFeatureActivating() override;
	virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;

private:
	void HandleGameInstanceStart(UGameInstance* GameInstance);

	virtual void AddToWorld(const FWorldContext& WorldContext) PURE_VIRTUAL(UGameFeatureAction_WorldActionBase::AddToWorld,);

	FDelegateHandle GameInstanceStartHandle;
};
