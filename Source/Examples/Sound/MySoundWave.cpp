// Copyright Epic Games, Inc. All Rights Reserved.

#include "MySoundWave.h"

#include "AudioDevice.h"
#include "Engine/Engine.h"
#include "UObject/AssetRegistryTagsContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MySoundWave)


UMySoundWave::UMySoundWave(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bProcedural = true;
	NumBufferUnderrunSamples = 512;
	NumSamplesToGeneratePerCallback = DEFAULT_PROCEDURAL_SOUNDWAVE_BUFFER_SIZE;
	static_assert(DEFAULT_PROCEDURAL_SOUNDWAVE_BUFFER_SIZE >= 512, "Should generate more samples than this per callback.");
	//checkf(NumSamplesToGeneratePerCallback >= NumBufferUnderrunSamples, TEXT("Should generate more samples than this per callback."));

	// If the main audio device has been set up, we can use this to define our callback size.
	// We need to do this for procedural sound waves that we do not process asynchronously,
	// to ensure that we do not underrun.
	
	SampleByteSize = 2;

}

void UMySoundWave::SetAudio(const uint8* AudioData, const int32 BufferSize)
{
	Audio::EAudioMixerStreamDataFormat::Type Format = GetGeneratedPCMDataFormat();
	SampleByteSize = (Format == Audio::EAudioMixerStreamDataFormat::Int16) ? 2 : 4;

	if (BufferSize == 0 || !ensure((BufferSize % SampleByteSize) == 0))
	{
		return;
	}

	{
		FWriteScopeLock WriteLock(AudioLock);
		if (!AudioBuffer)
		{
			AudioBuffer = MakeShared<TArray<uint8>>();
		}
		AudioBuffer->AddUninitialized(BufferSize);
		FMemory::Memcpy(AudioBuffer->GetData(), AudioData, BufferSize);
	}

	AvailableByteCount.Add(BufferSize);
}

int32 UMySoundWave::GeneratePCMData(uint8* PCMData, const int32 SamplesNeeded)
{
	FReadScopeLock ReadLock(AudioLock);
	if (!AudioBuffer)
	{
		return 0;
	}
	auto& AudioBufferRef = *AudioBuffer;

	Audio::EAudioMixerStreamDataFormat::Type Format = GetGeneratedPCMDataFormat();
	SampleByteSize = (Format == Audio::EAudioMixerStreamDataFormat::Int16) ? 2 : 4;

	int32 SamplesAvailable = AudioBufferRef.Num() / SampleByteSize;
	int32 SamplesToGenerate = FMath::Min(NumSamplesToGeneratePerCallback, SamplesNeeded);

	check(SamplesToGenerate >= NumBufferUnderrunSamples);

	if (SamplesAvailable < SamplesToGenerate)
	{
		// First try to use the virtual function which assumes we're writing directly into our audio buffer
		// since we're calling from the audio render thread.
		int32 NumSamplesGenerated = OnGeneratePCMAudio(AudioBufferRef, SamplesToGenerate);
		if (NumSamplesGenerated > 0)
		{
			// Shrink the audio buffer size to the actual number of samples generated
			const int32 BytesGenerated = NumSamplesGenerated * SampleByteSize;
			ensureAlwaysMsgf(BytesGenerated <= AudioBufferRef.Num(), TEXT("MySoundwave generated more bytes than expected (%d generated, %d expected)"), BytesGenerated, AudioBufferRef.Num());
			if (BytesGenerated < AudioBufferRef.Num())
			{
				AudioBufferRef.SetNum(BytesGenerated, EAllowShrinking::No);
			}
		}
	}

	SamplesAvailable = AudioBufferRef.Num() / SampleByteSize;

	// Wait until we have enough samples that are requested before starting.
	if (SamplesAvailable > 0)
	{
		const int32 SamplesToCopy = FMath::Min<int32>(SamplesToGenerate, SamplesAvailable);
		const int32 BytesToCopy = SamplesToCopy * SampleByteSize;

		FMemory::Memcpy((void*)PCMData, &AudioBufferRef[0], BytesToCopy);
		AudioBufferRef.RemoveAt(0, BytesToCopy, EAllowShrinking::No);

		return BytesToCopy;
	}

	// There wasn't enough data ready, write out zeros
	const int32 BytesCopied = NumBufferUnderrunSamples * SampleByteSize;
	FMemory::Memzero(PCMData, BytesCopied);
	return BytesCopied;
}

int32 UMySoundWave::GetResourceSizeForFormat(FName Format)
{
	return 0;
}

void UMySoundWave::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
	PRAGMA_DISABLE_DEPRECATION_WARNINGS;
	Super::GetAssetRegistryTags(OutTags);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS;
}

void UMySoundWave::GetAssetRegistryTags(FAssetRegistryTagsContext Context) const
{
	Super::GetAssetRegistryTags(Context);
}

bool UMySoundWave::HasCompressedData(FName Format, ITargetPlatform* TargetPlatform) const
{
	return false;
}

void UMySoundWave::BeginGetCompressedData(FName Format, const FPlatformAudioCookOverrides* CompressionOverrides, const ITargetPlatform* InTargetPlatform)
{
	// MySoundWave does not have compressed data and should generally not be asked about it
}

FByteBulkData* UMySoundWave::GetCompressedData(FName Format, const FPlatformAudioCookOverrides* CompressionOverrides, const ITargetPlatform* InTargetPlatform )
{
	// MySoundWave does not have compressed data and should generally not be asked about it
	return nullptr;
}

void UMySoundWave::Serialize(FArchive& Ar)
{
	// Do not call the USoundWave version of serialize
	USoundBase::Serialize(Ar);

#if WITH_EDITORONLY_DATA
	// Due to "skipping" USoundWave::Serialize above, modulation
	// versioning is required to be called explicitly here.
	if (Ar.IsLoading())
	{
		ModulationSettings.VersionModulators();
	}
#endif // WITH_EDITORONLY_DATA
}

void UMySoundWave::InitAudioResource(FByteBulkData& CompressedData)
{
	// Should never be pushing compressed data to a MySoundWave
	check(false);
}

bool UMySoundWave::InitAudioResource(FName Format)
{
	// Nothing to be done to initialize a UMySoundWave
	return true;
}

