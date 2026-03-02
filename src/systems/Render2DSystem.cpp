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

static glm::mat4 BuildSpriteModel(const Transform &t, const Sprite &s)
{
	glm::vec3 pos = t.position;

	glm::mat4 T = glm::translate(glm::mat4(1.f), pos);
	glm::mat4 R = glm::rotate(glm::mat4(1.f), t.rotationEuler.z, glm::vec3(0.f, 0.f, 1.f));

	glm::vec3 scale = t.scale;
	scale.x *= s.size.x;
	scale.y *= s.size.y;

	glm::mat4 S = glm::scale(glm::mat4(1.f), scale);

	return T * R * S;
}

void Render2DSystem::Render(Registry &registry,
							Renderer &renderer,
							Entity cameraEntity,
							int framebufferWidth,
							int framebufferHeight,
							const Mesh &quadMesh) const
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

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	std::vector<Entity> items;
	{
		auto entities = registry.ViewEntities<Transform, Sprite>();
		items.assign(entities.begin(), entities.end());
	}

	std::sort(items.begin(), items.end(), [&](Entity a, Entity b)
			  {
        const auto& sa = registry.Get<Sprite>(a);
        const auto& sb = registry.Get<Sprite>(b);
        if (sa.layer != sb.layer) return sa.layer < sb.layer;
        if (sa.orderInLayer != sb.orderInLayer) return sa.orderInLayer < sb.orderInLayer;
        const auto& ta = registry.Get<Transform>(a);
        const auto& tb = registry.Get<Transform>(b);
        return ta.position.z < tb.position.z; });

	for (Entity e : items)
	{
		const auto &t = registry.Get<Transform>(e);
		const auto &s = registry.Get<Sprite>(e);

		Material m;
		m.shader = s.shader;
		m.texture = s.texture;
		m.tint = s.tint;

		glm::mat4 model = BuildSpriteModel(t, s);
		renderer.Submit(quadMesh, m, model);
	}

	glDisable(GL_BLEND);
}