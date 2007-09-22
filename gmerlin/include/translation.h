#include <libintl.h>

/* Static strings (will be regognized by xgettext) */
#define TRS(s) s

/* Just add the string to the .po files, even if it doesn't occur
 * anywhere in the source. This can be used to translate messages
 * coming from other (not gettextized) libs.
 */

#define TRU(s)

#define TRD(s, d) (d ? dgettext(d, s) : dgettext(PACKAGE, s))

#define TR(s) dgettext(PACKAGE, s)

#define TR_DOM(str) dgettext((translation_domain ? translation_domain : PACKAGE), str)

void bg_translation_init();

void bg_bindtextdomain(const char * domainname, const char * dirname);

#define BG_LOCALE \
gettext_domain: PACKAGE, \
gettext_directory: LOCALE_DIR
