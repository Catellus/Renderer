@echo off
C:\VulkanSDK\1.2.135.0\Bin32\glslc.exe default.vert -o vert.spv
echo Vert compile successful
C:\VulkanSDK\1.2.135.0\Bin32\glslc.exe default.frag -o frag.spv
echo Frag compile successful
pause

