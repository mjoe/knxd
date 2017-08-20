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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <arpa/inet.h>
#include <ev.h>
#include "eibnetip.h"

/** aborts program with a printf like message */
void
die (const char *msg, ...)
{
  va_list ap;
  va_start (ap, msg);
  vprintf (msg, ap);
  printf ("\n");
  va_end (ap);

  exit (1);
}

void
HexDump (const uchar * data, int Len)
{
  for (int i = 0; i < Len; i++)
    printf (" %02X", data[i]);
  printf ("\n");
}

static void
end_me (EV_P_ ev_timer *w, int revents)
{
  die ("Request timed out");
}

static void
recv_me (EIBNetIPPacket *p1)
{
  EIBnet_DescriptionResponse resp;
  if (parseEIBnet_DescriptionResponse (*p1, resp))
    die ("Invalid description response");
  printf ("Medium: %d\nState: %d\nAddr: %s\nInstallID: %d\nSerial:",
          resp.KNXmedium, resp.devicestatus,
          FormatEIBAddr (resp.individual_addr).c_str(), resp.installid);
  HexDump (resp.serial, sizeof (resp.serial));
  printf ("Multicast-Addr: %s\nMAC:", inet_ntoa (resp.multicastaddr));
  HexDump (resp.MAC, sizeof (resp.MAC));
  printf ("Name: %s\n", resp.name);
  printf ("Optional: ");
  HexDump (resp.optional.data(), resp.optional.size());
  ITER(i, resp.services)
    printf ("Service %d Version %d\n", i->family, i->version);
  ev_break(EV_DEFAULT_ EVBREAK_ALL);
}

int
main (int ac, char *ag[])
{
  int tracelevel;
  int sport;
  int dport;
  char *a, *b, *c;
  if (ac != 2 && ac != 3)
    die ("Usage: %s ip[:dst-port[:src-port]] [tracelevel]", ag[0]);
  struct sockaddr_in saddr;
  struct sockaddr_in caddr;
  EIBNetIPSocket *sock;
  ev_timer timeout;
  ev_timer_init(&timeout, &end_me, 10.,0.);
  ev_timer_start(EV_DEFAULT_ &timeout);

  tracelevel = 0;
  if (ac == 3)
    tracelevel = atoi (ag[2]);
  a = strdup (ag[1]);
  if (!a)
    die ("out of memory");
  for (b = a; *b; b++)
    if (*b == ':')
      break;
  sport = 3672;
  if (*b == ':')
    {
      *b = 0;
      for (c = b + 1; *c; c++)
	if (*c == ':')
	  break;
      if (*c == ':')
	{
	  *c = 0;
	  sport = atoi (c + 1);
	}
      dport = atoi (b + 1);
    }
  else
    dport = 3671;

  std::string edesc = "eibnetdescribe";
  Trace t = Trace(edesc, "main");
  t.SetTraceLevel (tracelevel);

  printf ("Asking %s at port %d from port %d\n", a, dport, sport);

  if (!GetHostIP (NULL, &caddr, a))
    die ("Host not found");
  caddr.sin_port = htons (dport);
  if (!GetSourceAddress (TracePtr(new Trace(t,a)), &caddr, &saddr))
    die ("No route found");
  saddr.sin_port = htons (sport);
  sock = new EIBNetIPSocket (saddr, 0, TracePtr(new Trace(t,a)));
  sock->sendaddr = caddr;
  sock->recvaddr = caddr;

  if (!sock->init ())
    die ("IP initialisation failed");

  EIBnet_DescriptionRequest req;
  EIBNetIPPacket *p1;
  req.caddr = saddr;
  sock->on_read.set<recv_me>();

  sock->Send (req.ToPacket ());
  ev_run (EV_DEFAULT_ 0);

  return 0;
}
