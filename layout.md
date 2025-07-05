# LAYOUT

## Buffers
Camera buffer	            VkBuffer	n   Uniform buffer, fast and cheap
Output image                VkImage	    1	Storage image, rgba32f or similar
Triangle/Model buffer       VkBuffer	1	SSBO with all the geometry (Scene buffer)
Material buffer (not yet)   VkBuffer	1	
Index buffer (not yet)      VkBuffer	1	