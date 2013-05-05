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

#include <gmerlin/utils.h>

/* Service actions */

#define ARG_SearchCaps        1
#define ARG_SortCaps          2
#define ARG_Id                3
#define ARG_ObjectID          4
#define ARG_BrowseFlag        5
#define ARG_Filter            6
#define ARG_StartingIndex     7
#define ARG_RequestedCount    8
#define ARG_SortCriteria      9
#define ARG_Result            10
#define ARG_NumberReturned    11
#define ARG_TotalMatches      12

static int GetSearchCapabilities(bg_upnp_service_t * s)
  {
  char * ret = calloc(1, 1);
  bg_upnp_service_set_arg_out_string(&s->req, ARG_SearchCaps, ret);
  return 0;
  }

static int GetSortCapabilities(bg_upnp_service_t * s)
  {
  char * ret = calloc(1, 1);
  bg_upnp_service_set_arg_out_string(&s->req, ARG_SearchCaps, ret);
  return 0;
  }


static int GetSystemUpdateID(bg_upnp_service_t * s)
  {
  bg_upnp_service_set_arg_out_int(&s->req, ARG_Id, 1);
  return 0;
  }

char * def_result =
  "<DIDL-Lite xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
  " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\""
  " xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\">"
  "<container id=\"1\" parentID=\"0\" childCount=\"2\" restricted=\"1\">\n"
  "<dc:title>My Music</dc:title>\n"
  "<upnp:class>object.container.storageFolder</upnp:class>\n"
  "<dc:storageUsed>100000000</dc:storageUsed>\n"
  "<upnp:writeStatus>PROTECTED</upnp:writeStatus>\n"
  "</container>\n"
  "</DIDL-Lite>";

char * def_root =
  "<DIDL-Lite xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
  " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\""
  " xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\">"
  "<container id=\"0\" parentID=\"-1\" childCount=\"1\" restricted=\"1\">\n"
  "<dc:title>Root</dc:title>\n"
  "<upnp:class>object.container</upnp:class>\n"
  "</container>\n"
  "</DIDL-Lite>";


static int Browse(bg_upnp_service_t * s)
  {
  const char * ObjectID = bg_upnp_service_get_arg_in_string(&s->req, ARG_ObjectID);
  const char * BrowseFlag = bg_upnp_service_get_arg_in_string(&s->req, ARG_BrowseFlag);
  const char * Filter = bg_upnp_service_get_arg_in_string(&s->req, ARG_Filter);
  int StartingIndex = bg_upnp_service_get_arg_in_int(&s->req, ARG_StartingIndex);
  int RequestedCount = bg_upnp_service_get_arg_in_int(&s->req, ARG_RequestedCount);
  
  fprintf(stderr, "Browse: Id: %s, Flag: %s, Filter: %s, Start: %d, Num: %d\n",
          ObjectID, BrowseFlag, Filter, StartingIndex, RequestedCount);

  bg_upnp_service_set_arg_out_int(&s->req, ARG_NumberReturned, 1);
  bg_upnp_service_set_arg_out_int(&s->req, ARG_TotalMatches, 1);

  
  if(!strcmp(BrowseFlag, "BrowseMetadata"))
    bg_upnp_service_set_arg_out_string(&s->req, ARG_Result, gavl_strdup(def_root));
  else
    bg_upnp_service_set_arg_out_string(&s->req, ARG_Result, gavl_strdup(def_result));
    
  fprintf(stderr, "didl:\n%s\n", def_result);

  //  bg_upnp_service_set_arg_out_string(&s->req, ARG_Result, gavl_strdup(def_result));
  return 1;
  }


/* Initialize service description */

