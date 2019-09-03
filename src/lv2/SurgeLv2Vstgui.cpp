#include "SurgeLv2Vstgui.h"
#include <algorithm>
#include <memory>
#include <mutex>
#if LINUX
#include <dlfcn.h>
#include <poll.h>
#endif

#if LINUX
void Lv2IdleRunLoop::execIdle()
{
   std::chrono::steady_clock::time_point tick = std::chrono::steady_clock::now();

   for (Event& ev : fEvents)
   {
      if (!ev.alive)
         continue;

// TODO LV2: fix me, XCB descriptor polling not working at this point
#if 0
        pollfd pfd = {};
        pfd.fd = ev.fd;
        pfd.events = POLLIN|POLLERR|POLLHUP;
        if (poll(&pfd, 1, 0) > 0)
#endif
      {
         ev.handler->onEvent();
      }
   }

   for (Timer& tm : fTimers)
   {
      if (!tm.alive)
         continue;

      if (tm.lastTickValid)
      {
         std::chrono::steady_clock::duration duration = tick - tm.lastTick;
         tm.counter += std::chrono::duration_cast<std::chrono::microseconds>(duration);
         if (tm.counter >= tm.interval)
         {
            tm.handler->onTimer();
            tm.counter = std::min(tm.counter - tm.interval, tm.interval);
         }
      }
      tm.lastTick = tick;
      tm.lastTickValid = true;
   }

   garbageCollectDeadHandlers<Event>(fEvents);
   garbageCollectDeadHandlers<Timer>(fTimers);
}

bool Lv2IdleRunLoop::registerEventHandler(int fd, VSTGUI::X11::IEventHandler* handler)
{
   // fprintf(stderr, "registerEventHandler %d %p\n", fd, handler);

   Event ev;
   ev.fd = fd;
   ev.handler = handler;
   ev.alive = true;
   fEvents.push_back(ev);

   return true;
}

bool Lv2IdleRunLoop::unregisterEventHandler(VSTGUI::X11::IEventHandler* handler)
{
   // fprintf(stderr, "unregisterEventHandler %p\n", handler);

   auto it = std::find_if(fEvents.begin(), fEvents.end(), [handler](const Event& ev) -> bool {
      return ev.handler == handler && ev.alive;
   });

   if (it != fEvents.end())
      it->alive = false;

   return true;
}

bool Lv2IdleRunLoop::registerTimer(uint64_t interval, VSTGUI::X11::ITimerHandler* handler)
{
   // fprintf(stderr, "registerTimer %lu %p\n", interval, handler);

   Timer tm;
   tm.interval = std::chrono::milliseconds(interval);
   tm.counter = std::chrono::microseconds(0);
   tm.lastTickValid = false;
   tm.handler = handler;
   tm.alive = true;
   fTimers.push_back(tm);

   return true;
}

bool Lv2IdleRunLoop::unregisterTimer(VSTGUI::X11::ITimerHandler* handler)
{
   // fprintf(stderr, "unregisterTimer %p\n", handler);

   auto it = std::find_if(fTimers.begin(), fTimers.end(), [handler](const Timer& tm) -> bool {
      return tm.handler == handler && tm.alive;
   });

   if (it != fTimers.end())
      it->alive = false;

   return true;
}

template <class T> void Lv2IdleRunLoop::garbageCollectDeadHandlers(std::list<T>& handlers)
{
   auto pos = handlers.begin();
   auto end = handlers.end();

   while (pos != end)
   {
      auto curPos = pos++;
      if (!curPos->alive)
         handlers.erase(curPos);
   }
}
#endif

///
#if LINUX
namespace VSTGUI
{
void* soHandle = nullptr;

static volatile bool soHandleInitialized = false;
static std::mutex soHandleMutex;

struct Dl_handle_deleter
{
   void operator()(void* x) const noexcept
   {
      dlclose(x);
   }
};
static std::unique_ptr<void, Dl_handle_deleter> soHandlePointer;
} // namespace VSTGUI

void VSTGUI::initializeSoHandle()
{
   if (VSTGUI::soHandleInitialized)
      return;

   std::lock_guard<std::mutex> lock(VSTGUI::soHandleMutex);
   if (VSTGUI::soHandleInitialized)
      return;

   Dl_info info;
   if (dladdr((void*)&lv2ui_descriptor, &info))
   {
      VSTGUI::soHandle = dlopen(info.dli_fname, RTLD_LAZY);
      VSTGUI::soHandlePointer.reset(VSTGUI::soHandle);
   }
   VSTGUI::soHandleInitialized = true;
}
#endif
