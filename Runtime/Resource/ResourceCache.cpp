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

//= INCLUDES ======================
#include "ResourceCache.h"
#include "ProgressReport.h"
#include "../World/World.h"
#include "../World/Entity.h"
#include "../IO/FileStream.h"
#include "../Core/EventSystem.h"
#include "../RHI/RHI_Texture2D.h"
#include "../RHI/RHI_TextureCube.h"
//=================================

//= NAMESPACES ================
using namespace std;
using namespace Spartan::Math;
//=============================

namespace Spartan
{
	ResourceCache::ResourceCache(Context* context) : ISubsystem(context)
	{
		string data_dir = GetDataDirectory();

		// Add engine standard resource directories
		AddDataDirectory(Asset_Cubemaps,		data_dir + "cubemaps//");
		AddDataDirectory(Asset_Fonts,			data_dir + "fonts//");
		AddDataDirectory(Asset_Icons,			data_dir + "icons//");
		AddDataDirectory(Asset_Scripts,			data_dir + "scripts//");
		AddDataDirectory(Asset_ShaderCompiler,	data_dir + "shader_compiler//");	
		AddDataDirectory(Asset_Shaders,			data_dir + "shaders//");
		AddDataDirectory(Asset_Textures,		data_dir + "textures//");

		// Create project directory
		SetProjectDirectory("Project//");

		// Subscribe to events
		SUBSCRIBE_TO_EVENT(Event_World_Save,	EVENT_HANDLER(SaveResourcesToFiles));
		SUBSCRIBE_TO_EVENT(Event_World_Load,	EVENT_HANDLER(LoadResourcesFromFiles));
		SUBSCRIBE_TO_EVENT(Event_World_Unload,	EVENT_HANDLER(Clear));
	}

	ResourceCache::~ResourceCache()
	{
		// Unsubscribe from event
		UNSUBSCRIBE_FROM_EVENT(Event_World_Unload, EVENT_HANDLER(Clear));
		Clear();
	}

	bool ResourceCache::Initialize()
	{
		// Importers
		m_importer_image	= make_shared<ImageImporter>(m_context);
		m_importer_model	= make_shared<ModelImporter>(m_context);
		m_importer_font		= make_shared<FontImporter>(m_context);
		return true;
	}

	bool ResourceCache::IsCached(const string& resource_name, const Resource_Type resource_type /*= Resource_Unknown*/)
	{
		if (resource_name.empty())
		{
			LOG_ERROR_INVALID_PARAMETER();
			return false;
		}

		for (const auto& resource : m_resource_groups[resource_type])
		{
			if (resource_name == resource->GetResourceName())
				return true;
		}

		return false;
	}

	shared_ptr<IResource>& ResourceCache::GetByName(const string& name, const Resource_Type type)
	{
		for (auto& resource : m_resource_groups[type])
		{
			if (name == resource->GetResourceName())
				return resource;
		}

		return m_empty_resource;
	}

	vector<shared_ptr<IResource>> ResourceCache::GetByType(const Resource_Type type /*= Resource_Unknown*/)
	{
		vector<shared_ptr<IResource>> resources;

		if (type == Resource_Unknown)
		{
			for (const auto& resource_group : m_resource_groups)
			{
				resources.insert(resources.end(), resource_group.second.begin(), resource_group.second.end());
			}
		}
		else
		{
			resources = m_resource_groups[type];
		}

		return resources;
	}

	uint32_t ResourceCache::GetMemoryUsage(Resource_Type type /*= Resource_Unknown*/)
	{
		uint32_t size = 0;

		if (type = Resource_Unknown)
		{
			for (const auto& group : m_resource_groups)
			{
				for (const auto& resource : group.second)
				{
					if (!resource)
						continue;

					size += resource->GetMemoryUsage();
				}
			}
		}
		else
		{
			for (const auto& resource : m_resource_groups[type])
			{
				size += resource->GetMemoryUsage();
			}
		}

		return size;
	}

	void ResourceCache::SaveResourcesToFiles()
	{
		// Start progress report
		ProgressReport::Get().Reset(g_progress_resource_cache);
		ProgressReport::Get().SetIsLoading(g_progress_resource_cache, true);
		ProgressReport::Get().SetStatus(g_progress_resource_cache, "Loading resources...");

		// Create resource list file
		string file_path = GetProjectDirectoryAbsolute() + m_context->GetSubsystem<World>()->GetName() + "_resources.dat";
		auto file = make_unique<FileStream>(file_path, FileStream_Write);
		if (!file->IsOpen())
		{
			LOG_ERROR_GENERIC_FAILURE();
			return;
		}

		auto resource_count = GetResourceCount();
		ProgressReport::Get().SetJobCount(g_progress_resource_cache, resource_count);

		// Save resource count
		file->Write(resource_count);

		// Save all the currently used resources to disk
		for (const auto& resource_group : m_resource_groups)
		{
			for (const auto& resource : resource_group.second)
			{
				if (!resource->HasFilePath())
					continue;

				// Save file path
				file->Write(resource->GetResourceFilePath());
				// Save type
				file->Write(static_cast<uint32_t>(resource->GetResourceType()));
				// Save resource (to a dedicated file)
				resource->SaveToFile(resource->GetResourceFilePath());

				// Update progress
				ProgressReport::Get().IncrementJobsDone(g_progress_resource_cache);
			}
		}

		// Finish with progress report
		ProgressReport::Get().SetIsLoading(g_progress_resource_cache, false);
	}

	void ResourceCache::LoadResourcesFromFiles()
	{
		// Open resource list file
		auto file_path = GetProjectDirectoryAbsolute() + m_context->GetSubsystem<World>()->GetName() + "_resources.dat";
		auto file = make_unique<FileStream>(file_path, FileStream_Read);
		if (!file->IsOpen())
			return;
		
		// Load resource count
		auto resource_count = file->ReadAs<uint32_t>();

		for (uint32_t i = 0; i < resource_count; i++)
		{
			// Load resource file path
			auto file_path = file->ReadAs<string>();

			// Load resource type
			auto type = static_cast<Resource_Type>(file->ReadAs<uint32_t>());

			switch (type)
			{
			case Resource_Model:
				Load<Model>(file_path);
				break;
			case Resource_Material:
				Load<Material>(file_path);
				break;
			case Resource_Texture:
				Load<RHI_Texture>(file_path);
				break;
			case Resource_Texture2d:
				Load<RHI_Texture2D>(file_path);
				break;
			case Resource_TextureCube:
				Load<RHI_TextureCube>(file_path);
				break;
			}
		}
	}

	uint32_t ResourceCache::GetResourceCount(const Resource_Type type)
	{
		return static_cast<uint32_t>(GetByType(type).size());
	}

	void ResourceCache::AddDataDirectory(const Asset_Type type, const string& directory)
	{
		m_standard_resource_directories[type] = directory;
	}

	string ResourceCache::GetDataDirectory(const Asset_Type type)
	{
		for (auto& directory : m_standard_resource_directories)
		{
			if (directory.first == type)
				return directory.second;
		}

		return "";
	}

	void ResourceCache::SetProjectDirectory(const string& directory)
	{
		if (!FileSystem::DirectoryExists(directory))
		{
			FileSystem::CreateDirectory_(directory);
		}

		m_project_directory = directory;
	}

	string ResourceCache::GetProjectDirectoryAbsolute() const
	{
		return FileSystem::GetWorkingDirectory() + m_project_directory;
	}
}
