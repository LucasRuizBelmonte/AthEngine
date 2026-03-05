#pragma region Includes
#include "Render2DSystem.h"

#include "../components/Transform.h"
#include "../components/Camera.h"
#include "../components/Sprite.h"
#include "../components/Material.h"
#include "../rendering/Renderer.h"

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <algorithm>
#pragma endregion

#pragma region Function Definitions
static glm::vec3 PivotToLocalOffset(SpritePivot pivot)
{
	switch (pivot)
	{
	case SpritePivot::TopLeft:
		return {-0.5f, 0.5f, 0.f};
	case SpritePivot::Top:
		return {0.f, 0.5f, 0.f};
	case SpritePivot::TopRight:
		return {0.5f, 0.5f, 0.f};
	case SpritePivot::Left:
		return {-0.5f, 0.f, 0.f};
	case SpritePivot::Right:
		return {0.5f, 0.f, 0.f};
	case SpritePivot::BottomLeft:
		return {-0.5f, -0.5f, 0.f};
	case SpritePivot::Bottom:
		return {0.f, -0.5f, 0.f};
	case SpritePivot::BottomRight:
		return {0.5f, -0.5f, 0.f};
	case SpritePivot::Center:
	default:
		return {0.f, 0.f, 0.f};
	}
}

static glm::mat4 BuildSpriteModel(const Transform &t, const Sprite &s, float halfViewportWidth, float halfViewportHeight)
{
	glm::vec3 anchorOffset{
		t.pivot.x * halfViewportWidth,
		t.pivot.y * halfViewportHeight,
		t.pivot.z};
	glm::vec3 pos = t.worldPosition + anchorOffset;

	glm::mat4 T = glm::translate(glm::mat4(1.f), pos);
	glm::mat4 R = glm::rotate(glm::mat4(1.f), t.worldRotation.z, glm::vec3(0.f, 0.f, 1.f));
	glm::mat4 P = glm::translate(glm::mat4(1.f), -PivotToLocalOffset(s.pivot));

	glm::vec3 scale = t.worldScale;
	scale.x *= s.size.x;
	scale.y *= s.size.y;

	glm::mat4 S = glm::scale(glm::mat4(1.f), scale);

	return T * R * S * P;
}

void Render2DSystem::Render(Registry &registry,
							Renderer &renderer,
							Entity cameraEntity,
							int framebufferWidth,
							int framebufferHeight)
{
	if (cameraEntity == kInvalidEntity)
		return;

	if (!registry.Has<Camera>(cameraEntity))
		return;

	const auto &cam = registry.Get<Camera>(cameraEntity);
	if (cam.projection != ProjectionType::Orthographic)
		return;

	float aspect = (framebufferHeight == 0) ? 1.f : (float)framebufferWidth / (float)framebufferHeight;

	float halfH = cam.orthoHeight * 0.5f;
	float halfW = halfH * aspect;

	glm::mat4 proj = glm::ortho(-halfW, halfW, -halfH, halfH, cam.nearPlane, cam.farPlane);

	glm::mat4 view = glm::lookAt(
		cam.position,
		cam.position + cam.direction,
		glm::vec3(0.f, 1.f, 0.f));

	renderer.SetCamera(view, proj);
	const uint32_t quadMeshId = renderer.AcquireBuiltinQuad();
	if (quadMeshId == 0)
		return;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	registry.ViewEntities<Transform, Sprite>(m_items);

	std::sort(m_items.begin(), m_items.end(), [&](Entity a, Entity b)
			  {
        const auto& sa = registry.Get<Sprite>(a);
        const auto& sb = registry.Get<Sprite>(b);
        if (sa.layer != sb.layer) return sa.layer < sb.layer;
        if (sa.orderInLayer != sb.orderInLayer) return sa.orderInLayer < sb.orderInLayer;
        const auto& ta = registry.Get<Transform>(a);
        const auto& tb = registry.Get<Transform>(b);
        return ta.worldMatrix[3][2] < tb.worldMatrix[3][2]; });

	for (Entity e : m_items)
	{
		const auto &t = registry.Get<Transform>(e);
		const auto &s = registry.Get<Sprite>(e);

		Material m;
		m.shader = s.shader;
		m.texture = s.texture;
		m.tint = s.tint;

		const glm::mat4 model = BuildSpriteModel(t, s, halfW, halfH);
		renderer.SubmitMesh(quadMeshId, m, model);
	}

	glDisable(GL_BLEND);
}
#pragma endregion
