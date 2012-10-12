#include "horizon_thread_summary.h"

#define HORIZON_THREAD_SUMMARY_GET_PRIVATE(obj) ((HorizonThreadSummaryPrivate *)((HORIZON_THREAD_SUMMARY(obj))->priv))

G_DEFINE_TYPE (HorizonThreadSummary, horizon_thread_summary, G_TYPE_OBJECT);

enum
{
  PROP_0,

  PROP_UNIX_DATE,
  PROP_ID,
  PROP_IMAGE_COUNT,
  PROP_REPLY_COUNT,
  PROP_AUTHOR,
  PROP_TEASER,
  PROP_IS_SPOILER,
  PROP_SPOILER_IMAGE,
  
  N_PROPERTIES
};

struct _HorizonThreadSummaryPrivate
{
	gint64     unix_date;
	gint64     id;
	gint64     image_count;
	gint64     reply_count;
	gchar     *author;
	gchar     *teaser;
	gchar     *spoiler_image;
	gboolean   is_spoiler;
	gchar     *board;
	gchar     *url;
	gchar     *thumb_url;
	GdkPixbuf *thumb_pixbuf;
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
horizon_thread_summary_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
	HorizonThreadSummary *self = HORIZON_THREAD_SUMMARY (object);

	switch (property_id)
		{
		case PROP_UNIX_DATE:
			self->priv->unix_date = g_value_get_int64 (value);
			break;
		case PROP_ID:
			self->priv->id = g_value_get_int64 (value);
			break;
		case PROP_IMAGE_COUNT:
			self->priv->image_count   = g_value_get_int64  (value);
			break;
		case PROP_REPLY_COUNT:
			self->priv->reply_count  = g_value_get_int64  (value);
			break;
		case PROP_AUTHOR:
			g_free (self->priv->author);
			self->priv->author  = g_value_dup_string  (value);
			break;
		case PROP_TEASER:
			g_free (self->priv->teaser);
			self->priv->teaser  = g_value_dup_string  (value);
			break;
		case PROP_SPOILER_IMAGE:
			g_free (self->priv->spoiler_image);
			self->priv->spoiler_image  = g_value_dup_string  (value);
			break;
		case PROP_IS_SPOILER:
			self->priv->is_spoiler  = g_value_get_boolean  (value);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
		}
}

