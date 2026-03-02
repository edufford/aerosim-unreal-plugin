// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Weather/AerosimWeather.h"
#include "Weather/WeatherMapDataAsset.h"
#include "Components/SceneComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FLinearColor JsonToColor(TSharedPtr<FJsonObject> JsonColor)
{
	if (!JsonColor.IsValid())
		return FLinearColor::White; // Default color if invalid

	FLinearColor Color;
	Color.R = JsonColor->GetNumberField(TEXT("R"));
	Color.G = JsonColor->GetNumberField(TEXT("G"));
	Color.B = JsonColor->GetNumberField(TEXT("B"));
	Color.A = JsonColor->GetNumberField(TEXT("A"));
	return Color;
}

TSharedPtr<FJsonObject> ColorToJson(const FLinearColor& Color)
{
	TSharedPtr<FJsonObject> JsonColor = MakeShareable(new FJsonObject());
	JsonColor->SetNumberField("R", Color.R);
	JsonColor->SetNumberField("G", Color.G);
	JsonColor->SetNumberField("B", Color.B);
	JsonColor->SetNumberField("A", Color.A);
	return JsonColor;
}

AAerosimWeather::AAerosimWeather(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SceneComponent = CreateDefaultSubobject<USceneComponent>("SceneComponent");
	RootComponent = SceneComponent;

	ExponentialHeightFogComponent = CreateDefaultSubobject<UExponentialHeightFogComponent>("ExponentialHeightFogComponent");
	ExponentialHeightFogComponent->SetupAttachment(RootComponent);

	VolumetricCloudComponent = CreateDefaultSubobject<UVolumetricCloudComponent>("VolumetricCloudComponent");
	VolumetricCloudComponent->SetupAttachment(RootComponent);
}

void AAerosimWeather::BeginPlay()
{
	Super::BeginPlay();
	InitializeReferences();
}

