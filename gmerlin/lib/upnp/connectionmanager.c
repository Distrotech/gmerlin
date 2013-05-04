/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/

#include <config.h>
#include <upnp_service.h>
#include <string.h>

/* Initialize service description */

static void init_service_desc(bg_upnp_service_desc_t * d)
  {
  bg_upnp_sv_val_t  val;
  bg_upnp_sv_desc_t * sv;
  bg_upnp_sa_desc_t * sa;

  /* State variables */
  
  bg_upnp_service_desc_add_sv(d, "SourceProtocolInfo",
                              BG_UPNP_SV_EVENT, BG_UPNP_SV_STRING);
  bg_upnp_service_desc_add_sv(d, "SinkProtocolInfo",
                              BG_UPNP_SV_EVENT, BG_UPNP_SV_STRING);
  bg_upnp_service_desc_add_sv(d, "CurrentConnectionIDs",
                              BG_UPNP_SV_EVENT, BG_UPNP_SV_STRING);

  sv = bg_upnp_service_desc_add_sv(d, "A_ARG_TYPE_ConnectionStatus",
                                   BG_UPNP_SV_ARG_ONLY,
                                   BG_UPNP_SV_STRING);
  
  val.s = "OK";
  bg_upnp_sv_desc_add_allowed(sv, &val);
  val.s = "ContentFormatMismatch";
  bg_upnp_sv_desc_add_allowed(sv, &val);
  val.s = "InsufficientBandwidth";
  bg_upnp_sv_desc_add_allowed(sv, &val);
  val.s = "UnreliableChannel";
  bg_upnp_sv_desc_add_allowed(sv, &val);
  val.s = "Unknown";
  bg_upnp_sv_desc_add_allowed(sv, &val);

  bg_upnp_service_desc_add_sv(d, "A_ARG_TYPE_ConnectionManager",
                              BG_UPNP_SV_ARG_ONLY,
                              BG_UPNP_SV_STRING);
  
  sv = bg_upnp_service_desc_add_sv(d, "A_ARG_TYPE_Direction",
                                   BG_UPNP_SV_ARG_ONLY,
                                   BG_UPNP_SV_STRING);
  
  val.s = "Output";
  bg_upnp_sv_desc_add_allowed(sv, &val);
  val.s = "Input";
  bg_upnp_sv_desc_add_allowed(sv, &val);

  bg_upnp_service_desc_add_sv(d, "A_ARG_TYPE_ProtocolInfo",
                              BG_UPNP_SV_ARG_ONLY,
                              BG_UPNP_SV_STRING);
  
  bg_upnp_service_desc_add_sv(d, "A_ARG_TYPE_ConnectionID",
                              BG_UPNP_SV_ARG_ONLY,
                              BG_UPNP_SV_INT4);
  bg_upnp_service_desc_add_sv(d, "A_ARG_TYPE_AVTransportID",
                              BG_UPNP_SV_ARG_ONLY,
                              BG_UPNP_SV_INT4);
  bg_upnp_service_desc_add_sv(d, "A_ARG_TYPE_RcsID",
                              BG_UPNP_SV_ARG_ONLY,
                              BG_UPNP_SV_INT4);

  /* Actions */

  sa = bg_upnp_service_desc_add_action(d, "GetProtocolInfo");
  bg_upnp_sa_desc_add_arg_out(sa, "Source",
                              "SourceProtocolInfo", 0);
  bg_upnp_sa_desc_add_arg_out(sa, "Sink",
                              "SinkProtocolInfo", 0);

  /*
  sa = bg_upnp_service_desc_add_action(d, "PrepareForConnection");
  sa = bg_upnp_service_desc_add_action(d, "ConnectionComplete");
  */

  sa = bg_upnp_service_desc_add_action(d, "GetCurrentConnectionIDs");
  bg_upnp_sa_desc_add_arg_out(sa, "ConnectionIDs",
                              "CurrentConnectionIDs", 0);

  sa = bg_upnp_service_desc_add_action(d, "GetCurrentConnectionInfo");
  bg_upnp_sa_desc_add_arg_in(sa, "ConnectionID",
                              "A_ARG_TYPE_ConnectionID", 0);
  bg_upnp_sa_desc_add_arg_out(sa, "RcsID",
                              "A_ARG_TYPE_RcsID", 0);
  bg_upnp_sa_desc_add_arg_out(sa, "AVTransportID",
                              "A_ARG_TYPE_AVTransportID", 0);
  bg_upnp_sa_desc_add_arg_out(sa, "ProtocolInfo",
                              "A_ARG_TYPE_ProtocolInfo", 0);
  bg_upnp_sa_desc_add_arg_out(sa, "PeerConnectionManager",
                              "A_ARG_TYPE_ConnectionManager", 0);
  bg_upnp_sa_desc_add_arg_out(sa, "PeerConnectionID",
                              "A_ARG_TYPE_ConnectionID", 0);
  bg_upnp_sa_desc_add_arg_out(sa, "Direction",
                              "A_ARG_TYPE_Direction", 0);
  bg_upnp_sa_desc_add_arg_out(sa, "Status",
                              "A_ARG_TYPE_ConnectionStatus", 0);
  }

void bg_upnp_service_init_connection_manager(bg_upnp_service_t * ret,
                                             const char * name)
  {
  bg_upnp_service_init(ret, name, "ConnectionManager", 1);
  init_service_desc(&ret->desc);
  bg_upnp_service_start(ret);
  }
