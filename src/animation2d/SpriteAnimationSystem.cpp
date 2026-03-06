#pragma region Includes
#include "SpriteAnimationSystem.h"

#include "Animation2DLibrary.h"
#include "SpriteAnimator.h"
#include "../components/Sprite.h"
#include "../resources/TextureManager.h"
#include "../rendering/Texture.h"

#include <algorithm>
#include <cmath>
#pragma endregion

#pragma region File Scope
namespace
{
	static int WrapFrameIndex(int frame, int frameCount)
	{
		if (frameCount <= 0)
			return 0;

		int wrapped = frame % frameCount;
		if (wrapped < 0)
			wrapped += frameCount;
		return wrapped;
	}

	static bool BuildGridFrameUV(const Texture &texture,
								 const SpriteAnimator &animator,
								 int frameIndex,
								 int &outFrameCount,
								 glm::vec4 &outUv)
	{
		const int textureWidth = texture.GetWidth();
		const int textureHeight = texture.GetHeight();
		if (textureWidth <= 0 || textureHeight <= 0)
			return false;

		const int columns = std::max(1, animator.columns);
		const int rows = std::max(1, animator.rows);
		const int gapX = std::max(0, animator.gapX);
		const int gapY = std::max(0, animator.gapY);
		const int marginX = std::max(0, animator.marginX);
		const int marginY = std::max(0, animator.marginY);
		const int startX = std::clamp(animator.startIndexX, 0, columns - 1);
		const int startY = std::clamp(animator.startIndexY, 0, rows - 1);

		const int autoCellWidth = (textureWidth - (2 * marginX) - ((columns - 1) * gapX)) / columns;
		const int autoCellHeight = (textureHeight - (2 * marginY) - ((rows - 1) * gapY)) / rows;
		const int cellWidth = (animator.cellWidth > 0) ? animator.cellWidth : autoCellWidth;
		const int cellHeight = (animator.cellHeight > 0) ? animator.cellHeight : autoCellHeight;
		if (cellWidth <= 0 || cellHeight <= 0)
			return false;

		const int startFlat = (startY * columns) + startX;
		const int totalCells = columns * rows;
		const int remainingCells = std::max(0, totalCells - startFlat);
		if (remainingCells <= 0)
			return false;

		outFrameCount = (animator.frameCount > 0)
							? std::min(animator.frameCount, remainingCells)
							: remainingCells;
		if (outFrameCount <= 0)
			return false;

		const int clampedFrame = std::clamp(frameIndex, 0, outFrameCount - 1);
		const int sheetIndex = startFlat + clampedFrame;
		const int col = sheetIndex % columns;
		const int row = sheetIndex / columns;

		const int pixelX = marginX + col * (cellWidth + gapX);
		const int pixelY = marginY + row * (cellHeight + gapY);
		const float u0 = static_cast<float>(pixelX) / static_cast<float>(textureWidth);
		const float v0 = static_cast<float>(pixelY) / static_cast<float>(textureHeight);
		const float u1 = static_cast<float>(pixelX + cellWidth) / static_cast<float>(textureWidth);
		const float v1 = static_cast<float>(pixelY + cellHeight) / static_cast<float>(textureHeight);

		outUv = glm::vec4{
			std::clamp(u0, 0.f, 1.f),
			std::clamp(v0, 0.f, 1.f),
			std::clamp(u1, 0.f, 1.f),
			std::clamp(v1, 0.f, 1.f)};
		return true;
	}
}
#pragma endregion

#pragma region Function Definitions
void SpriteAnimationSystem::Update(Registry &registry, const Animation2DLibrary &library, TextureManager &textureManager, float dt)
{
	m_frameEvents.clear();
	registry.ViewEntities<Sprite, SpriteAnimator>(m_items);

	for (Entity e : m_items)
	{
		Sprite &sprite = registry.Get<Sprite>(e);
		SpriteAnimator &animator = registry.Get<SpriteAnimator>(e);

		const SpriteAnimationClip *clip = animator.clipId.empty() ? nullptr : library.FindClip(animator.clipId);
		const bool useClip = (clip != nullptr && !clip->framesUV.empty());

		int frameCount = 0;
		float fps = animator.fps;
		bool effectiveLoop = animator.loop;
		if (useClip)
		{
			frameCount = static_cast<int>(clip->framesUV.size());
			fps = clip->fps;
			effectiveLoop = (animator.loop && clip->loop);
		}
		else
		{
			Texture *texture = textureManager.Get(sprite.texture);
			glm::vec4 initialUv{};
			if (!texture || !BuildGridFrameUV(*texture, animator, 0, frameCount, initialUv))
				continue;
		}

		if (frameCount <= 0)
			continue;

		if (fps <= 0.f)
		{
			animator.currentFrame = 0;
			animator.playing = false;
			if (useClip)
			{
				sprite.uv = clip->framesUV[0];
			}
			else
			{
				Texture *texture = textureManager.Get(sprite.texture);
				glm::vec4 uv{};
				if (texture && BuildGridFrameUV(*texture, animator, 0, frameCount, uv))
					sprite.uv = uv;
			}
			continue;
		}

		const int previousFrame = std::clamp(animator.currentFrame, 0, frameCount - 1);

		if (animator.playing)
			animator.time += (dt * animator.speed);

		int frame = static_cast<int>(std::floor(animator.time * fps));
		if (effectiveLoop)
		{
			frame = WrapFrameIndex(frame, frameCount);
			const float duration = static_cast<float>(frameCount) / fps;
			if (duration > 0.f)
			{
				animator.time = std::fmod(animator.time, duration);
				if (animator.time < 0.f)
					animator.time += duration;
			}
		}
		else
		{
			const int lastFrame = frameCount - 1;
			if (frame > lastFrame)
			{
				frame = lastFrame;
				animator.playing = false;
				animator.time = static_cast<float>(lastFrame) / fps;
			}
			else if (frame < 0)
			{
				frame = 0;
				animator.playing = false;
				animator.time = 0.f;
			}
		}

		animator.currentFrame = frame;
		if (useClip)
		{
			sprite.uv = clip->framesUV[frame];
		}
		else
		{
			Texture *texture = textureManager.Get(sprite.texture);
			glm::vec4 uv{};
			if (!texture || !BuildGridFrameUV(*texture, animator, frame, frameCount, uv))
				continue;
			sprite.uv = uv;
		}

		if (frame != previousFrame)
		{
			m_frameEvents.push_back(FrameEvent{
				e,
				animator.clipId,
				frame});
		}
	}
}

const std::vector<SpriteAnimationSystem::FrameEvent> &SpriteAnimationSystem::GetFrameEvents() const
{
	return m_frameEvents;
}

void SpriteAnimationSystem::ClearFrameEvents()
{
	m_frameEvents.clear();
}
#pragma endregion
