// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DrawMaterialShader.h"

class CustomMaterialShader
{
public:
	static void MaterialShaderDraw(FRHICommandListImmediate& RHICmdList, FShaderFogMaskParameters& DrawParameters);
};
