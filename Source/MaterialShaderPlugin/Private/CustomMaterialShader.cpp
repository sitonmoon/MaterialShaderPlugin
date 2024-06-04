// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomMaterialShader.h"
#include "Runtime/Renderer/Private/ScenePrivate.h"
#include "ShaderParameterUtils.h"
#include "RHIStaticStates.h"
#include "Shader.h"
#include "GlobalShader.h"
#include "MaterialShader.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "ShaderParameterMacros.h"
#include "ShaderParameterStruct.h"
#include "UniformBuffer.h"
#include "RHICommandList.h"
#include "Containers/DynamicRHIResourceArray.h"
#include "Runtime/RenderCore/Public/PixelShaderUtils.h"

/************************************************************************/
/* Simple static vertex buffer.                                         */
/************************************************************************/
class FSimpleScreenVertexBuffer : public FVertexBuffer
{
public:
	/** Initialize the RHI for this rendering resource */
	void InitRHI()
	{
		TResourceArray<FFilterVertex, VERTEXBUFFER_ALIGNMENT> Vertices;
		Vertices.SetNumUninitialized(6);

		Vertices[0].Position = FVector4(-1, 1, 0, 1);
		Vertices[0].UV = FVector2D(0, 0);

		Vertices[1].Position = FVector4(1, 1, 0, 1);
		Vertices[1].UV = FVector2D(1, 0);

		Vertices[2].Position = FVector4(-1, -1, 0, 1);
		Vertices[2].UV = FVector2D(0, 1);

		Vertices[3].Position = FVector4(1, -1, 0, 1);
		Vertices[3].UV = FVector2D(1, 1);

		// Create vertex buffer. Fill buffer with initial data upon creation
		FRHIResourceCreateInfo CreateInfo(&Vertices);
		VertexBufferRHI = RHICreateVertexBuffer(Vertices.GetResourceDataSize(), BUF_Static, CreateInfo);
	}
};
TGlobalResource<FSimpleScreenVertexBuffer> GSimpleScreenVertexBuffer;


class FMyMaterialVS : public FMaterialShader
{
public:
	DECLARE_SHADER_TYPE(FMyMaterialVS, Material);

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView View, const FMaterialRenderProxy* MaterialProxy, const FMaterial& Material)
	{
		FRHIPixelShader* ShaderRHI = RHICmdList.GetBoundPixelShader();
		FMaterialShader::SetParameters(RHICmdList, ShaderRHI, MaterialProxy, Material, View);
	}

	FMyMaterialVS() { }
	FMyMaterialVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FMaterialShader(Initializer) { }
};


class FMyMaterialPS : public FMaterialShader
{
	DECLARE_SHADER_TYPE(FMyMaterialPS, Material);

public:

	static void ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("MY_MATERIALSHADER"), 1);
		FMaterialShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}

	static bool ShouldCompilePermutation(const FMaterialShaderPermutationParameters& Parameters)
	{
		// if (Parameters.MaterialParameters.)
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	FMyMaterialPS() {}
	FMyMaterialPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FMaterialShader(Initializer)
	{
		MainTexture.Bind(Initializer.ParameterMap, TEXT("MainTexture"), SPF_Optional);
		MainTextureSampler.Bind(Initializer.ParameterMap, TEXT("MainTextureSampler"), SPF_Optional); 
	}

	void SetParameters_Texture(FRHICommandList& RHICmdList, FRHITexture* TextureRHI) const
	{
		FRHISamplerState* MainTextureSamplerState = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), MainTexture, MainTextureSampler, MainTextureSamplerState, TextureRHI);
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView View, const FMaterialRenderProxy* MaterialProxy, const FMaterial& Material)
	{
		FRHIPixelShader* ShaderRHI = RHICmdList.GetBoundPixelShader();

		FMaterialShader::SetViewParameters(RHICmdList, ShaderRHI, View, View.ViewUniformBuffer);
		FMaterialShader::SetParameters(RHICmdList, ShaderRHI, MaterialProxy, Material, View);
		
	}

