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

//= INCLUDES =========================
#include "Engine.h"
#include "Timer.h"
#include "EventSystem.h"
#include "Settings.h"
#include "../Audio/Audio.h"
#include "../Input/Input.h"
#include "../Physics/Physics.h"
#include "../Profiling/Profiler.h"
#include "../Rendering/Renderer.h"
#include "../Resource/ResourceCache.h"
#include "../Scripting/Scripting.h"
#include "../Threading/Threading.h"
#include "../World/World.h"
#include "../Math/MathHelper.h"
//====================================

//= NAMESPACES ===============
using namespace std;
using namespace Spartan::Math;
//============================

namespace Spartan
{
	Engine::Engine(const WindowData& window_data)
	{
        // Window
        m_window_data = window_data;

        // Flags
        m_flags |= Engine_Physics;
        m_flags |= Engine_Game;

        // Create context
		m_context = make_shared<Context>();
        m_context->m_engine = this;

		// Register subsystems     
        m_context->RegisterSubsystem<Timer>(Tick_Variable);
		m_context->RegisterSubsystem<ResourceCache>(Tick_Variable);		
		m_context->RegisterSubsystem<Threading>(Tick_Variable);			
		m_context->RegisterSubsystem<Audio>(Tick_Variable);
        m_context->RegisterSubsystem<Physics>(Tick_Variable); // integrates internally
        m_context->RegisterSubsystem<Input>(Tick_Smoothed);
		m_context->RegisterSubsystem<Scripting>(Tick_Smoothed);
        m_context->RegisterSubsystem<Renderer>(Tick_Smoothed);
		m_context->RegisterSubsystem<World>(Tick_Smoothed);      
        m_context->RegisterSubsystem<Profiler>(Tick_Variable);
        m_context->RegisterSubsystem<Settings>(Tick_Variable);
             	
        // Initialize global/static subsystems
        FileSystem::Initialize();

		// Initialize above subsystems
		m_context->Initialize();
	}

	Engine::~Engine()
	{
		EventSystem::Get().Clear(); // this must become a subsystem
	}

	void Engine::Tick()
	{
        Timer* timer = m_context->GetSubsystem<Timer>().get();
        m_context->Tick(Tick_Variable, static_cast<float>(timer->GetDeltaTimeSec()));
        m_context->Tick(Tick_Smoothed, static_cast<float>(timer->GetDeltaTimeSmoothedSec()));
	}
}
