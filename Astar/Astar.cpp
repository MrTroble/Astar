#include "tgengine/TGEngine/public/TGEngine.hpp"
#include "tgengine/TGEngine/public/pipeline/buffer/Texturebuffer.hpp"
#include "tgengine/TGEngine/public/io/Resource.hpp"
#include "tgengine/TGEngine/public/gamecontent/Actor.hpp"
#include "tgengine/TGEngine/public/pipeline/buffer/UniformBuffer.hpp"
#include <array>

using namespace std;
using namespace tge::tex;
using namespace tge::io;
using namespace tge::gmc;
using namespace tge::buf;

TextureInputInfo texture;
bool textureChanged = false;

int main()
{
	initEngine();

	SamplerInputInfo inputInfo;
	inputInfo.uSamplerMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	inputInfo.vSamplerMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	inputInfo.filterMagnification = VK_FILTER_NEAREST;
	inputInfo.filterMignification = VK_FILTER_NEAREST;

	createSampler(inputInfo, &currentMap.sampler);

	createdMaterials = new Material[255];
	createdMaterials[0] = { { 1, 1, 1, 1}, 0};

	texture.data = stbi_load("canvas.png", &texture.x, &texture.y, &texture.comp, 0);

	currentMap.textures.resize(1);
	createTextures(&texture, 1, currentMap.textures.data());

	playercontroller = [](Input input) {
		if (!textureChanged)
			return;
		destroyTexture(currentMap.textures.data(), 1);

		createTextures(&texture, 1, currentMap.textures.data());
	};

	actorDescriptor.push_back({ 6, 0, 0, UINT32_MAX });
	actorProperties.push_back({ { glm::fmat4(1), 0, 0 }, 0, 0});

	BufferInputInfo bufferInputInfos[4];
	bufferInputInfos[0].flags = VK_SHADER_STAGE_ALL_GRAPHICS;
	bufferInputInfos[0].size = 6 * sizeof(uint32_t);
	bufferInputInfos[0].memoryIndex = vlibDeviceHostVisibleCoherentIndex;
	bufferInputInfos[0].bufferUsageFlag = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

	bufferInputInfos[1].flags = VK_SHADER_STAGE_ALL_GRAPHICS;
	bufferInputInfos[1].size = sizeof(float) * 4 * 4;
	bufferInputInfos[1].memoryIndex = vlibDeviceHostVisibleCoherentIndex;
	bufferInputInfos[1].bufferUsageFlag = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	bufferInputInfos[2].flags = VK_SHADER_STAGE_ALL_GRAPHICS;
	bufferInputInfos[2].size = 1;
	bufferInputInfos[2].memoryIndex = vlibDeviceLocalMemoryIndex;
	bufferInputInfos[2].bufferUsageFlag = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	bufferInputInfos[3].flags = VK_SHADER_STAGE_ALL_GRAPHICS;
	bufferInputInfos[3].size = 1;
	bufferInputInfos[3].memoryIndex = vlibDeviceHostVisibleCoherentIndex;
	bufferInputInfos[3].bufferUsageFlag = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	createBuffers(bufferInputInfos, 4, currentMap.mapBuffers);

	void* memory;
	constexpr uint32_t IDS[] = { 2, 1, 0, 3, 2, 0 };
	CHECKFAIL(vkMapMemory(device, currentMap.mapBuffers[0].memory, 0, VK_WHOLE_SIZE, 0, &memory));
	memcpy(memory, &IDS, sizeof(IDS));
	vkUnmapMemory(device, currentMap.mapBuffers[0].memory);

	constexpr float vert[] = { -1, -1, 0, 0, -1, 1, 0, 1, 1, 1, 1, 1, 1, -1, 1, 0 };
	CHECKFAIL(vkMapMemory(device, currentMap.mapBuffers[1].memory, 0, VK_WHOLE_SIZE, 0, &memory));
	memcpy(memory, &vert, sizeof(vert));
	vkUnmapMemory(device, currentMap.mapBuffers[1].memory);

	constexpr float mem[] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0.1, 0, 0, 0, 0, 1, 0, 0, 1, 1 };
	fillUniformBuffer(TRANSFORM_BUFFER, (void*)mem, sizeof(mem));

	startTGEngine();
}
