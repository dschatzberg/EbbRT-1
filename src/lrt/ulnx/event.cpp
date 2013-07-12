/*
  EbbRT: Distributed, Elastic, Runtime
  Copyright (C) 2013 SESA Group, Boston University

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <sys/epoll.h>

#include <cerrno>
#include <cstdio>

#include "ebb/EventManager/EventManager.hpp"
#include "lrt/event.hpp"
#include "lrt/ulnx/init.hpp"

void
ebbrt::lrt::event::_event_interrupt(uint8_t interrupt)
{
  event_manager->HandleInterrupt(interrupt);
}

void
ebbrt::lrt::event::register_fd(int fd, uint32_t events, uint8_t interrupt)
{
#ifndef __bg__
  struct epoll_event event;
  event.events = events;
  event.data.u32 = interrupt;
  if (epoll_ctl(active_context->epoll_fd_, EPOLL_CTL_ADD, fd, &event) == -1) {
    throw std::runtime_error("epoll_ctl failed");
  }
#else
  assert((events | EPOLLIN) || (events | EPOLLOUT));
  short poll_events = 0;
  if (events | EPOLLIN) {
    poll_events |= POLLIN;
  }
  if (events | EPOLLOUT) {
    poll_events |= POLLOUT;
  }

  struct pollfd pfd;
  pfd.fd = fd;
  pfd.events = poll_events;

  active_context->fds_.push_back(pfd);
  active_context->interrupts_.push_back(interrupt);
#endif
}

#ifdef __bg__
void
ebbrt::lrt::event::register_function(std::function<int()> func)
{
  active_context->funcs_.push_back(func);
}
#endif

void
ebbrt::lrt::event::_event_altstack_push(uintptr_t val)
{
  active_context->altstack_.push(val);
}

uintptr_t
ebbrt::lrt::event::_event_altstack_pop()
{
  uintptr_t ret = active_context->altstack_.top();
  active_context->altstack_.pop();
  return ret;
}

ebbrt::lrt::event::Location
ebbrt::lrt::event::get_location()
{
  return active_context->location_;
}

unsigned
ebbrt::lrt::event::get_max_contexts()
{
  return 256;
}
