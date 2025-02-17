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

//= INCLUDES ========================
#include "Settings.h"
#include "Timer.h"
#include "Context.h"
#include <fstream>
#include "../Logging/Log.h"
#include "../FileSystem/FileSystem.h"
#include "../Rendering/Renderer.h"
#include "../Threading/Threading.h"
//===================================

//= NAMESPACES ================
using namespace std;
using namespace Spartan::Math;
//=============================

namespace _Settings
{
	ofstream fout;
	ifstream fin;
	string file_name = "Spartan.ini";

    template <class T>
    void write_setting(ofstream& fout, const string& name, T value)
    {
        fout << name << "=" << value << endl;
    }

    template <class T>
    void read_setting(ifstream& fin, const string& name, T& value)
    {
        for (string line; getline(fin, line); )
        {
            const auto first_index = line.find_first_of('=');
            if (name == line.substr(0, first_index))
            {
                const auto lastindex = line.find_last_of('=');
                const auto read_value = line.substr(lastindex + 1, line.length());
                value = static_cast<T>(stof(read_value));
                return;
            }
        }
    }
}

namespace Spartan
{
    Settings::Settings(Context* context) : ISubsystem(context)
    {
        m_context = context;
    }

    Settings::~Settings()
    {
        Reflect();
        Save();
    }

    bool Settings::Initialize()
    {
        // Acquire default settings
        Reflect();

        if (FileSystem::FileExists(_Settings::file_name))
        {
            Load();
            Map();
        }
        else
        {
            Save();
        }

        LOGF_INFO("Resolution: %dx%d", static_cast<int>(m_resolution.x), static_cast<int>(m_resolution.y));
        LOGF_INFO("FPS Limit: %f", m_fps_limit);
        LOGF_INFO("Shadow resolution: %d", m_shadow_map_resolution);
        LOGF_INFO("Anisotropy: %d", m_anisotropy);
        LOGF_INFO("Max threads: %d", m_max_thread_count);

        return true;
    }

    void Settings::Save() const
	{
		// Create a settings file
		_Settings::fout.open(_Settings::file_name, ofstream::out);

		// Write the settings
		_Settings::write_setting(_Settings::fout, "bFullScreen",           m_is_fullscreen);
		_Settings::write_setting(_Settings::fout, "bIsMouseVisible",       m_is_mouse_visible);
        _Settings::write_setting(_Settings::fout, "fResolutionWidth",      m_resolution.x);
        _Settings::write_setting(_Settings::fout, "fResolutionHeight",     m_resolution.y);
		_Settings::write_setting(_Settings::fout, "iShadowMapResolution",  m_shadow_map_resolution);
		_Settings::write_setting(_Settings::fout, "iAnisotropy",           m_anisotropy);
		_Settings::write_setting(_Settings::fout, "fFPSLimit",             m_fps_limit);
		_Settings::write_setting(_Settings::fout, "iMaxThreadCount",       m_max_thread_count);

		// Close the file.
		_Settings::fout.close();
	}

	void Settings::Load()
	{
		// Create a settings file
		_Settings::fin.open(_Settings::file_name, ifstream::in);

		float resolution_x = 0;
		float resolution_y = 0;

		// Read the settings
		_Settings::read_setting(_Settings::fin, "bFullScreen",             m_is_fullscreen);
		_Settings::read_setting(_Settings::fin, "bIsMouseVisible",         m_is_mouse_visible);
		_Settings::read_setting(_Settings::fin, "fResolutionWidth",        resolution_x);
		_Settings::read_setting(_Settings::fin, "fResolutionHeight",       resolution_y);
		_Settings::read_setting(_Settings::fin, "iShadowMapResolution",    m_shadow_map_resolution);
		_Settings::read_setting(_Settings::fin, "iAnisotropy",             m_anisotropy);
		_Settings::read_setting(_Settings::fin, "fFPSLimit",               m_fps_limit);
		_Settings::read_setting(_Settings::fin, "iMaxThreadCount",         m_max_thread_count);

		// Close the file.
		_Settings::fin.close();
	}

    void Settings::Reflect()
    {
        Renderer* renderer = m_context->GetSubsystem<Renderer>().get();

        m_fps_limit             = m_context->GetSubsystem<Timer>()->GetTargetFps();
        m_max_thread_count      = m_context->GetSubsystem<Threading>()->GetThreadCountMax();
        m_resolution            = renderer->GetResolution();   
        m_shadow_map_resolution = renderer->GetShadowResolution();
        m_anisotropy            = renderer->GetAnisotropy();
    }

    void Settings::Map()
    {
        Renderer* renderer = m_context->GetSubsystem<Renderer>().get();

        m_context->GetSubsystem<Timer>()->SetTargetFps(m_fps_limit);
        renderer->SetAnisotropy(m_anisotropy);
        renderer->SetShadowResolution(m_shadow_map_resolution);
    }
}