static void
horizon_thread_summary_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
	HorizonThreadSummary *self = HORIZON_THREAD_SUMMARY (object);

	switch (property_id)
		{
		case PROP_UNIX_DATE:
			g_value_set_int64 (value, self->priv->unix_date);
			break;
		case PROP_ID:
			g_value_set_int64 (value, self->priv->id);
			break;
		case PROP_IMAGE_COUNT:
			g_value_set_int64  (value, self->priv->image_count  );
			break;
		case PROP_REPLY_COUNT:
			g_value_set_int (value, self->priv->reply_count );
			break;
		case PROP_AUTHOR:
			g_value_set_string (value, self->priv->author  );
			break;
		case PROP_TEASER:
			g_value_set_string (value, self->priv->teaser  );
			break;
		case PROP_SPOILER_IMAGE:
			g_value_set_string (value, self->priv->spoiler_image  );
			break;
		case PROP_IS_SPOILER:
			g_value_set_boolean (value, self->priv->is_spoiler  );
			break;
		default:
			/* We don't have any other property... */
	    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
horizon_thread_summary_init (HorizonThreadSummary *self)
{
  HorizonThreadSummaryPrivate *priv = NULL;

  self->priv = priv = (HorizonThreadSummaryPrivate*) g_malloc0(sizeof(HorizonThreadSummaryPrivate));
}

static void
horizon_thread_summary_dispose (GObject *gobject)
{
	//HorizonThreadSummary *self = HORIZON_THREAD_SUMMARY (gobject);

  /* 
   * In dispose, you are supposed to free all types referenced from this
   * object which might themselves hold a reference to self. Generally,
   * the most simple solution is to unref all members on which you own a 
   * reference.
   */

  /* dispose might be called multiple times, so we must guard against
   * calling g_object_unref() on an invalid GObject.
   */
  /*
  if (self->priv->an_object)
    {
      g_object_unref (self->priv->an_object);

      self->priv->an_object = NULL;
    }
  */
  /* Chain up to the parent class */
  G_OBJECT_CLASS (horizon_thread_summary_parent_class)->dispose (gobject);
}

static void
horizon_thread_summary_finalize (GObject *gobject)
{
  HorizonThreadSummary *self = HORIZON_THREAD_SUMMARY (gobject);


  g_free (self->priv->author);
  g_free (self->priv->teaser);
  g_free (self->priv->spoiler_image);
  g_free (self->priv->board);
  g_free (self->priv->url);
  g_free (self->priv->thumb_url);

  if (self->priv->thumb_pixbuf)
	  g_object_unref(self->priv->thumb_pixbuf);

  g_free (self->priv);
  /* Chain up to the parent class */
  G_OBJECT_CLASS (horizon_thread_summary_parent_class)->finalize (gobject);
}




static void
horizon_thread_summary_class_init (HorizonThreadSummaryClass *klass)
{
  g_type_class_add_private (klass, sizeof (HorizonThreadSummaryPrivate));

  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->set_property = horizon_thread_summary_set_property;
  gobject_class->get_property = horizon_thread_summary_get_property;
  gobject_class->dispose      = horizon_thread_summary_dispose;
  gobject_class->finalize     = horizon_thread_summary_finalize;

  obj_properties[PROP_ID] =
	  g_param_spec_int64 ("id", /* name */
	                       "Thread number", /* nick */
	                       "Thread's unique identifier", /* blurb */
	                       1  /* minimum value */,
	                       9999999999999 /* maximum value */,
	                       1  /* default value */,
	                       G_PARAM_READWRITE);
  
  obj_properties[PROP_UNIX_DATE] =
	  g_param_spec_int64 ("date",
	                       "Unix timestamp",
	                       "",
	                       0  /* minimum value */,
	                       G_MAXINT64 /* maximum value */,
	                       0  /* default value */,
	                       G_PARAM_READWRITE);
  obj_properties[PROP_IMAGE_COUNT] =
	  g_param_spec_int64 ("i",
	                      "Image count",
	                      "Number of images posted in thread",
	                      0,
	                      G_MAXINT64,
	                      0, /* default value */
	                      G_PARAM_READWRITE);
  obj_properties[PROP_REPLY_COUNT] =
	  g_param_spec_int64 ("r",
	                      "Reply count",
	                      "Number of posts in the thread",
	                      0,
	                      G_MAXINT64,
	                      0, /* default value */
	                      G_PARAM_READWRITE);
  obj_properties[PROP_AUTHOR] =
	  g_param_spec_string ("author",
	                       "Author of the thread",
	                       "",
	                       "no-author-set", /* default value */
	                       G_PARAM_READWRITE);
  obj_properties[PROP_TEASER] =
	  g_param_spec_string ("teaser",
	                       "A summary of the thread",
	                       "",
	                       "no-teaser-set", /* default value */
	                       G_PARAM_READWRITE);
  obj_properties[PROP_SPOILER_IMAGE] =
	  g_param_spec_string ("s",
	                       "Filename of the spoiler image",
	                       "",
	                       "no-spoiler-image-set", /* default value */
	                       G_PARAM_READWRITE);
  obj_properties[PROP_IS_SPOILER] =
	  g_param_spec_boolean ("splr",
	                        "Whether the image is spoilered or not",
	                        "",
	                        FALSE, /* default value */
	                        G_PARAM_READWRITE);
  
  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);
  
}

gint64
horizon_thread_summary_get_unix_date (const HorizonThreadSummary *ts) {
	g_return_val_if_fail (HORIZON_IS_THREAD_SUMMARY (ts), 0);

	return ts->priv->unix_date;
}

gint64
horizon_thread_summary_get_id (const HorizonThreadSummary *ts) {
	g_return_val_if_fail (HORIZON_IS_THREAD_SUMMARY (ts), 0);

	return ts->priv->id;
}

void
horizon_thread_summary_set_id (HorizonThreadSummary *ts, gint64 id) {
	g_return_if_fail (HORIZON_IS_THREAD_SUMMARY (ts));

	ts->priv->id = id;

	g_free(ts->priv->url);
	ts->priv->url = g_strdup_printf("http://boards.4chan.org/%s/res/%" G_GINT64_FORMAT, ts->priv->board, ts->priv->id);

	g_free(ts->priv->thumb_url);
	ts->priv->thumb_url = g_strdup_printf("http://catalog.neet.tv/%s/src/%" G_GINT64_FORMAT ".jpg", ts->priv->board, ts->priv->id);
}

