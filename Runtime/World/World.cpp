/*
Copyright(c) 2016-2019 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

//= INCLUDES ==========================
#include "World.h"
#include "Entity.h"
#include "Components/Transform.h"
#include "Components/Camera.h"
#include "Components/Light.h"
#include "Components/Script.h"
#include "Components/Environment.h"
#include "Components/AudioListener.h"
#include "../Core/Engine.h"
#include "../Core/Stopwatch.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/ProgressReport.h"
#include "../IO/FileStream.h"
#include "../Profiling/Profiler.h"
#include "../Rendering/Renderer.h"
#include "../Input/Input.h"
//=====================================

//= NAMESPACES ================
using namespace std;
using namespace Spartan::Math;
//=============================

namespace Spartan
{
	World::World(Context* context) : ISubsystem(context)
	{
		// Subscribe to events
		SUBSCRIBE_TO_EVENT(Event_World_Resolve_Pending, [this](Variant) { m_is_dirty = true; });
		SUBSCRIBE_TO_EVENT(Event_World_Stop,	        [this](Variant)	{ m_state = Idle; });
		SUBSCRIBE_TO_EVENT(Event_World_Start,	        [this](Variant)	{ m_state = Ticking; });
	}

	World::~World()
	{
		Unload();
	}

	bool World::Initialize()
	{
		m_input		= m_context->GetSubsystem<Input>().get();
		m_profiler	= m_context->GetSubsystem<Profiler>().get();

		CreateCamera();
		CreateEnvironment();
		CreateDirectionalLight();

		return true;
	}

	void World::Tick(float delta_time)
	{	
		if (m_state == Request_Loading)
		{
			m_state = Loading;
			return;
		}

		if (m_state != Ticking)
			return;

        TIME_BLOCK_START_CPU(m_profiler);

		// Tick entities
		{
			// Detect game toggling
			const auto started	= m_context->m_engine->EngineMode_IsSet(Engine_Game) && m_wasInEditorMode;
			const auto stopped	= !m_context->m_engine->EngineMode_IsSet(Engine_Game) && !m_wasInEditorMode;
			m_wasInEditorMode	= !m_context->m_engine->EngineMode_IsSet(Engine_Game);

			// Start
			if (started)
			{
				for (const auto& entity : m_entities)
				{
					entity->Start();
				}
			}

			// Stop
			if (stopped)
			{
				for (const auto& entity : m_entities)
				{
					entity->Stop();
				}
			}

			// Tick
			for (const auto& entity : m_entities)
			{
				entity->Tick(delta_time);
			}
		}

        TIME_BLOCK_END(m_profiler);

		if (m_is_dirty)
		{
            // Notify Renderer
			FIRE_EVENT_DATA(Event_World_Resolve_Complete, m_entities);
            m_is_dirty = false;
		}
	}

	void World::Unload()
	{
        // Notify any systems that the entities are about to be cleared
		FIRE_EVENT(Event_World_Unload);

        m_entities.clear();
        m_entities.shrink_to_fit();

		m_is_dirty = true;
	}

	bool World::SaveToFile(const string& filePathIn)
	{
		// Start progress report and timer
		ProgressReport::Get().Reset(g_progress_world);
		ProgressReport::Get().SetIsLoading(g_progress_world, true);
		ProgressReport::Get().SetStatus(g_progress_world, "Saving world...");
		Stopwatch timer;
	
		// Add scene file extension to the filepath if it's missing
		auto file_path = filePathIn;
		if (FileSystem::GetExtensionFromFilePath(file_path) != EXTENSION_WORLD)
		{
			file_path += EXTENSION_WORLD;
		}
		m_name = FileSystem::GetFileNameNoExtensionFromFilePath(file_path);

		// Notify subsystems that need to save data
		FIRE_EVENT(Event_World_Save);

		// Create a prefab file
		auto file = make_unique<FileStream>(file_path, FileStream_Write);
		if (!file->IsOpen())
		{
			LOG_ERROR_GENERIC_FAILURE();
			return false;
		}

		// Only save root entities as they will also save their descendants
		auto root_actors = EntityGetRoots();
		const auto root_entity_count = static_cast<uint32_t>(root_actors.size());

		ProgressReport::Get().SetJobCount(g_progress_world, root_entity_count);

		// Save root entity count
		file->Write(root_entity_count);

		// Save root entity IDs
		for (const auto& root : root_actors)
		{
			file->Write(root->GetId());
		}

		// Save root entities
		for (const auto& root : root_actors)
		{
			root->Serialize(file.get());
			ProgressReport::Get().IncrementJobsDone(g_progress_world);
		}

		// Finish with progress report and timer
		ProgressReport::Get().SetIsLoading(g_progress_world, false);
		LOG_INFO("Saving took " + to_string(static_cast<int>(timer.GetElapsedTimeMs())) + " ms");

		// Notify subsystems waiting for us to finish
		FIRE_EVENT(Event_World_Saved);

		return true;
	}

	bool World::LoadFromFile(const string& file_path)
	{
		if (!FileSystem::FileExists(file_path))
		{
			LOG_ERROR(file_path + " was not found.");
			return false;
		}

		// Thread safety: Wait for scene and the renderer to stop the entities (could do double buffering in the future)
		while (m_state != Loading || m_context->GetSubsystem<Renderer>()->IsRendering()) { m_state = Request_Loading; this_thread::sleep_for(chrono::milliseconds(16)); }

		// Start progress report and timing
		ProgressReport::Get().Reset(g_progress_world);
		ProgressReport::Get().SetIsLoading(g_progress_world, true);
		ProgressReport::Get().SetStatus(g_progress_world, "Loading world...");
		Stopwatch timer;
		
		// Unload current entities
		Unload();

		// Read all the resource file paths
		auto file = make_unique<FileStream>(file_path, FileStream_Read);
		if (!file->IsOpen())
			return false;

		m_name = FileSystem::GetFileNameNoExtensionFromFilePath(file_path);

		// Notify subsystems that need to load data
		FIRE_EVENT(Event_World_Load);

		// Load root entity count
		auto root_entity_count = file->ReadAs<uint32_t>();

		ProgressReport::Get().SetJobCount(g_progress_world, root_entity_count);

		// Load root entity IDs
		for (uint32_t i = 0; i < root_entity_count; i++)
		{
			auto& entity = EntityCreate();
			entity->SetId(file->ReadAs<uint32_t>());
		}

		// Serialize root entities
		for (uint32_t i = 0; i < root_entity_count; i++)
		{
			m_entities[i]->Deserialize(file.get(), nullptr);
			ProgressReport::Get().IncrementJobsDone(g_progress_world);
		}

		m_is_dirty	= true;
		m_state		= Ticking;
		ProgressReport::Get().SetIsLoading(g_progress_world, false);	
		LOG_INFO("Loading took " + to_string(static_cast<int>(timer.GetElapsedTimeMs())) + " ms");	

		FIRE_EVENT(Event_World_Loaded);
		return true;
	}

    shared_ptr<Spartan::Entity>& World::EntityCreate(bool is_active /*= true*/)
    {
        auto& entity = m_entities.emplace_back(make_shared<Entity>(m_context));
        entity->SetActive(is_active);
        return entity;
    }

    shared_ptr<Entity>& World::EntityAdd(const shared_ptr<Entity>& entity)
    {
        static shared_ptr<Entity> empty;

		if (!entity)
			return empty;

		return m_entities.emplace_back(entity);
	}

	bool World::EntityExists(const shared_ptr<Entity>& entity)
	{
		if (!entity)
			return false;

		return EntityGetById(entity->GetId()) != nullptr;
	}

	// Removes an entity and all of it's children
	void World::EntityRemove(const shared_ptr<Entity>& entity)
	{
		if (!entity)
			return;

		// Remove any descendants
		auto children = entity->GetTransform_PtrRaw()->GetChildren();
		for (const auto& child : children)
		{
			EntityRemove(child->GetEntity_PtrShared());
		}

		// Keep a reference to it's parent (in case it has one)
		auto parent = entity->GetTransform_PtrRaw()->GetParent();

		// Remove this entity
		for (auto it = m_entities.begin(); it < m_entities.end();)
		{
			const auto temp = *it;
			if (temp->GetId() == entity->GetId())
			{
				it = m_entities.erase(it);
				break;
			}
			++it;
		}

		// If there was a parent, update it
		if (parent)
		{
			parent->AcquireChildren();
		}

        m_is_dirty = true;
	}

	vector<shared_ptr<Entity>> World::EntityGetRoots()
	{
		vector<shared_ptr<Entity>> root_entities;
		for (const auto& entity : m_entities)
		{
			if (entity->GetTransform_PtrRaw()->IsRoot())
			{
				root_entities.emplace_back(entity);
			}
		}

		return root_entities;
	}

	const shared_ptr<Entity>& World::EntityGetByName(const string& name)
	{
		for (const auto& entity : m_entities)
		{
			if (entity->GetName() == name)
				return entity;
		}

        static shared_ptr<Entity> empty;
		return empty;
	}

	const shared_ptr<Entity>& World::EntityGetById(const uint32_t id)
	{
		for (const auto& entity : m_entities)
		{
			if (entity->GetId() == id)
				return entity;
		}

        static shared_ptr<Entity> empty;
		return empty;
	}

	shared_ptr<Entity>& World::CreateEnvironment()
	{
		auto& environment = EntityCreate();
		environment->SetName("Environment");
		environment->AddComponent<Environment>();

		return environment;
	}
	shared_ptr<Entity> World::CreateCamera()
	{
		auto resource_mng		= m_context->GetSubsystem<ResourceCache>();
		const auto dir_scripts	= resource_mng->GetDataDirectory(Asset_Scripts);

		auto entity = EntityCreate();
		entity->SetName("Camera");
		entity->AddComponent<Camera>();
		entity->AddComponent<AudioListener>();
		//entity->AddComponent<Script>()->SetScript(dir_scripts + "MouseLook.as");
		//entity->AddComponent<Script>()->SetScript(dir_scripts + "FirstPersonController.as");
		entity->GetTransform_PtrRaw()->SetPositionLocal(Vector3(0.0f, 1.0f, -5.0f));

		return entity;
	}

	shared_ptr<Entity>& World::CreateDirectionalLight()
	{
		auto& light = EntityCreate();
		light->SetName("DirectionalLight");
		light->GetTransform_PtrRaw()->SetRotationLocal(Quaternion::FromEulerAngles(30.0f, 30.0, 0.0f));
		light->GetTransform_PtrRaw()->SetPosition(Vector3(0.0f, 10.0f, 0.0f));

		auto light_comp = light->AddComponent<Light>().get();
		light_comp->SetLightType(LightType_Directional);
		light_comp->SetIntensity(1.5f);

		return light;
	}
}
