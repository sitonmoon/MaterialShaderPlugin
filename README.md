# UE4 自定义MaterialShader插件
一个基于MaterialShader的自定义Shader插件。

📝 Doc：[知乎-UE4 自定义MaterialShader插件](https://zhuanlan.zhihu.com/p/701623055)

🧐 Features： 
* 同时支持PC和移动端  
* 在自定义的Pass里渲染到RT(对于PC端挂接到IRendererModule::FOnResolvedSceneColor,对于移动端挂接到IRendererModule::FOnPostOpaqueRender)  
* 使用自定义的Material Shader  
* 支持材质球编辑  
* 材质球中可以引用系统的SceneTextures  
