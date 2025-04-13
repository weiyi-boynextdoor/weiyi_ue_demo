// Fill out your copyright notice in the Description page of Project Settings.


#include "MyAudioComponent.h"
#include "MySoundWave.h"

void UMyAudioComponent::Play(float StartTime)
{
	Super::Play(StartTime);
	if (UMySoundWave* MySoundWave = Cast<UMySoundWave>(Sound))
	{
		MySoundWave->Seek(0);
	}
}
