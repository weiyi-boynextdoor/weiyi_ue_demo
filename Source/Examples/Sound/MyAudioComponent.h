// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/AudioComponent.h"
#include "MyAudioComponent.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = (Audio, Common), HideCategories = (Object, ActorComponent, Physics, Rendering, Mobility, LOD), ShowCategories = Trigger, meta = (BlueprintSpawnableComponent), MinimalAPI)
class UMyAudioComponent : public UAudioComponent
{
	GENERATED_BODY()

public:
	virtual void Play(float StartTime = 0.0f) override;
};
