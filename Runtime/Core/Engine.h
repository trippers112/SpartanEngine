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

#pragma once

//= INCLUDES ==========
#include "EngineDefs.h"
#include <memory>
//=====================

namespace Spartan
{
	class Context;

    struct WindowData
    {
        void* handle        = nullptr;
        void* instance      = nullptr;
        uint32_t message    = 0;
        float width         = 0;
        float height        = 0;
        int64_t wparam      = 0;
        uint64_t lparam     = 0;
    };

	enum Engine_Mode : uint32_t
	{
		Engine_Physics	= 1UL << 0, // Should the physics tick?	
		Engine_Game		= 1UL << 1,	// Is the engine running in game or editor mode?
	};

	class SPARTAN_CLASS Engine
	{
	public:
		Engine(const WindowData& window_data);
		~Engine();

		// Performs a simulation cycle
		void Tick();

		//  Flags
		auto EngineMode_GetAll()					        { return m_flags; }
		void EngineMode_SetAll(const uint32_t flags)	    { m_flags = flags; }
		void EngineMode_Enable(const Engine_Mode flag)	    { m_flags |= flag; }
		void EngineMode_Disable(const Engine_Mode flag)	    { m_flags &= ~flag; }
		void EngineMode_Toggle(const Engine_Mode flag)	    { m_flags = !EngineMode_IsSet(flag) ? m_flags | flag : m_flags & ~flag;}
		bool EngineMode_IsSet(const Engine_Mode flag) const	{ return m_flags & flag; }

        // Window
        WindowData& GetWindowData()                 { return m_window_data; }
        void SetWindowData(WindowData& window_data) { m_window_data = window_data; }

        auto GetContext() const { return m_context.get(); }

	private:
        WindowData m_window_data;
        uint32_t m_flags            = 0;
		std::shared_ptr<Context> m_context;
	};
}
