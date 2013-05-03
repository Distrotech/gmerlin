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
#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "upnp.servicedesc"

#include <string.h>



/* Values */

char * bg_upnp_val_to_string(bg_upnp_sv_type_t type,
                             const bg_upnp_sv_val_t * val)
  {
  switch(type)
    {
    case BG_UPNP_SV_INT4:
      return bg_sprintf("%d", val->i);
      break;
    case BG_UPNP_SV_STRING:
      return gavl_strdup(val->s);
      break;
    }
  return NULL;
  }

int bg_upnp_string_to_val(bg_upnp_sv_type_t type,
                          const char * str, bg_upnp_sv_val_t * val)
  {
  switch(type)
    {
    case BG_UPNP_SV_INT4:
      {
      char * rest;
      val->i = strtol(str, &rest, 10);
      if(*rest != '\0')
        return 0;
      return 1;
      }
      break;
    case BG_UPNP_SV_STRING:
      val->s = gavl_strrep(val->s, str);
      return 1;
      break;
    }
  return 0;
  }

void bg_upnp_sv_val_free(bg_upnp_sv_val_t * val)
  {
  if(val->s)
    free(val->s);
  }

void bg_upnp_sv_val_copy(bg_upnp_sv_type_t type,
                         bg_upnp_sv_val_t * dst,
                         const bg_upnp_sv_val_t * src)
  {
   switch(type)
    {
    case BG_UPNP_SV_INT4:
      dst->i = src->i;
      break;
    case BG_UPNP_SV_STRING:
      dst->s = gavl_strrep(dst->s, src->s);
      break;
    }
  }

/* State variable */

void bg_upnp_sv_desc_set_default(bg_upnp_sv_desc_t * d,
                                 bg_upnp_sv_val_t * val)
  {
  bg_upnp_sv_val_copy(d->type, &d->def, val);
  d->flags |= BG_UPNP_SV_HAS_DEFAULT;
  }

void bg_upnp_sv_desc_set_range(bg_upnp_sv_desc_t * d,
                               bg_upnp_sv_val_t * min,
                               bg_upnp_sv_val_t * max,
                               bg_upnp_sv_val_t * step)
  {
  bg_upnp_sv_val_copy(d->type, &d->range.min, min); 
  bg_upnp_sv_val_copy(d->type, &d->range.max, max); 
  bg_upnp_sv_val_copy(d->type, &d->range.step, step); 
  d->flags |= BG_UPNP_SV_HAS_RANGE;
  }

void bg_upnp_sv_desc_add_allowed(bg_upnp_sv_desc_t * d,
                                 bg_upnp_sv_val_t * val)
  {
  if(d->num_allowed + 1 > d->allowed_alloc)
    {
    d->allowed_alloc += 8;
    d->allowed = realloc(d->allowed, d->allowed_alloc * sizeof(*d->allowed));
    memset(d->allowed + d->num_allowed, 0, (d->allowed_alloc - d->num_allowed) * sizeof(*d->allowed));
    }
  bg_upnp_sv_val_copy(d->type, d->allowed + d->num_allowed, val);
  d->num_allowed++;
  }

void bg_upnp_sv_desc_free(bg_upnp_sv_desc_t * d)
  {
  bg_upnp_sv_val_free(&d->def);
  bg_upnp_sv_val_free(&d->range.min);
  bg_upnp_sv_val_free(&d->range.max);
  bg_upnp_sv_val_free(&d->range.step);
  free(d->name);
  if(d->allowed)
    {
    int i;
    for(i = 0; i < d->num_allowed; i++)
      bg_upnp_sv_val_free(&d->allowed[i]);
    free(d->allowed);
    }
  }

/* Argument for an action */

void bg_upnp_arg_desc_free(bg_upnp_sa_arg_desc_t * d)
  {
  if(d->name)
    free(d->name);
  if(d->rsv_name)
    free(d->rsv_name);
  }

/* Description of a service action */

static void add_sa_arg(bg_upnp_sa_arg_desc_t ** argp, int * nump, int * allocp,
                       const char * name, const char * rsv_name, int flags)
  {
  int num = *nump;
  int alloc = *allocp;
  bg_upnp_sa_arg_desc_t * arg = *argp;

  if(num + 1 > alloc)
    {
    alloc += 8;
    arg = realloc(arg, alloc * sizeof(*arg));
    memset(arg + num, 0, (alloc - num)*sizeof(*arg));
    }
  arg[num].name = gavl_strdup(name);
  arg[num].rsv_name = gavl_strdup(rsv_name);
  arg[num].flags = flags;
  num++;

  *nump = num;
  *allocp = alloc;
  *argp = arg;
  }

static void free_sa_args(bg_upnp_sa_arg_desc_t * arg, int num)
  {
  if(arg)
    {
    int i;
    for(i = 0; i < num; i++)
      bg_upnp_arg_desc_free(arg + i) ;
    free(arg);
    }
  }