void horizon_thread_summary_set_id_from_string (HorizonThreadSummary *ts, const gchar* id_str) {
	g_return_if_fail (HORIZON_IS_THREAD_SUMMARY (ts));

	ts->priv->id = g_ascii_strtoll( id_str, NULL, 10 );

	g_free(ts->priv->url);
	ts->priv->url = g_strdup_printf("http://boards.4chan.org/%s/res/%" G_GINT64_FORMAT, ts->priv->board, ts->priv->id);

	g_free(ts->priv->thumb_url);
	ts->priv->thumb_url = g_strdup_printf("http://catalog.neet.tv/%s/src/%" G_GINT64_FORMAT ".jpg", ts->priv->board, ts->priv->id);
}

gint64
horizon_thread_summary_get_image_count (const HorizonThreadSummary *ts) {
	g_return_val_if_fail (HORIZON_IS_THREAD_SUMMARY (ts), 0);

	return ts->priv->image_count;
}

gint64
horizon_thread_summary_get_reply_count (const HorizonThreadSummary *ts) {
	g_return_val_if_fail (HORIZON_IS_THREAD_SUMMARY (ts), 0);

	return ts->priv->reply_count;
}

const gchar*
horizon_thread_summary_get_author (const HorizonThreadSummary *ts) {
	g_return_val_if_fail (HORIZON_IS_THREAD_SUMMARY (ts), 0);

	return ts->priv->author;
}

const gchar*
horizon_thread_summary_get_teaser (const HorizonThreadSummary *ts) {
	g_return_val_if_fail (HORIZON_IS_THREAD_SUMMARY (ts), 0);

	return ts->priv->teaser;
}

const gchar*
horizon_thread_summary_get_spoiler_image (const HorizonThreadSummary *ts) {
	g_return_val_if_fail (HORIZON_IS_THREAD_SUMMARY (ts), 0);

	return ts->priv->spoiler_image;
}

gboolean
horizon_thread_summary_is_spoiler (const HorizonThreadSummary *ts) {
	g_return_val_if_fail (HORIZON_IS_THREAD_SUMMARY (ts), 0);

	return ts->priv->is_spoiler;
}

void
horizon_thread_summary_set_board(HorizonThreadSummary *ts, const gchar* board) {
	g_return_if_fail (HORIZON_IS_THREAD_SUMMARY (ts));

	g_free(ts->priv->board);
	ts->priv->board = g_strdup(board);

	g_free(ts->priv->url);
	ts->priv->url = g_strdup_printf("http://boards.4chan.org/%s/res/%" G_GINT64_FORMAT, ts->priv->board, ts->priv->id);

	g_free(ts->priv->thumb_url);
	ts->priv->thumb_url = g_strdup_printf("http://catalog.neet.tv/%s/src/%" G_GINT64_FORMAT ".jpg", ts->priv->board, ts->priv->id);

}

const gchar* horizon_thread_summary_get_board(const HorizonThreadSummary *ts) {
	g_return_val_if_fail (HORIZON_IS_THREAD_SUMMARY (ts), NULL);

	return ts->priv->board;
}

const gchar*
horizon_thread_summary_get_url(const HorizonThreadSummary *ts) {
	g_return_val_if_fail (HORIZON_IS_THREAD_SUMMARY (ts), 0);

	return ts->priv->url;
}

const gchar*
horizon_thread_summary_get_thumb_url(const HorizonThreadSummary *ts) {
	g_return_val_if_fail (HORIZON_IS_THREAD_SUMMARY (ts), 0);

	return ts->priv->thumb_url;
}

void
horizon_thread_summary_set_thumb_pixbuf(HorizonThreadSummary *ts, GdkPixbuf* pixbuf) {
	g_return_if_fail (HORIZON_IS_THREAD_SUMMARY (ts));

	if (ts->priv->thumb_pixbuf)
		g_object_unref(ts->priv->thumb_pixbuf);

	ts->priv->thumb_pixbuf = pixbuf;
	g_object_ref(ts->priv->thumb_pixbuf);
}

GdkPixbuf*
horizon_thread_summary_get_thumb_pixbuf(HorizonThreadSummary *ts) {
	g_return_val_if_fail (HORIZON_IS_THREAD_SUMMARY (ts), 0);

	return ts->priv->thumb_pixbuf;
}