void AAerosimWeather::SaveWeatherPresetData()
{
	InitializeReferences();
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());

	// Save Exponential Height Fog properties
	JsonObject->SetNumberField("FogDensity", ExponentialHeightFogComponent->FogDensity);
	JsonObject->SetNumberField("FogHeightFalloff", ExponentialHeightFogComponent->FogHeightFalloff);
	JsonObject->SetNumberField("SecondFogDensity", ExponentialHeightFogComponent->SecondFogData.FogDensity);
	JsonObject->SetNumberField("SecondFogHeightFalloff", ExponentialHeightFogComponent->SecondFogData.FogHeightFalloff);
	JsonObject->SetNumberField("SecondFogHeightOffset", ExponentialHeightFogComponent->SecondFogData.FogHeightFalloff);
	JsonObject->SetNumberField("FogMaxOpacity", ExponentialHeightFogComponent->FogMaxOpacity);
	JsonObject->SetNumberField("StartDistance", ExponentialHeightFogComponent->StartDistance);
	JsonObject->SetNumberField("FogCutoffDistance", ExponentialHeightFogComponent->FogCutoffDistance);

	JsonObject->SetNumberField("DirectionalInScatteringExponent", ExponentialHeightFogComponent->DirectionalInscatteringExponent);
	JsonObject->SetNumberField("DirectionalInScatteringStartDistance", ExponentialHeightFogComponent->DirectionalInscatteringStartDistance);
	JsonObject->SetObjectField("DirectionalInScatteringColor", ColorToJson(ExponentialHeightFogComponent->DirectionalInscatteringLuminance));

	JsonObject->SetBoolField("bVolumetricFog", ExponentialHeightFogComponent->bEnableVolumetricFog);
	JsonObject->SetNumberField("VolumetricFogScatteringDistribution", ExponentialHeightFogComponent->VolumetricFogScatteringDistribution);
	JsonObject->SetObjectField("VolumetricFogAlbedo", ColorToJson(ExponentialHeightFogComponent->VolumetricFogAlbedo));
	JsonObject->SetObjectField("VolumetricFogEmissive", ColorToJson(ExponentialHeightFogComponent->VolumetricFogEmissive));
	JsonObject->SetNumberField("VolumetricFogExtinctionScale", ExponentialHeightFogComponent->VolumetricFogExtinctionScale);
	JsonObject->SetNumberField("VolumetricFogViewDistance", ExponentialHeightFogComponent->VolumetricFogDistance);
	JsonObject->SetNumberField("VolumetricFogNearInDistance", ExponentialHeightFogComponent->VolumetricFogNearFadeInDistance);
	JsonObject->SetNumberField("VolumetricFogStaticLightingScatteringIntensity", ExponentialHeightFogComponent->VolumetricFogStaticLightingScatteringIntensity);

	// Save Volumetric Cloud properties
	JsonObject->SetStringField("CloudMaterial", VolumetricCloudComponent->Material->GetPathName());

	JsonObject->SetNumberField("LayerBottomAltitude", VolumetricCloudComponent->LayerBottomAltitude);
	JsonObject->SetNumberField("LayerHeight", VolumetricCloudComponent->LayerHeight);
	JsonObject->SetNumberField("TracingStartMaxDistance", VolumetricCloudComponent->TracingStartMaxDistance);
	JsonObject->SetNumberField("TracingMaxDistance", VolumetricCloudComponent->TracingMaxDistance);

	JsonObject->SetBoolField("UsePerSampleAtmosphericLightTransmitannce", VolumetricCloudComponent->bUsePerSampleAtmosphericLightTransmittance);
	JsonObject->SetNumberField("SkyLightCloudBottomOcclusion", VolumetricCloudComponent->SkyLightCloudBottomOcclusion);
	JsonObject->SetNumberField("ViewSampleCountScale", VolumetricCloudComponent->ViewSampleCountScale);
	JsonObject->SetNumberField("ReflectionViewSampleCountScaleValue", VolumetricCloudComponent->ReflectionViewSampleCountScaleValue);
	JsonObject->SetNumberField("ShadowViewSampleCountScale", VolumetricCloudComponent->ShadowViewSampleCountScale);
	JsonObject->SetNumberField("ShadowReflectionViewSampleCountScaleValue", VolumetricCloudComponent->ShadowReflectionViewSampleCountScaleValue);
	JsonObject->SetNumberField("ShadowTracingDistance", VolumetricCloudComponent->ShadowTracingDistance);
	JsonObject->SetNumberField("StopTracingTransmittanceThreshold", VolumetricCloudComponent->StopTracingTransmittanceThreshold);

	// Save Sky Atmosphere properties
	JsonObject->SetNumberField("RayleighScatteringScale", SkyAtmosphereComponent->RayleighScatteringScale);
	JsonObject->SetNumberField("RayleighExponentialDistribution", SkyAtmosphereComponent->RayleighExponentialDistribution);
	JsonObject->SetObjectField("RayleighScattering", ColorToJson(SkyAtmosphereComponent->RayleighScattering));

	JsonObject->SetNumberField("MieScatteringScale", SkyAtmosphereComponent->MieScatteringScale);
	JsonObject->SetObjectField("MieScattering", ColorToJson(SkyAtmosphereComponent->MieScattering));
	JsonObject->SetNumberField("MieAbsorptionScale", SkyAtmosphereComponent->MieAbsorptionScale);
	JsonObject->SetObjectField("MieAbsorption", ColorToJson(SkyAtmosphereComponent->MieAbsorption));
	JsonObject->SetNumberField("MieAnisotropy", SkyAtmosphereComponent->MieAnisotropy);
	JsonObject->SetNumberField("MieExponentialDistribution", SkyAtmosphereComponent->MieExponentialDistribution);

	// Convert to string
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	// Save to data asset
	if (WeatherMapDataAsset->WeatherMap.Contains(WeatherPresetName))
	{
		WeatherMapDataAsset->WeatherMap[WeatherPresetName] = OutputString;
	}
	else
	{
		WeatherMapDataAsset->WeatherMap.Add(WeatherPresetName, OutputString);
	}
}

