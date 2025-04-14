// Fill out your copyright notice in the Description page of Project Settings.


#include "MyBlueprintFunctionLibrary.h"
#include "SoundFileIO/SoundFileIO.h"
#include "Sound/MySoundWave.h"
#include "Examples.h"
#include "Interfaces/IAudioFormat.h"
#include "Decoders/VorbisAudioInfo.h"

struct FMySoundWaveInfo
{
    int32 SampleRate;
    int32 NumChannels;
    int32 NumSamples;
    float Duration;
    float TotalSamples;
    TArray<uint8> PCMData;

    FMySoundWaveInfo() = default;
    FMySoundWaveInfo(const FMySoundWaveInfo& Other) = default;
    FMySoundWaveInfo(FMySoundWaveInfo&& Other) = default;
    FMySoundWaveInfo& operator=(const FMySoundWaveInfo& Other) = default;
    FMySoundWaveInfo& operator=(FMySoundWaveInfo&& Other) = default;
};

static bool GetSoundWaveInfoFromWav(FMySoundWaveInfo& Info, TArray<uint8> RawWaveData)
{
    FWaveModInfo WaveInfo;
    FString ErrorMessage;
    if (!WaveInfo.ReadWaveInfo(RawWaveData.GetData(), RawWaveData.Num(), &ErrorMessage))
    {
        UE_LOG(LogMyExamples, Error, TEXT("Unable to read wave file - \"%s\""), *ErrorMessage);
        return false;
    }

    UMySoundWave* Sound = NewObject<UMySoundWave>();
    int32 ChannelCount = (int32)*WaveInfo.pChannels;
    check(ChannelCount > 0);
    int32 SizeOfSample = (*WaveInfo.pBitsPerSample) / 8;
    int32 NumSamples = WaveInfo.SampleDataSize / SizeOfSample;
    int32 NumFrames = NumSamples / ChannelCount;

	Info.SampleRate = *WaveInfo.pSamplesPerSec;
    Info.Duration = (float)NumFrames / *WaveInfo.pSamplesPerSec;
	Info.NumChannels = ChannelCount;
	Info.TotalSamples = *WaveInfo.pSamplesPerSec * Sound->Duration;
    Info.PCMData = MoveTemp(RawWaveData);

    return true;
}

static bool GetSoundWaveInfoFromOgg(FMySoundWaveInfo& Info, const TArray<uint8>& OggData)
{
    FVorbisAudioInfo	AudioInfo;
    FSoundQualityInfo	QualityInfo;
    if (!AudioInfo.ReadCompressedInfo(OggData.GetData(), OggData.Num(), &QualityInfo))
    {
        return false;
    }
    TArray<uint8> PCMData;
    PCMData.AddUninitialized(QualityInfo.SampleDataSize);
    AudioInfo.ReadCompressedData(PCMData.GetData(), false, QualityInfo.SampleDataSize);

    Info.SampleRate = QualityInfo.SampleRate;
    Info.Duration = QualityInfo.Duration;
    Info.NumChannels = QualityInfo.NumChannels;
    Info.TotalSamples = QualityInfo.Duration * QualityInfo.Duration;
    Info.PCMData = MoveTemp(PCMData);
    return true;
}

void UMyBlueprintFunctionLibrary::CreateSoundWaveFromFile(const FString& FilePath, const FOnSoundWaveDelegate& SoundWaveCallback)
{
	AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [FilePath, SoundWaveCallback]()
		{
            TArray<uint8> FileContent;
            if (!FFileHelper::LoadFileToArray(FileContent, *FilePath))
            {
                UE_LOG(LogMyExamples, Error, TEXT("Failed to load file at path: %s"), *FilePath);
                return;
            }
            double LoadingStartTime = FPlatformTime::Seconds();
            bool bSuccess = false;
            FMySoundWaveInfo SoundWaveInfo;
            if (FilePath.ToLower().EndsWith(".wav"))
            {
				if (GetSoundWaveInfoFromWav(SoundWaveInfo, FileContent))
				{
					bSuccess = true;
				}
            }
            else if (FilePath.ToLower().EndsWith(".ogg"))
            {
				if (GetSoundWaveInfoFromOgg(SoundWaveInfo, FileContent))
				{
					bSuccess = true;
				}
            }
            else
            {
                // UE runtime only supports wav or ogg
            }
            AsyncTask(ENamedThreads::GameThread, [SoundWaveCallback, bSuccess, SoundWaveInfo, FilePath]()
                {
                    if (!bSuccess)
                    {
                        UE_LOG(LogMyExamples, Error, TEXT("Failed to create sound wave from file at path: %s"), *FilePath);
                        SoundWaveCallback.ExecuteIfBound(nullptr);
                        return;
                    }
                    UMySoundWave* SoundWave = NewObject<UMySoundWave>();
                    SoundWave->SetAudio(SoundWaveInfo.PCMData);
                    SoundWave->Duration = SoundWaveInfo.Duration;
                    SoundWave->SetImportedSampleRate(SoundWaveInfo.SampleRate);
                    SoundWave->SetSampleRate(SoundWaveInfo.SampleRate);
                    SoundWave->NumChannels = SoundWaveInfo.NumChannels;
                    SoundWave->TotalSamples = SoundWaveInfo.TotalSamples;
					SoundWaveCallback.ExecuteIfBound(SoundWave);
                });
		});
}
