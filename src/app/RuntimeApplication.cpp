#include "../platform/GL.h"

#include "RuntimeApplication.h"

#include "../input/Input.h"
#include "../utils/AssetPath.h"

#include <cctype>
#include <fstream>
#include <stdexcept>
#include <string>

namespace
{
	constexpr bool kUseFixedTimestep = true;
	constexpr float kFixedDeltaSeconds = 1.0f / 60.0f;
	constexpr float kMaxFrameTimeSeconds = 0.10f;
	constexpr const char *kRuntimeStartupConfigPath = "runtime/startup.cfg";

	static std::string TrimCopy(const std::string &value)
	{
		size_t first = 0u;
		while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first])) != 0)
			++first;

		size_t last = value.size();
		while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1u])) != 0)
			--last;

		return value.substr(first, last - first);
	}

	static std::string StripOptionalQuotes(std::string value)
	{
		if (value.size() < 2u)
			return value;

		const char first = value.front();
		const char last = value.back();
		if ((first == '"' && last == '"') || (first == '\'' && last == '\''))
			return value.substr(1u, value.size() - 2u);

		return value;
	}

	static std::string LoadStartupScenePathFromConfig()
	{
		std::string configPath;
		std::string configResolveError;
		if (!AssetPath::TryResolveRuntimeFilePath(kRuntimeStartupConfigPath, configPath, configResolveError))
			throw std::runtime_error("Could not resolve runtime startup config: " + configResolveError);

		std::ifstream in(configPath);
		if (!in)
			throw std::runtime_error("Could not open runtime startup config: " + configPath);

		std::string startupScenePath;
		std::string line;
		int lineNumber = 0;
		while (std::getline(in, line))
		{
			++lineNumber;
			std::string trimmed = TrimCopy(line);
			if (trimmed.empty() || trimmed.rfind("#", 0) == 0 || trimmed.rfind("//", 0) == 0)
				continue;

			const size_t eq = trimmed.find('=');
			if (eq == std::string::npos)
				throw std::runtime_error("Invalid runtime startup config entry at line " + std::to_string(lineNumber) + ".");

			const std::string key = TrimCopy(trimmed.substr(0u, eq));
			const std::string value = StripOptionalQuotes(TrimCopy(trimmed.substr(eq + 1u)));
			if (key == "scene")
			{
				startupScenePath = value;
				continue;
			}

			throw std::runtime_error("Unknown runtime startup config key '" + key + "' at line " + std::to_string(lineNumber) + ".");
		}

		if (startupScenePath.empty())
			throw std::runtime_error("Runtime startup config is missing 'scene=<path>' in " + configPath + ".");

		std::string normalizedScenePath;
		std::string normalizeError;
		if (!AssetPath::TryNormalizeRuntimeAssetPath(startupScenePath, normalizedScenePath, normalizeError))
		{
			throw std::runtime_error("Runtime startup scene path is invalid in " + configPath +
			                         ": " + normalizeError);
		}

		return normalizedScenePath;
	}
}

RuntimeApplication::RuntimeApplication()
{
	m_Window = std::make_unique<Window>(1280, 720, "AthEngine Runtime");

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
		throw std::runtime_error("Failed to initialize GLEW");

	glEnable(GL_DEPTH_TEST);

	m_Renderer = std::make_unique<Renderer>(m_ShaderManager, m_TextureManager);
	m_Scenes = std::make_unique<RuntimeSceneManager>(
		m_ShaderManager,
		m_TextureManager,
		*m_Window->GetNative(),
		LoadStartupScenePathFromConfig());

	glfwSetInputMode(m_Window->GetNative(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	Input::AttachWindow(m_Window->GetNative());

	m_LastTime = static_cast<float>(glfwGetTime());
}

RuntimeApplication::~RuntimeApplication() noexcept
{
	try
	{
		if (m_Window && m_Window->GetNative())
			glfwMakeContextCurrent(m_Window->GetNative());
	}
	catch (...)
	{
	}

	try
	{
		m_Scenes.reset();
	}
	catch (...)
	{
	}

	try
	{
		m_Renderer.reset();
	}
	catch (...)
	{
	}

	try
	{
		m_TextureManager = TextureManager{};
	}
	catch (...)
	{
	}

	try
	{
		m_ShaderManager = ShaderManager{};
	}
	catch (...)
	{
	}
}

void RuntimeApplication::Run()
{
	float fixedAccumulator = 0.0f;

	while (!m_Window->ShouldClose())
	{
		m_Window->PollEvents();
		if (m_Window->ShouldClose())
			break;

		Input::BeginFrame();

		const float now = static_cast<float>(glfwGetTime());
		float dt = now - m_LastTime;
		m_LastTime = now;

		if (dt < 0.0f)
			dt = 0.0f;
		if (dt > kMaxFrameTimeSeconds)
			dt = kMaxFrameTimeSeconds;

		fixedAccumulator += dt;
		HandleRuntimeInput();
		if (m_Window->ShouldClose())
			break;

		if (kUseFixedTimestep)
		{
			while (fixedAccumulator >= kFixedDeltaSeconds)
			{
				m_Scenes->FixedUpdate(kFixedDeltaSeconds);
				fixedAccumulator -= kFixedDeltaSeconds;
			}
		}
		else
		{
			m_Scenes->FixedUpdate(dt);
			fixedAccumulator = 0.0f;
		}

		m_Scenes->Update(dt, now, Input::GetState());

		int width = 1;
		int height = 1;
		glfwGetFramebufferSize(m_Window->GetNative(), &width, &height);

		m_Renderer->BeginFrame(width, height);
		m_Scenes->Render3D(*m_Renderer, width, height);
		m_Scenes->Render2D(*m_Renderer, width, height);

		Input::EndFrame();
		m_Window->SwapBuffers();
	}
}

void RuntimeApplication::HandleRuntimeInput()
{
	if (Input::GetKeyDown(GLFW_KEY_ESCAPE))
		m_Window->Close();
}
