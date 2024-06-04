// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#include "RenderGraphResources.h"
#include "Runtime/Engine/Classes/Engine/TextureRenderTarget2D.h"

// This struct contains all the data we need to pass from the game thread to draw our effect.
struct FShaderFogMaskParameters
{
	UTexture2D* InTexture;
	UTextureRenderTarget2D* OutRT;
	UMaterialInterface* UseMaterial;

	//transient
	FTextureRHIRef SceneDepth;
	FVector2D ViewSize;
	FVector4 InvDeviceZToWorldZTransform;
	float BlurRadius;
	float RimFactor;
	int Iterations;
	FSceneView* View;
	TUniformBufferRef<FSceneTextureUniformParameters> SceneTexturesUniformParams;
	TUniformBufferRef<FMobileSceneTextureUniformParameters> MobileSceneTexturesUniformParams;

	FIntPoint GetRenderTargetSize() const
	{
		return CachedRenderTargetSize;
	}

	FShaderFogMaskParameters() { }
	FShaderFogMaskParameters(UTextureRenderTarget2D* Src, UTextureRenderTarget2D* Temp, 
		UTextureRenderTarget2D* Temp01, UTextureRenderTarget2D* Dest)
	{
		CachedRenderTargetSize = FIntPoint(256,256);
	}

private:
	FIntPoint CachedRenderTargetSize;
};

class FMaterialShaderPluginModule : public IModuleInterface
{
public:
	static inline FMaterialShaderPluginModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FMaterialShaderPluginModule>("MaterialShaderPlugin");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("MaterialShaderPlugin");
	}
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
public:
	// Call this when you want to hook onto the renderer and start drawing. The shader will be executed once per frame.
	void BeginRendering();

	// When you are done, call this to stop drawing.
	void EndRendering();

	// Call this whenever you have new parameters to share. You could set this up to update different sets of properties at
	// different intervals to save on locking and GPU transfer time.
	void UpdateParameters(FShaderFogMaskParameters& DrawParameters);

private:
	FShaderFogMaskParameters CachedFogMaskParameters;
	FDelegateHandle OnPostResolvedSceneColorHandle;
	FDelegateHandle OnPostOpaqueRenderDelegateHandle;
	FCriticalSection RenderEveryFrameLock;
	volatile bool bCachedParametersValid;
	volatile bool bIsGameView = false;
	volatile bool bIsSceneCapture = false;

	void PostResolveSceneColor_RenderThread(FRHICommandListImmediate& RHICmdList, class FSceneRenderTargets& SceneContext);
	void Draw_RenderThread(FShaderFogMaskParameters& DrawParameters);
	void PostOpaqueRender_RenderThread(FPostOpaqueRenderParameters& Parameters);
};