void
bg_upnp_sa_desc_add_arg_in(bg_upnp_sa_desc_t * d, const char * name,
                           const char * rsv_name, int flags)
  {
  add_sa_arg(&d->args_in, &d->num_args_in, &d->args_in_alloc,
             name, rsv_name, flags);
  }

void
bg_upnp_sa_desc_add_arg_out(bg_upnp_sa_desc_t * d, const char * name,
                            const char * rsv_name, int flags)
  {
  add_sa_arg(&d->args_out, &d->num_args_out, &d->args_out_alloc,
             name, rsv_name, flags);

  }

void bg_upnp_sa_desc_free(bg_upnp_sa_desc_t * d)
  {
  if(d->name)
    free(d->name);
  free_sa_args(d->args_in, d->num_args_in);
  free_sa_args(d->args_out, d->num_args_out);
  }

static const bg_upnp_sa_arg_desc_t *
sa_arg_by_name(bg_upnp_sa_arg_desc_t * args,
               int num, const char * name)
  {
  int i;
  for(i = 0; i < num; i++)
    {
    if(!strcmp(args[i].name, name))
      return &args[i];
    }
  return NULL;
  }

const bg_upnp_sa_arg_desc_t *
bg_upnp_sa_desc_in_arg_by_name(bg_upnp_sa_desc_t * d, const char * name)
  {
  return sa_arg_by_name(d->args_in, d->num_args_in, name);
  }

const bg_upnp_sa_arg_desc_t *
bg_upnp_sa_desc_out_arg_by_name(bg_upnp_sa_desc_t * d, const char * name)
  {
  return sa_arg_by_name(d->args_out, d->num_args_out, name);
  }

/* Service description */

// Add state variable
bg_upnp_sv_desc_t *
bg_upnp_service_desc_add_sv(bg_upnp_service_desc_t * d, const char * name,
                            int flags, bg_upnp_sv_type_t type)
  {
  bg_upnp_sv_desc_t * ret;
  if(d->num_sv + 1 > d->sv_alloc)
    {
    d->sv_alloc += 8;
    d->sv = realloc(d->sv, d->sv_alloc * sizeof(*d->sv));
    memset(d->sv + d->num_sv, 0, (d->sv_alloc - d->num_sv) * sizeof(*d->sv));
    }
  ret = d->sv + d->num_sv;
  ret->name = gavl_strdup(name);
  ret->flags = flags;
  ret->type = type;
  d->num_sv++;
  return ret;
  }

bg_upnp_sa_desc_t *
bg_upnp_service_desc_add_action(bg_upnp_service_desc_t * d, const char * name)
  {
  bg_upnp_sa_desc_t * ret;
  if(d->num_sa + 1 > d->sa_alloc)
    {
    d->sa_alloc += 8;
    d->sa = realloc(d->sa, d->sa_alloc * sizeof(*d->sa));
    memset(d->sa + d->num_sa, 0, (d->sa_alloc - d->num_sa) * sizeof(*d->sa));
    }
  ret = d->sa + d->num_sa;
  ret->name = gavl_strdup(name);
  d->num_sa++;
  return ret;
  }

void bg_upnp_service_desc_free(bg_upnp_service_desc_t * d)
  {
  int i;
  if(d->sa)
    {
    for(i = 0; i < d->num_sa; i++)
      bg_upnp_sa_desc_free(d->sa + i);
    free(d->sa);
    }
  if(d->sv)
    {
    for(i = 0; i < d->num_sv; i++)
      bg_upnp_sv_desc_free(d->sv + i);
    free(d->sv);
    }
  }

const bg_upnp_sv_desc_t *
bg_upnp_service_desc_sv_by_name(bg_upnp_service_desc_t * d, const char * name)
  {
  int i;
  for(i = 0; i < d->num_sv; i++)
    {
    if(!strcmp(d->sv[i].name, name))
      return &d->sv[i];
    }
  return NULL;
  }

static int resolve_sv_refs(bg_upnp_service_desc_t * d,
                           bg_upnp_sa_arg_desc_t * args, int num)
  {
  int i;
  for(i = 0; i < num; i++)
    {
    args[i].rsv = bg_upnp_service_desc_sv_by_name(d, args[i].rsv_name);
    if(!args[i].rsv)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Couldn't find state variable %s",
             args[i].rsv_name);
      return 0;
      }
    }
  return 1;
  }

int bg_upnp_service_desc_resolve_refs(bg_upnp_service_desc_t * d)
  {
  int i;
  for(i = 0; i < d->num_sa; i++)
    {
    if(!resolve_sv_refs(d, d->sa[i].args_in, d->sa[i].num_args_in) ||
       !resolve_sv_refs(d, d->sa[i].args_out, d->sa[i].num_args_out))
      return 0;
    }
  return 1;
  }
