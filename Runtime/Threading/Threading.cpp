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

//= INCLUDES ================
#include "Threading.h"
#include "../Core/Settings.h"
//===========================

//= NAMESPACES =====
using namespace std;
//==================

namespace Spartan
{
	Threading::Threading(Context* context) : ISubsystem(context)
	{
		m_stopping		= false;
        m_thread_max    = thread::hardware_concurrency();
		m_thread_count	= m_thread_max - 1;

		for (uint32_t i = 0; i < m_thread_count; i++)
		{
			m_threads.emplace_back(thread(&Threading::Invoke, this));
		}
		LOGF_INFO("%d threads have been created", m_thread_count);
	}

	Threading::~Threading()
	{
		// Put unique lock on task mutex.
		unique_lock<mutex> lock(m_tasksMutex);

		// Set termination flag to true.
		m_stopping = true;

		// Unlock the mutex
		lock.unlock();

		// Wake up all threads.
		m_conditionVar.notify_all();

		// Join all threads.
		for (auto& thread : m_threads)
		{
			thread.join();
		}

		// Empty worker threads.
		m_threads.clear();
	}

	void Threading::Invoke()
	{
		shared_ptr<Task> task;
		while (true)
		{
			// Lock tasks mutex
			unique_lock<mutex> lock(m_tasksMutex);

			// Check condition on notification
			m_conditionVar.wait(lock, [this] { return !m_tasks.empty() || m_stopping; });

			// If m_stopping is true, it's time to shut everything down
			if (m_stopping && m_tasks.empty())
				return;

			// Get next task in the queue.
			task = m_tasks.front();

			// Remove it from the queue.
			m_tasks.pop();

			// Unlock the mutex
			lock.unlock();

			// Execute the task.
			task->Execute();
		}
	}
}
