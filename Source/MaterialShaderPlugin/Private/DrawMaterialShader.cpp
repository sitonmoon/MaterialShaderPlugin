// Copyright Epic Games, Inc. All Rights Reserved.

#include "DrawMaterialShader.h"
#include "CustomMaterialShader.h"

#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "RHI.h"
#include "GlobalShader.h"
#include "RHICommandList.h"
#include "RenderGraphBuilder.h"
#include "RenderTargetPool.h"
#include "Runtime/Core/Public/Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Runtime/Renderer/Private/PostProcess/SceneRenderTargets.h"
#include "Runtime/Renderer/Private/SceneRendering.h"

#define LOCTEXT_NAMESPACE "FMaterialShaderPluginModule"

DECLARE_GPU_STAT_NAMED(ShaderPlugin_Render, TEXT("ShaderPlugin: Root Render"));
DECLARE_GPU_STAT_NAMED(ShaderPlugin_Pixel, TEXT("ShaderPlugin: Render Pixel Shader"));

void FMaterialShaderPluginModule::StartupModule()
{
	OnPostResolvedSceneColorHandle.Reset();
	bCachedParametersValid = false;

	// Maps virtual shader source directory to the plugin's actual shaders directory.
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("MaterialShaderPlugin"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Shaders"), PluginShaderDir);
}

void FMaterialShaderPluginModule::ShutdownModule()
{
	EndRendering();
}

void FMaterialShaderPluginModule::BeginRendering()
{
	if (OnPostResolvedSceneColorHandle.IsValid())
	{
		return;
	}
	bCachedParametersValid = false;
	const FName RendererModuleName("Renderer");
	IRendererModule* RendererModule = FModuleManager::GetModulePtr<IRendererModule>(RendererModuleName);
	if (RendererModule)
	{
		OnPostResolvedSceneColorHandle = RendererModule->GetResolvedSceneColorCallbacks().AddRaw(this, 
			&FMaterialShaderPluginModule::PostResolveSceneColor_RenderThread);
		OnPostOpaqueRenderDelegateHandle = RendererModule->RegisterPostOpaqueRenderDelegate(
			FPostOpaqueRenderDelegate::CreateRaw(this, &FMaterialShaderPluginModule::PostOpaqueRender_RenderThread));
	}
}

void FMaterialShaderPluginModule::EndRendering()
{
	if (!OnPostResolvedSceneColorHandle.IsValid())
	{
		return;
	}
	const FName RendererModuleName("Renderer");
	IRendererModule* RendererModule = FModuleManager::GetModulePtr<IRendererModule>(RendererModuleName);
	if (RendererModule)
	{
		RendererModule->GetResolvedSceneColorCallbacks().Remove(OnPostResolvedSceneColorHandle);
		RendererModule->RemovePostOpaqueRenderDelegate(OnPostOpaqueRenderDelegateHandle);
	}
	OnPostResolvedSceneColorHandle.Reset();
}

void FMaterialShaderPluginModule::UpdateParameters(FShaderFogMaskParameters& DrawParameters)
{
	FVector2D ViewSize;
	RenderEveryFrameLock.Lock(); 
	ViewSize = CachedFogMaskParameters.ViewSize;
	CachedFogMaskParameters.BlurRadius = DrawParameters.BlurRadius;
	CachedFogMaskParameters.InTexture = DrawParameters.InTexture;
	CachedFogMaskParameters.UseMaterial = DrawParameters.UseMaterial;
	CachedFogMaskParameters.RimFactor = DrawParameters.RimFactor;
	CachedFogMaskParameters.OutRT = DrawParameters.OutRT;
	bCachedParametersValid = true;
	RenderEveryFrameLock.Unlock();
	if (bIsGameView && !bIsSceneCapture)
		DrawParameters.ViewSize = ViewSize;
}

void FMaterialShaderPluginModule::PostResolveSceneColor_RenderThread(FRHICommandListImmediate& RHICmdList, class FSceneRenderTargets& SceneContext)
{
	if (!bCachedParametersValid || !bIsGameView)
	{
		return;
	}

	// Depending on your data, you might not have to lock here, just added this code to show how you can do it if you have to.
	RenderEveryFrameLock.Lock();
	FShaderFogMaskParameters Copy = CachedFogMaskParameters;
	const FTexture2DRHIRef& RHISceneDepth =
		(const FTexture2DRHIRef&)SceneContext.SceneDepthZ->GetRenderTargetItem().ShaderResourceTexture;
	Copy.SceneDepth = RHISceneDepth;
	RenderEveryFrameLock.Unlock();

	Draw_RenderThread(Copy);
}

void FMaterialShaderPluginModule::PostOpaqueRender_RenderThread(FPostOpaqueRenderParameters& Parameters)
{
	if (!bCachedParametersValid)
	{
		return;
	}

	// Depending on your data, you might not have to lock here, just added this code to show how you can do it if you have to.
	RenderEveryFrameLock.Lock();
	const FViewInfo* View =(FViewInfo*)Parameters.Uid;
	bIsGameView = View->bIsGameView;
	bIsSceneCapture = View->bIsSceneCapture;
	CachedFogMaskParameters.View = (FSceneView*)Parameters.Uid;
	if (bIsGameView && !bIsSceneCapture)
	{
		CachedFogMaskParameters.InvDeviceZToWorldZTransform = View->InvDeviceZToWorldZTransform;
		CachedFogMaskParameters.ViewSize = Parameters.ViewportRect.Size();
	}
	CachedFogMaskParameters.SceneTexturesUniformParams = Parameters.SceneTexturesUniformParams;
	CachedFogMaskParameters.MobileSceneTexturesUniformParams = Parameters.MobileSceneTexturesUniformParams;
	RenderEveryFrameLock.Unlock();

	if (IsMobilePlatform(View->GetShaderPlatform()))
	{
		Draw_RenderThread(CachedFogMaskParameters);
	}
}

void FMaterialShaderPluginModule::Draw_RenderThread(FShaderFogMaskParameters& DrawParameters)
{
	check(IsInRenderingThread());

	if (!bIsGameView || bIsSceneCapture || DrawParameters.ViewSize.X <= 0 || DrawParameters.ViewSize.Y <= 0)
	{
		return;
	}

	FRHICommandListImmediate& RHICmdList = GRHICommandList.GetImmediateCommandList();

	QUICK_SCOPE_CYCLE_COUNTER(STAT_MaterialShaderPlugin_Render); // Used to gather CPU profiling data for the UE4 session frontend
	SCOPED_DRAW_EVENT(RHICmdList, MaterialShaderPlugin_Render); // Used to profile GPU activity and add metadata to be consumed by for example RenderDoc

	CustomMaterialShader::MaterialShaderDraw(RHICmdList, DrawParameters);
	
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMaterialShaderPluginModule, MaterialShaderPlugin)