// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "DrawMaterialActor.generated.h"

UCLASS()
class MATERIALSHADERPLUGIN_API ADrawMaterialActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADrawMaterialActor();
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DrawMaterialShader")
	UTexture2D* InTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DrawMaterialShader")
	UTextureRenderTarget2D* OutRenderTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DrawMaterialShader")
	UMaterialInterface* MyMaterial;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;
	void InitRT();
	UTextureRenderTarget2D* CreateRT(const TEnumAsByte<ETextureRenderTargetFormat> RenderTargetFormat);
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
