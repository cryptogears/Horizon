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

const gchar * horizon_post_get_name (const HorizonPost *post) G_GNUC_PURE;
const gchar * horizon_post_get_tripcode (const HorizonPost *post) G_GNUC_PURE;
const gchar * horizon_post_get_comment (const HorizonPost *post) G_GNUC_PURE;
const gchar * horizon_post_get_subject (const HorizonPost *post) G_GNUC_PURE;
gint64 horizon_post_get_time (const HorizonPost *post) G_GNUC_PURE;
gint64 horizon_post_get_post_number (const HorizonPost *post) G_GNUC_PURE;
gint64 horizon_post_get_renamed_filename(const HorizonPost *post) G_GNUC_PURE;
const gchar * horizon_post_get_original_filename(const HorizonPost *post) G_GNUC_PURE;
const gchar * horizon_post_get_ext(const HorizonPost *post) G_GNUC_PURE;
const gchar * horizon_post_get_md5(const HorizonPost *post) G_GNUC_PURE;
const gchar * horizon_post_get_thumb_url(HorizonPost *post) G_GNUC_PURE;
void horizon_post_set_thumb_url(HorizonPost *post, const gchar*);
const gchar * horizon_post_get_image_url(HorizonPost *post) G_GNUC_PURE;
gint64 horizon_post_get_thumbnail_width (const HorizonPost *post) G_GNUC_PURE;
gint64 horizon_post_get_thumbnail_height (const HorizonPost *post) G_GNUC_PURE;
gint64 horizon_post_get_width (const HorizonPost *post) G_GNUC_PURE;
gint64 horizon_post_get_height (const HorizonPost *post) G_GNUC_PURE;
gint64 horizon_post_get_fsize (const HorizonPost *post) G_GNUC_PURE;
gboolean horizon_post_has_image (const HorizonPost *post) G_GNUC_PURE;
gint horizon_post_get_sticky (const HorizonPost *post) G_GNUC_PURE;
gint horizon_post_get_closed (const HorizonPost *post) G_GNUC_PURE;
gint horizon_post_get_deleted (const HorizonPost *post) G_GNUC_PURE;
gint horizon_post_get_spoiler (const HorizonPost *post) G_GNUC_PURE;
gboolean horizon_post_is_gif (const HorizonPost *post) G_GNUC_PURE;

const gchar * horizon_post_get_board (const HorizonPost *post) G_GNUC_PURE;
const gchar * horizon_post_set_board (HorizonPost *post, const gchar *board) G_GNUC_MALLOC;
gint64 horizon_post_get_thread_id(const HorizonPost *post) G_GNUC_PURE;
gint64 horizon_post_set_thread_id(HorizonPost *post, const gint64 id);

gboolean horizon_post_set_rendered (HorizonPost *post, const gboolean rendered);
gboolean horizon_post_is_rendered (const HorizonPost *post) G_GNUC_PURE;
gboolean horizon_post_is_same_post (const HorizonPost *left, const HorizonPost *right) G_GNUC_PURE;
gboolean horizon_post_is_not_same_post (const HorizonPost *left, const HorizonPost *right) G_GNUC_PURE;
const gchar * horizon_post_get_capcode(const HorizonPost *post);

#endif
