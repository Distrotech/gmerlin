#include <libintl.h>

/* Static strings (will be regognized by xgettext) */
#define TRS(s) s

#define TRD(s, d) (d ? dgettext(d, s) : dgettext(PACKAGE, s))

#define TR(s) dgettext(PACKAGE, s)

#define TR_DOM(str) dgettext((translation_domain ? translation_domain : PACKAGE), str)

void bg_translation_init();

#define BG_LOCALE \
gettext_domain: PACKAGE, \
gettext_directory: LOCALE_DIR
