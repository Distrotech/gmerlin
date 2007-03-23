/*****************************************************************
 
  registry_priv.h
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

typedef enum
  {
    BG_CFG_INT,
    BG_CFG_FLOAT,
    BG_CFG_STRING,
    BG_CFG_STRING_HIDDEN,
    BG_CFG_COLOR,
    BG_CFG_TIME /* int64 */
  } bg_cfg_type_t;

typedef struct bg_cfg_item_s     bg_cfg_item_t;

struct bg_cfg_item_s
  {
  char * name;
  bg_parameter_value_t value;
  bg_cfg_type_t type;
  struct bg_cfg_item_s * next;
  };

struct bg_cfg_section_s
  {
  char * name;
  char * gettext_domain;
  char * gettext_directory;
  
  bg_cfg_item_t * items;

  struct bg_cfg_section_s * next;
  struct bg_cfg_section_s * children;
  };

struct bg_cfg_registry_s
  {
  bg_cfg_section_t * sections;
  
  };

/* Create an empty item */

bg_cfg_item_t * bg_cfg_item_create_empty(const char * name);

/* Value can be NULL, then the default is used */

bg_cfg_item_t * bg_cfg_item_create(const bg_parameter_info_t *,
                                   bg_parameter_value_t * value);

void bg_cfg_destroy_item(bg_cfg_item_t *);



bg_cfg_item_t * bg_cfg_section_find_item(bg_cfg_section_t * section,
                                         const bg_parameter_info_t * info);

bg_cfg_item_t * bg_cfg_item_copy(bg_cfg_item_t * src);
void bg_cfg_item_transfer(bg_cfg_item_t * src, bg_cfg_item_t * dst);
void bg_cfg_item_to_parameter(bg_cfg_item_t * src, bg_parameter_info_t * info);