static void init_service_desc(bg_upnp_service_desc_t * d)
  {
  bg_upnp_sv_val_t  val;
  bg_upnp_sv_desc_t * sv;
  bg_upnp_sa_desc_t * sa;

  /*
    bg_upnp_service_desc_add_sv(d, "TransferIDs",
                                BG_UPNP_SV_EVENT,
                                BG_UPNP_SV_STRING);

   */
  
  bg_upnp_service_desc_add_sv(d, "A_ARG_TYPE_ObjectID",
                              BG_UPNP_SV_ARG_ONLY,
                              BG_UPNP_SV_STRING);
  bg_upnp_service_desc_add_sv(d, "A_ARG_TYPE_Result",
                              BG_UPNP_SV_ARG_ONLY,
                              BG_UPNP_SV_STRING);
  //  Optional
  //  bg_upnp_service_desc_add_sv(d, "A_ARG_TYPE_SearchCriteria",
  //                              BG_UPNP_SV_ARG_ONLY,
  //                              BG_UPNP_SV_STRING);

  sv = bg_upnp_service_desc_add_sv(d, "A_ARG_TYPE_BrowseFlag",
                                   BG_UPNP_SV_ARG_ONLY,
                                   BG_UPNP_SV_STRING);

  val.s = "BrowseMetadata";
  bg_upnp_sv_desc_add_allowed(sv, &val);
  val.s = "BrowseDirectChildren";
  bg_upnp_sv_desc_add_allowed(sv, &val);
  
  bg_upnp_service_desc_add_sv(d, "A_ARG_TYPE_Filter",
                              BG_UPNP_SV_ARG_ONLY,
                              BG_UPNP_SV_STRING);
  bg_upnp_service_desc_add_sv(d, "A_ARG_TYPE_SortCriteria",
                              BG_UPNP_SV_ARG_ONLY,
                              BG_UPNP_SV_STRING);
  bg_upnp_service_desc_add_sv(d, "A_ARG_TYPE_Index",
                              BG_UPNP_SV_ARG_ONLY,
                              BG_UPNP_SV_INT4);
  bg_upnp_service_desc_add_sv(d, "A_ARG_TYPE_Count",
                              BG_UPNP_SV_ARG_ONLY,
                              BG_UPNP_SV_INT4);
  bg_upnp_service_desc_add_sv(d, "A_ARG_TYPE_UpdateID",
                              BG_UPNP_SV_ARG_ONLY,
                              BG_UPNP_SV_INT4);
  /*
  bg_upnp_service_desc_add_sv(d, "A_ARG_TYPE_TransferID",
                              BG_UPNP_SV_ARG_ONLY,
                              BG_UPNP_SV_INT4);
  bg_upnp_service_desc_add_sv(d, "A_ARG_TYPE_TransferStatus",
                              BG_UPNP_SV_ARG_ONLY,
                              BG_UPNP_SV_STRING);
  bg_upnp_service_desc_add_sv(d, "A_ARG_TYPE_TransferLength",
                              BG_UPNP_SV_ARG_ONLY,
                              BG_UPNP_SV_STRING);
  bg_upnp_service_desc_add_sv(d, "A_ARG_TYPE_TransferTotal",
                              BG_UPNP_SV_ARG_ONLY,
                              BG_UPNP_SV_STRING);
  bg_upnp_service_desc_add_sv(d, "A_ARG_TYPE_TagValueList",
                              BG_UPNP_SV_ARG_ONLY,
                              BG_UPNP_SV_STRING);
  bg_upnp_service_desc_add_sv(d, "A_ARG_TYPE_URI",
                              BG_UPNP_SV_ARG_ONLY,
                              BG_UPNP_SV_STRING);
  */

  bg_upnp_service_desc_add_sv(d, "SearchCapabilities",
                              0, BG_UPNP_SV_STRING);
  bg_upnp_service_desc_add_sv(d, "SortCapabilities",
                              0, BG_UPNP_SV_STRING);
  bg_upnp_service_desc_add_sv(d, "SystemUpdateID",
                              BG_UPNP_SV_EVENT, BG_UPNP_SV_INT4);
  /*
  bg_upnp_service_desc_add_sv(d, "ContainerUpdateIDs",
                              BG_UPNP_SV_EVENT, BG_UPNP_SV_STRING);
  */

  /* Actions */

  sa = bg_upnp_service_desc_add_action(d, "GetSearchCapabilities",
                                       GetSearchCapabilities);

  bg_upnp_sa_desc_add_arg_out(sa, "SearchCaps",
                              "SearchCapabilities", 0,
                              ARG_SearchCaps);

  sa = bg_upnp_service_desc_add_action(d, "GetSortCapabilities",
                                       GetSortCapabilities);
  bg_upnp_sa_desc_add_arg_out(sa, "SortCaps",
                              "SortCapabilities", 0,
                              ARG_SortCaps);
  
  sa = bg_upnp_service_desc_add_action(d, "GetSystemUpdateID",
                                       GetSystemUpdateID);

  bg_upnp_sa_desc_add_arg_out(sa, "Id",
                              "SystemUpdateID", 0,
                              ARG_Id);

  sa = bg_upnp_service_desc_add_action(d, "Browse", Browse);
  bg_upnp_sa_desc_add_arg_in(sa, "ObjectID",
                             "A_ARG_TYPE_ObjectID", 0,
                             ARG_ObjectID);
  bg_upnp_sa_desc_add_arg_in(sa, "BrowseFlag",
                             "A_ARG_TYPE_BrowseFlag", 0,
                             ARG_BrowseFlag);
  bg_upnp_sa_desc_add_arg_in(sa, "Filter",
                             "A_ARG_TYPE_Filter", 0,
                             ARG_Filter);
  bg_upnp_sa_desc_add_arg_in(sa, "StartingIndex",
                             "A_ARG_TYPE_Index", 0,
                             ARG_StartingIndex);
  bg_upnp_sa_desc_add_arg_in(sa, "RequestedCount",
                             "A_ARG_TYPE_Count", 0,
                             ARG_RequestedCount);
  bg_upnp_sa_desc_add_arg_in(sa, "SortCriteria",
                             "A_ARG_TYPE_SortCriteria", 0,
                             ARG_SortCriteria);
  
  bg_upnp_sa_desc_add_arg_out(sa, "Result",
                              "A_ARG_TYPE_Result", 0,
                              ARG_Result);
  bg_upnp_sa_desc_add_arg_out(sa, "NumberReturned",
                              "A_ARG_TYPE_Count", 0,
                              ARG_NumberReturned);
  bg_upnp_sa_desc_add_arg_out(sa, "TotalMatches",
                              "A_ARG_TYPE_Count", 0,
                              ARG_TotalMatches);
  
  /*

    sa = bg_upnp_service_desc_add_action(d, "Search");
    sa = bg_upnp_service_desc_add_action(d, "CreateObject");
    sa = bg_upnp_service_desc_add_action(d, "DestroyObject");
    sa = bg_upnp_service_desc_add_action(d, "UpdateObject");
    sa = bg_upnp_service_desc_add_action(d, "ImportResource");
    sa = bg_upnp_service_desc_add_action(d, "ExportResource");
    sa = bg_upnp_service_desc_add_action(d, "StopTransferResource");
    sa = bg_upnp_service_desc_add_action(d, "GetTransferProgress");
  */

  
  
  }


void bg_upnp_service_init_content_directory(bg_upnp_service_t * ret,
                                            const char * name,
                                            bg_db_t * db)
  {
  bg_upnp_service_init(ret, name, "ContentDirectory", 1);
  init_service_desc(&ret->desc);
  bg_upnp_service_start(ret);
  }
