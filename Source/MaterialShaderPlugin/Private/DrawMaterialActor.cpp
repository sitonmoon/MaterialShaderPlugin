// Fill out your copyright notice in the Description page of Project Settings.


#include "DrawMaterialActor.h"
#include "DrawMaterialShader.h"

// Sets default values
ADrawMaterialActor::ADrawMaterialActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
}

UTextureRenderTarget2D* ADrawMaterialActor::CreateRT(const TEnumAsByte<ETextureRenderTargetFormat> RenderTargetFormat)
{
	UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(this);
	RT->ClearColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
	RT->RenderTargetFormat = RenderTargetFormat;
	RT->InitAutoFormat(256, 256);
	return RT;
}

void ADrawMaterialActor::InitRT()
{
}

// Called when the game starts or when spawned
void ADrawMaterialActor::BeginPlay()
{
	Super::BeginPlay();
	FMaterialShaderPluginModule::Get().BeginRendering();
}

void ADrawMaterialActor::BeginDestroy()
{
	FMaterialShaderPluginModule::Get().EndRendering();
	Super::BeginDestroy();
}

// Called every frame
void ADrawMaterialActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	FShaderFogMaskParameters DrawParameters;
	DrawParameters.InTexture = InTexture;
	DrawParameters.OutRT = OutRenderTarget;
	DrawParameters.UseMaterial = MyMaterial;

	// If doing this for realsies, you should avoid doing this every frame unless you have to of course.
	// We set it every frame here since we're updating the end color and simulation state. Boop.
	FMaterialShaderPluginModule::Get().UpdateParameters(DrawParameters);
}

