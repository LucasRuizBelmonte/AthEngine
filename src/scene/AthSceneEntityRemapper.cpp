#include "AthSceneIOInternal.h"

namespace AthSceneIO
{
	void EntityRemapper::Reserve(size_t count)
	{
		m_remap.reserve(count);
	}

	void EntityRemapper::Map(Entity source, Entity target)
	{
		m_remap[source] = target;
	}

	Entity EntityRemapper::Resolve(Entity source) const
	{
		const auto it = m_remap.find(source);
		if (it == m_remap.end())
			return source;
		return it->second;
	}
}