void AAerosimWeather::LoadWeatherPresetData()
{
	LoadWeatherPresetDataByName(WeatherPresetName);
}

void AAerosimWeather::LoadWeatherPresetDataByName(const FString& WeatherPreset)
{
	InitializeReferences();

	if (WeatherMapDataAsset->WeatherMap.Contains(WeatherPreset))
	{
		ApplyLoadedWeatherPreset(WeatherMapDataAsset->WeatherMap[WeatherPreset]);
	}
	else
	{
		UE_LOG(LogAerosimConnector, Error, TEXT("Weather preset not found"));
	}
}

void AAerosimWeather::InitializeReferences()
{
	if (SkyAtmosphereComponent != nullptr && SkyLightComponent != nullptr)
	{
		return;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACesiumSunSky::StaticClass(), FoundActors);

	if (FoundActors.Num() > 0)
	{
		ACesiumSunSky* SunSky = Cast<ACesiumSunSky>(FoundActors[0]);
		if (SunSky)
		{
			SkyLightComponent = SunSky->SkyLight;
			SkyAtmosphereComponent = SunSky->SkyAtmosphere;
		}
	}
}

void AAerosimWeather::ApplyLoadedWeatherPreset(const FString& JsonString)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		// Load and apply settings
		ExponentialHeightFogComponent->FogDensity = JsonObject->GetNumberField(TEXT("FogDensity"));
		ExponentialHeightFogComponent->SetFogHeightFalloff(JsonObject->GetNumberField(TEXT("FogHeightFalloff")));
		ExponentialHeightFogComponent->SecondFogData.FogDensity = JsonObject->GetNumberField(TEXT("SecondFogDensity"));
		ExponentialHeightFogComponent->SecondFogData.FogHeightFalloff = JsonObject->GetNumberField(TEXT("SecondFogHeightFalloff"));
		ExponentialHeightFogComponent->SecondFogData.FogHeightOffset = JsonObject->GetNumberField(TEXT("SecondFogHeightOffset"));
		ExponentialHeightFogComponent->FogMaxOpacity = JsonObject->GetNumberField(TEXT("FogMaxOpacity"));
		ExponentialHeightFogComponent->StartDistance = JsonObject->GetNumberField(TEXT("StartDistance"));
		ExponentialHeightFogComponent->FogCutoffDistance = JsonObject->GetNumberField(TEXT("FogCutoffDistance"));

		ExponentialHeightFogComponent->DirectionalInscatteringExponent = JsonObject->GetNumberField(TEXT("DirectionalInScatteringExponent"));
		ExponentialHeightFogComponent->DirectionalInscatteringStartDistance = JsonObject->GetNumberField(TEXT("DirectionalInScatteringStartDistance"));
		ExponentialHeightFogComponent->DirectionalInscatteringLuminance = JsonToColor(JsonObject->GetObjectField(TEXT("DirectionalInScatteringColor")));

		ExponentialHeightFogComponent->bEnableVolumetricFog = JsonObject->GetBoolField(TEXT("bVolumetricFog"));
		ExponentialHeightFogComponent->VolumetricFogScatteringDistribution = JsonObject->GetNumberField(TEXT("VolumetricFogScatteringDistribution"));
		ExponentialHeightFogComponent->VolumetricFogAlbedo = JsonToColor(JsonObject->GetObjectField(TEXT("VolumetricFogAlbedo"))).ToFColor(false);
		ExponentialHeightFogComponent->VolumetricFogEmissive = JsonToColor(JsonObject->GetObjectField(TEXT("VolumetricFogEmissive")));
		ExponentialHeightFogComponent->VolumetricFogExtinctionScale = JsonObject->GetNumberField(TEXT("VolumetricFogExtinctionScale"));
		ExponentialHeightFogComponent->VolumetricFogDistance = JsonObject->GetNumberField(TEXT("VolumetricFogViewDistance"));
		ExponentialHeightFogComponent->VolumetricFogNearFadeInDistance = JsonObject->GetNumberField(TEXT("VolumetricFogNearInDistance"));
		ExponentialHeightFogComponent->VolumetricFogStaticLightingScatteringIntensity = JsonObject->GetNumberField(TEXT("VolumetricFogStaticLightingScatteringIntensity"));

		VolumetricCloudComponent->LayerBottomAltitude = JsonObject->GetNumberField(TEXT("LayerBottomAltitude"));
		VolumetricCloudComponent->LayerHeight = JsonObject->GetNumberField(TEXT("LayerHeight"));
		VolumetricCloudComponent->TracingStartMaxDistance = JsonObject->GetNumberField(TEXT("TracingStartMaxDistance"));
		VolumetricCloudComponent->TracingMaxDistance = JsonObject->GetNumberField(TEXT("TracingMaxDistance"));

		FString MaterialPath = JsonObject->GetStringField(TEXT("CloudMaterial"));
		if (MaterialPath == "None")
		{
			VolumetricCloudComponent->SetMaterial(nullptr);
		}
		else
		{
			VolumetricCloudComponent->SetMaterial(Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, *JsonObject->GetStringField(TEXT("CloudMaterial")))));
		}
		VolumetricCloudComponent->bUsePerSampleAtmosphericLightTransmittance = JsonObject->GetBoolField(TEXT("UsePerSampleAtmosphericLightTransmitannce"));
		VolumetricCloudComponent->SkyLightCloudBottomOcclusion = JsonObject->GetNumberField(TEXT("SkyLightCloudBottomOcclusion"));
		VolumetricCloudComponent->ViewSampleCountScale = JsonObject->GetNumberField(TEXT("ViewSampleCountScale"));
		VolumetricCloudComponent->ReflectionViewSampleCountScaleValue = JsonObject->GetNumberField(TEXT("ReflectionViewSampleCountScaleValue"));
		VolumetricCloudComponent->ShadowViewSampleCountScale = JsonObject->GetNumberField(TEXT("ShadowViewSampleCountScale"));
		VolumetricCloudComponent->ShadowReflectionViewSampleCountScaleValue = JsonObject->GetNumberField(TEXT("ShadowReflectionViewSampleCountScaleValue"));
		VolumetricCloudComponent->ShadowTracingDistance = JsonObject->GetNumberField(TEXT("ShadowTracingDistance"));
		VolumetricCloudComponent->StopTracingTransmittanceThreshold = JsonObject->GetNumberField(TEXT("StopTracingTransmittanceThreshold"));

		SkyAtmosphereComponent->RayleighScatteringScale = JsonObject->GetNumberField(TEXT("RayleighScatteringScale"));
		SkyAtmosphereComponent->RayleighExponentialDistribution = JsonObject->GetNumberField(TEXT("RayleighExponentialDistribution"));
		SkyAtmosphereComponent->RayleighScattering = JsonToColor(JsonObject->GetObjectField(TEXT("RayleighScattering")));

		SkyAtmosphereComponent->MieScatteringScale = JsonObject->GetNumberField(TEXT("MieScatteringScale"));
		SkyAtmosphereComponent->MieScattering = JsonToColor(JsonObject->GetObjectField(TEXT("MieScattering")));
		SkyAtmosphereComponent->MieAbsorptionScale = JsonObject->GetNumberField(TEXT("MieAbsorptionScale"));
		SkyAtmosphereComponent->MieAbsorption = JsonToColor(JsonObject->GetObjectField(TEXT("MieAbsorption")));
		SkyAtmosphereComponent->MieAnisotropy = JsonObject->GetNumberField(TEXT("MieAnisotropy"));
		SkyAtmosphereComponent->MieExponentialDistribution = JsonObject->GetNumberField(TEXT("MieExponentialDistribution"));
	}
}

void AAerosimWeather::SetWeatherDataAsset(UWeatherMapDataAsset* WeatherDataAsset)
{
	WeatherMapDataAsset = WeatherDataAsset;
}
