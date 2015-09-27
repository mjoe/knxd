/*
    EIBD eib bus access and management daemon
    Copyright (C) 2005-2011 Martin Koegler <mkoegler@auto.tuwien.ac.at>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <unistd.h>
#include "server.h"
#include "client.h"


BaseServer::BaseServer (Layer3 * layer3, Trace * tr)
	: Layer2virtual (layer3, tr)
{
}

BaseServer::~BaseServer ()
{
  TRACEPRINTF (t, 8, this, "StopBaseServer");
  Stop ();
  if (l3)
    l3->deregisterServer (this);
}

bool
BaseServer::init ()
{
  l3->registerServer (this);
  return true;
}

Server::~Server ()
{
  TRACEPRINTF (t, 8, this, "StopServer");
  Stop ();
  for (unsigned int i = 0; i < connections (); i++)
    connections[i]->StopDelete ();
  while (connections () != 0)
    pth_yield (0);

  if (fd != -1)
    close (fd);
}

bool
Server::deregister (ClientConnection * con)
{
  for (unsigned i = 0; i < connections (); i++)
    if (connections[i] == con)
      {
	connections.deletepart (i, 1);
	return 1;
      }
  return 0;
}

Server::Server (Layer3 * layer3, Trace * tr)
    : BaseServer (layer3, tr)
{
  fd = -1;
}

bool
Server::init ()
{
  if (fd == -1)
    return false;
  return BaseServer::init();
}

void
Server::Run (pth_sem_t * stop1)
{
  pth_event_t stop = pth_event (PTH_EVENT_SEM, stop1);
  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      int cfd;
      cfd = pth_accept_ev (fd, 0, 0, stop);
      if (cfd != -1)
	{
	  TRACEPRINTF (t, 8, this, "New Connection");
	  setupConnection (cfd);
	  ClientConnection *c = new ClientConnection (this, l3, t, cfd);
	  connections.setpart (&c, connections (), 1);
	  c->Start ();
	}
    }
  pth_event_free (stop, PTH_FREE_THIS);
}

void
Server::setupConnection (int cfd UNUSED)
{
}
