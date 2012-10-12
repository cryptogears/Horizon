#ifndef HORIZON_THREAD_SUMMARY_H
#define HORIZON_THREAD_SUMMARY_H
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <glib-object.h>

/*
 * Type macros.
 */

#define HORIZON_TYPE_THREAD_SUMMARY        (horizon_thread_summary_get_type ())
#define HORIZON_THREAD_SUMMARY(obj)        (G_TYPE_CHECK_INSTANCE_CAST ((obj), HORIZON_TYPE_THREAD_SUMMARY, HorizonThreadSummary))
#define HORIZON_IS_THREAD_SUMMARY(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HORIZON_TYPE_THREAD_SUMMARY))
#define HORIZON_THREAD_SUMMARY_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), HORIZON_TYPE_THREAD_SUMMARY, HorizonThreadSummaryClass))
#define HORIZON_IS_THREAD_SUMMARY_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), HORIZON_TYPE_THREAD_SUMMARY))
#define HORIZON_THREAD_SUMMARY_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), HORIZON_TYPE_THREAD_SUMMARY, HorizonThreadSummaryClass))

typedef struct _HorizonThreadSummary        HorizonThreadSummary;
typedef struct _HorizonThreadSummaryClass   HorizonThreadSummaryClass;
typedef struct _HorizonThreadSummaryPrivate HorizonThreadSummaryPrivate;

struct _HorizonThreadSummary
{
	GObject parent_instance;
	
	/* instance members */

	/* private */
	HorizonThreadSummaryPrivate *priv;
};

struct _HorizonThreadSummaryClass
{
  GObjectClass parent_class;

  /* class members */
};

/* used by HORIZON_TYPE_THREAD_SUMMARY */

GType horizon_thread_summary_get_type (void);

/*
 * Method definitions.
 */

gint64 horizon_thread_summary_get_unix_date (const HorizonThreadSummary *ts) G_GNUC_PURE;
gint64 horizon_thread_summary_get_id (const HorizonThreadSummary *ts) G_GNUC_PURE;
void horizon_thread_summary_set_id (HorizonThreadSummary *ts, gint64 id);
void horizon_thread_summary_set_id_from_string (HorizonThreadSummary *ts, const gchar* id_str);
gint64 horizon_thread_summary_get_image_count (const HorizonThreadSummary *ts) G_GNUC_PURE;
gint64 horizon_thread_summary_get_reply_count (const HorizonThreadSummary *ts) G_GNUC_PURE;
const gchar* horizon_thread_summary_get_author (const HorizonThreadSummary *ts) G_GNUC_PURE;
const gchar* horizon_thread_summary_get_teaser (const HorizonThreadSummary *ts) G_GNUC_PURE;
const gchar* horizon_thread_summary_get_spoiler_image (const HorizonThreadSummary *ts) G_GNUC_PURE;
gboolean horizon_thread_summary_is_spoiler (const HorizonThreadSummary *ts) G_GNUC_PURE;
void horizon_thread_summary_set_board(HorizonThreadSummary *ts, const gchar* board);
const gchar* horizon_thread_summary_get_board( const HorizonThreadSummary *ts);
const gchar* horizon_thread_summary_get_url(const HorizonThreadSummary *ts) G_GNUC_PURE;
const gchar* horizon_thread_summary_get_thumb_url(const HorizonThreadSummary *ts) G_GNUC_PURE;
void horizon_thread_summary_set_thumb_pixbuf(HorizonThreadSummary *ts, GdkPixbuf* pixbuf);
GdkPixbuf* horizon_thread_summary_get_thumb_pixbuf(HorizonThreadSummary *ts);

#endif