private:
	LAYOUT_FIELD(FShaderResourceParameter, MainTexture);
	LAYOUT_FIELD(FShaderResourceParameter, MainTextureSampler);
};

IMPLEMENT_MATERIAL_SHADER_TYPE(, FMyMaterialVS, TEXT("/Shaders/CustomMaterialShader.usf"), TEXT("MainVS"), SF_Vertex);
IMPLEMENT_MATERIAL_SHADER_TYPE(, FMyMaterialPS, TEXT("/Shaders/CustomMaterialShader.usf"), TEXT("MainPS"), SF_Pixel);

void CustomMaterialShader::MaterialShaderDraw(FRHICommandListImmediate& RHICmdList, FShaderFogMaskParameters& DrawParameters)
{
	check(IsInRenderingThread());
	UMaterialInterface* MaterialInterface = DrawParameters.UseMaterial;
	if (IsValid(MaterialInterface))
	{
#if WANTS_DRAW_MESH_EVENTS  
		SCOPED_DRAW_EVENTF(RHICmdList, MaterialShaderReneding, TEXT("MyMaterialShaderPassTest"));
#else  
		SCOPED_DRAW_EVENT(RHICmdList, MaterialShaderReneding);
#endif  
		const ERHIFeatureLevel::Type FeatureLevel = DrawParameters.View->GetFeatureLevel();
		FUniformBufferRHIRef PassUniformBuffer = CreateSceneTextureUniformBufferDependentOnShadingPath(RHICmdList, FeatureLevel);
		SCOPED_UNIFORM_BUFFER_GLOBAL_BINDINGS(RHICmdList, PassUniformBuffer);
		
		FRHITexture* RenderTargetRHI = DrawParameters.OutRT->GetRenderTargetResource()->GetRenderTargetTexture();
		FRHIRenderPassInfo RPInfo(RenderTargetRHI, ERenderTargetActions::DontLoad_Store, RenderTargetRHI);
		RHICmdList.BeginRenderPass(RPInfo, TEXT("MyMaterialShaderPassTest"));

		// Get shaders.
		const FMaterialRenderProxy* MaterialProxy = MaterialInterface->GetRenderProxy();
		const FMaterial& MyMaterial = MaterialProxy->GetMaterialWithFallback(FeatureLevel, MaterialProxy);
		const FMaterialShaderMap* const MaterialShaderMap = MyMaterial.GetRenderingThreadShaderMap();
		TShaderRef<FMyMaterialVS> VertexShader = MaterialShaderMap->GetShader<FMyMaterialVS>();
		TShaderRef<FMyMaterialPS> PixelShader = MaterialShaderMap->GetShader<FMyMaterialPS>();

		// Set the graphic pipeline state.
		FGraphicsPipelineStateInitializer GraphicsPSOInit;
		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
		GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
		GraphicsPSOInit.PrimitiveType = PT_TriangleList;
		GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_DepthNearOrEqual>::GetRHI();
		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
		
		// Or you can use FPixelShaderUtils::InitFullscreenPipelineState to set GraphicsPSOInit
		//auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
		//FPixelShaderUtils::InitFullscreenPipelineState(RHICmdList, ShaderMap, PixelShader, GraphicsPSOInit);

		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

		// Update shader uniform parameters.
		if (DrawParameters.InTexture)
		{
			PixelShader->SetParameters_Texture(RHICmdList, DrawParameters.InTexture->TextureReference.TextureReferenceRHI);
		}
		PixelShader->SetParameters(RHICmdList, *DrawParameters.View, MaterialInterface->GetRenderProxy(), MyMaterial);

		// Draw
		FPixelShaderUtils::DrawFullscreenQuad(RHICmdList, 1);

		RHICmdList.EndRenderPass();
	}
}
