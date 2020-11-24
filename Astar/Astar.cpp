#include "tgengine/TGEngine/public/TGEngine.hpp"
#include "tgengine/TGEngine/public/pipeline/buffer/Texturebuffer.hpp"
#include "tgengine/TGEngine/public/io/Resource.hpp"
#include "tgengine/TGEngine/public/gamecontent/Actor.hpp"
#include "tgengine/TGEngine/public/pipeline/buffer/UniformBuffer.hpp"
#include "tgengine/TGEngine/public/pipeline/CommandBuffer.hpp"
#include <array>
#include <thread>
#include <vector>

using namespace std;
using namespace tge::tex;
using namespace tge::io;
using namespace tge::gmc;
using namespace tge::buf;

TextureInputInfo texture;
bool textureChanged = false;
bool wait = true;
bool lock = false;

#define RUN
#ifdef RUN
#define WAIT() textureChanged = true; while(textureChanged) continue
#else
#define WAIT() textureChanged = true; wait = false; while(!wait) continue
#endif

inline uint8_t* getEntry(const size_t x, const size_t y) {
	return texture.data + ((y * texture.x + x) * 4);
}

inline uint8_t* getEntry(const glm::vec2 pos) {
	return getEntry(pos.x, pos.y);
}

constexpr uint8_t white[] = { 0xFF, 0xFF, 0xFF, 0xFF };
constexpr uint8_t red[] = { 0xFF, 0, 0, 0xFF };
constexpr uint8_t green[] = { 0, 0xFF, 0, 0xFF };
constexpr uint8_t blue[] = { 0, 0, 0xFF, 0xFF };
constexpr uint8_t black[] = { 0, 0, 0, 0xFF };

inline void setColor(uint8_t* ptr, const uint8_t* color) {
	memcpy(ptr, color, 4);
}

inline bool isColor(const uint8_t* ptr, const uint8_t* color) {
	return memcmp(ptr, color, 4) == 0;
}

inline bool isWhite(const uint8_t* ptr) {
	return isColor(ptr, white);
}

inline bool isBlack(const uint8_t* ptr) {
	return isColor(ptr, black);
}

struct NavData;

struct NavPath {
	uint32_t next = -1;
	uint32_t heuristic = -1;
};

struct NavData {
	NavPath possiblePaths[4];
};

inline void getColorFromHeuristics(const NavData& data, uint8_t* colorout) {
	uint32_t color = 0;
	for (uint32_t i = 0; i < sizeof(data.possiblePaths) / sizeof(NavPath); i++) {
		const uint32_t heur = data.possiblePaths[i].heuristic;
		if (heur == -1) continue;
		color += heur;
	}
	float delta = color / (float)texture.x * 2.0f;
	colorout[0] = 0xFF ;
	colorout[1] = 0xFF * delta;
	colorout[2] = 0xFF * (1 - delta);
	colorout[3] = 0xFF;
}

static void runAstar() {
	WAIT();


	glm::vec2 startpoint = { 0, 0 };
	for (size_t i = 0; i < texture.x; i++) {
		uint8_t* ptr = getEntry(i, 0);
		if (isWhite(ptr)) {
			memcpy(ptr, green, 4);
			startpoint.x = i;
			break;
		}
	}

	WAIT();

	glm::vec2 endpoint = { 0, texture.y };
	for (size_t i = 0; i < texture.x; i++) {
		uint8_t* ptr = getEntry(i, texture.y - 1);
		if (isWhite(ptr)) {
			memcpy(ptr, blue, 4);
			endpoint.x = i;
			break;
		}
	}

	WAIT();

	std::vector<glm::vec2> candidatePositions;
	const size_t predictionSize = texture.y * texture.x / 2;
	candidatePositions.reserve(predictionSize);

	struct Directions {
		bool right;
		bool left;
		bool up;
		bool down;
	};
	std::vector<Directions> candidateDirections;
	candidateDirections.reserve(predictionSize);

	for (size_t y = 1; y < texture.y - 1; y++) {
		for (size_t x = 1; x < texture.x - 1; x++) {
			uint8_t* ptr = getEntry(x, y);
			if (!isWhite(ptr)) continue;
			const bool right = !isBlack(ptr + 4);
			const bool left = !isBlack(ptr - 4);
			const bool up = !isBlack(getEntry(x, y + 1));
			const bool down = !isBlack(getEntry(x, y - 1));
			if (((right || left) && (up || down) ||
				(((int)right + (int)left + (int)up + (int)down) == 1))) {
				memcpy(ptr, red, 4);
				candidatePositions.push_back({ x, y });
				candidateDirections.push_back({ right, left, up, down});
			}
		}
		WAIT();
	}

	WAIT();

	std::vector<NavData> navpath;
	navpath.reserve(predictionSize);

	for (size_t i = 0; i < candidatePositions.size(); i++) {
		NavData navdata;
		const glm::vec2 pos = candidatePositions[i];
		const Directions dir = candidateDirections[i];

		const auto begin = candidatePositions.begin();
		const auto end = candidatePositions.end();

		for (uint32_t j = 0; j < candidatePositions.size(); j++) {
			const glm::vec2 p = candidatePositions[j];
			const bool onX = p.x == pos.x;
			const bool onY = p.y == pos.y;
			if (!onX && !onY) continue;
			int diffx = p.x - pos.x;
			int diffy = p.y - pos.y;
			if (dir.right && diffx > 0 && navdata.possiblePaths[0].heuristic > diffx)
				navdata.possiblePaths[0] = { j, (uint32_t)diffx };
			if (dir.left && diffx < 0 && navdata.possiblePaths[1].heuristic >(diffx *= -1))
				navdata.possiblePaths[1] = { j, (uint32_t)diffx };
			if (dir.up && diffy > 0 && navdata.possiblePaths[2].heuristic > diffy)
				navdata.possiblePaths[2] = { j, (uint32_t)diffy };
			if (dir.down && diffy < 0 && navdata.possiblePaths[3].heuristic >(diffy *= -1))
				navdata.possiblePaths[3] = { j, (uint32_t)diffy };
		}
		uint8_t colorHeur[4];
		getColorFromHeuristics(navdata, colorHeur);
		setColor(getEntry(pos), colorHeur);
		navpath.push_back(navdata);
		WAIT();
	}

	WAIT();
}

int main() {
	initEngine();

	SamplerInputInfo inputInfo;
	inputInfo.uSamplerMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	inputInfo.vSamplerMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	inputInfo.filterMagnification = VK_FILTER_NEAREST;
	inputInfo.filterMignification = VK_FILTER_NEAREST;

	createSampler(inputInfo, &currentMap.sampler);

	createdMaterials = new Material[255];
	createdMaterials[0] = { { 1, 1, 1, 1}, 0 };

	texture.data = stbi_load("canvas.png", &texture.x, &texture.y, &texture.comp, 0);

	currentMap.textures.resize(1);
	createTextures(&texture, 1, currentMap.textures.data());

	std::thread thread(runAstar);
	thread.detach();

	playercontroller = [](Input input) {
		if (input.inputX == 0) lock = false;
		if (lock == true)
			return;
		if (input.inputX > 0 && wait != true) {
			lock = true;
			wait = true;
		}

		if (!textureChanged)
			return;

		textureChanged = false;
		destroyTexture(currentMap.textures.data(), 1);

		createTextures(&texture, 1, currentMap.textures.data());

		fillCommandBuffer();
	};

	actorDescriptor.push_back({ 6, 0, 0, UINT32_MAX });
	actorProperties.push_back({ { glm::fmat4(1), 0, 0 }, 0, 0 });

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
