#ifndef HORIZON_POST_H
#define HORIZON_POST_H

#include <glib-object.h>

/*
 * Type macros.
 */

#define HORIZON_TYPE_POST                  (horizon_post_get_type ())
#define HORIZON_POST(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), HORIZON_TYPE_POST, HorizonPost))
#define HORIZON_IS_POST(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HORIZON_TYPE_POST))
#define HORIZON_POST_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), HORIZON_TYPE_POST, HorizonPostClass))
#define HORIZON_IS_POST_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), HORIZON_TYPE_POST))
#define HORIZON_POST_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), HORIZON_TYPE_POST, HorizonPostClass))

typedef struct _HorizonPost        HorizonPost;
typedef struct _HorizonPostClass   HorizonPostClass;
typedef struct _HorizonPostPrivate HorizonPostPrivate;

struct _HorizonPost
{
	GObject parent_instance;
	
	/* instance members */

	/* private */
	HorizonPostPrivate *priv;
};

struct _HorizonPostClass
{
  GObjectClass parent_class;

  /* class members */
};

/* used by HORIZON_TYPE_POST */

GType horizon_post_get_type (void);

/*
 * Method definitions.
 */

const gchar * horizon_post_get_md5 (HorizonPost *post);
const gint64 horizon_post_get_fsize (HorizonPost *post);
const gint64 horizon_post_get_time (HorizonPost *post);
const gint64 horizon_post_get_post_number (HorizonPost *post);
const gchar * horizon_post_get_name (HorizonPost *post);
const gchar * horizon_post_get_subject (HorizonPost *post);
const gchar * horizon_post_get_comment (HorizonPost *post);
const gint64 horizon_post_get_renamed_filename(HorizonPost *post);
const gchar * horizon_post_get_original_filename(HorizonPost *post);
const gchar * horizon_post_get_ext(HorizonPost *post);
const gint64 horizon_post_get_width (HorizonPost *post);
const gint64 horizon_post_get_height (HorizonPost *post);
const gint64 horizon_post_get_thumbnail_width (HorizonPost *post);
const gint64 horizon_post_get_thumbnail_height (HorizonPost *post);

#endif
