#include "horizon_post.h"

#define HORIZON_POST_GET_PRIVATE(obj) ((HorizonPostPrivate *)((HORIZON_POST(obj))->priv))

G_DEFINE_TYPE (HorizonPost, horizon_post, G_TYPE_OBJECT);

enum
{
  PROP_0,

  PROP_POST_NUMBER,
  PROP_RESTO,
  PROP_STICKY,
  PROP_CLOSED,
  PROP_NOW,
  PROP_TIME,
  PROP_NAME,
  PROP_TRIP,
  PROP_ID,
  PROP_CAPCODE,
  PROP_COUNTRY,
  PROP_COUNTRY_NAME,
  PROP_EMAIL,
  PROP_SUBJECT,
  PROP_COMMENT,
  PROP_RENAMED_FILENAME,
  PROP_FILENAME,
  PROP_EXT,
  PROP_FSIZE,
  PROP_MD5,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_THUMBNAIL_WIDTH,
  PROP_THUMBNAIL_HEIGHT,
  PROP_FILEDELETED,
  PROP_SPOILER,
  PROP_CUSTOM_SPOILER,
  PROP_IMAGE_URL,
  PROP_THUMB_URL,
  PROP_BOARD,
  PROP_RENDERED,
  PROP_THREAD_ID,

  N_PROPERTIES
};

struct _HorizonPostPrivate
{
	gint64    post_number;
	gint64    resto;
	gint      sticky;
	gint      closed;
	gint64    time;
	gchar    *now;
	gchar    *name;
	gchar    *trip;
	gchar    *id;
	gchar    *capcode;
	gchar    *country_code;
	gchar    *country_name;
	gchar    *email;
	gchar    *subject;
	gchar    *comment;
	gint64    renamed_filename;
	gchar    *filename;
	gchar    *ext;
	gchar    *md5;
	gint64    fsize;
	gint      image_width;
	gint      image_height;
	gint      thumbnail_width;
	gint      thumbnail_height;
	gint      is_file_deleted;
	gint      is_spoiler;
	gint      custom_spoiler;
	gchar    *image_url;
	gchar    *thumb_url;
	gchar    *board;
	gboolean  rendered;
	gint64    thread_id;
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
horizon_post_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
	HorizonPost *self = HORIZON_POST (object);

	switch (property_id)
		{
		case PROP_POST_NUMBER:
			self->priv->post_number = g_value_get_int64 (value);
			break;
		case PROP_RESTO:
			self->priv->resto = g_value_get_int64 (value);
			break;
		case PROP_STICKY:
			self->priv->sticky   = g_value_get_int  (value);
			break;
		case PROP_CLOSED:
			self->priv->closed  = g_value_get_int  (value);
			break;
		case PROP_NOW:
			g_free (self->priv->now);
			self->priv->now  = g_value_dup_string  (value);
			break;
		case PROP_TIME:
			self->priv->time  = g_value_get_int64  (value);
			break;
		case PROP_NAME:
			g_free (self->priv->name);
			self->priv->name  = g_value_dup_string  (value);
			break;
		case PROP_TRIP:
			g_free (self->priv->trip);
			self->priv->trip  = g_value_dup_string  (value);
			break;
		case PROP_ID:
			g_free (self->priv->id);
			self->priv->id  = g_value_dup_string  (value);
			break;
		case PROP_CAPCODE:
			g_free (self->priv->capcode);
			self->priv->capcode  = g_value_dup_string  (value);
			break;
		case PROP_COUNTRY:
			g_free (self->priv->country_code);
			self->priv->country_code  = g_value_dup_string  (value);
			break;
		case PROP_COUNTRY_NAME:
			g_free (self->priv->country_name);
			self->priv->country_name  = g_value_dup_string  (value);
			break;
		case PROP_EMAIL:
			g_free (self->priv->email);
			self->priv->email  = g_value_dup_string  (value);
			break;
		case PROP_SUBJECT:
			g_free (self->priv->subject);
			self->priv->subject  = g_value_dup_string  (value);
			break;
		case PROP_COMMENT:
			g_free (self->priv->comment);
			self->priv->comment  = g_value_dup_string  (value);
			break;
		case PROP_RENAMED_FILENAME:
			self->priv->renamed_filename  = g_value_get_int64  (value);
			break;
		case PROP_FILENAME:
			g_free (self->priv->filename);
			self->priv->filename  = g_value_dup_string  (value);
			break;
		case PROP_EXT:
			g_free (self->priv->ext);
			self->priv->ext  = g_value_dup_string  (value);
			break;
		case PROP_FSIZE:
			self->priv->fsize  = g_value_get_int64  (value);
			break;
		case PROP_MD5:
			g_free (self->priv->md5);
			self->priv->md5  = g_value_dup_string  (value);
			break;
		case PROP_WIDTH:
			self->priv->image_width  = g_value_get_int  (value);
			break;
		case PROP_HEIGHT:
			self->priv->image_height  = g_value_get_int  (value);
			break;
		case PROP_THUMBNAIL_WIDTH:
			self->priv->thumbnail_width  = g_value_get_int  (value);
			break;
		case PROP_THUMBNAIL_HEIGHT:
			self->priv->thumbnail_height  = g_value_get_int  (value);
			break;
		case PROP_FILEDELETED:
			self->priv->is_file_deleted  = g_value_get_int  (value);
			break;
		case PROP_SPOILER:
			self->priv->is_spoiler  = g_value_get_int  (value);
			break;
		case PROP_CUSTOM_SPOILER:
			self->priv->custom_spoiler  = g_value_get_int  (value);
			break;
		case PROP_IMAGE_URL:
			g_free(self->priv->image_url);
			self->priv->image_url = g_value_dup_string (value);
			break;
		case PROP_THUMB_URL:
			g_free(self->priv->thumb_url);
			self->priv->thumb_url = g_value_dup_string (value);
			break;
		case PROP_BOARD:
			g_free(self->priv->board);
			self->priv->board = g_value_dup_string (value);
			break;
		case PROP_RENDERED:
			self->priv->rendered = g_value_get_boolean (value);
			break;
		case PROP_THREAD_ID:
			self->priv->thread_id = g_value_get_int64 (value);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
		}
}

static void
horizon_post_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
	HorizonPost *self = HORIZON_POST (object);

	switch (property_id)
		{
		case PROP_POST_NUMBER:
			g_value_set_int64 (value, self->priv->post_number);
			break;
		case PROP_RESTO:
			g_value_set_int64 (value, self->priv->resto);
			break;
		case PROP_STICKY:
			g_value_set_int  (value, self->priv->sticky  );
			break;
		case PROP_CLOSED:
			g_value_set_int (value, self->priv->closed  );
			break;
		case PROP_NOW:
			g_value_set_string (value, self->priv->now  );
			break;
		case PROP_TIME:
			g_value_set_int64 (value, self->priv->time  );
			break;
		case PROP_NAME:
			g_value_set_string  (value, self->priv->name  );
			break;
		case PROP_TRIP:
			g_value_set_string  (value, self->priv->trip  );
			break;
		case PROP_ID:
			g_value_set_string  (value, self->priv->id  );
			break;
		case PROP_CAPCODE:
			g_value_set_string  (value, self->priv->capcode  );
			break;
		case PROP_COUNTRY:
			g_value_set_string  (value, self->priv->country_code  );
			break;
		case PROP_COUNTRY_NAME:
			g_value_set_string  (value, self->priv->country_name  );
			break;
		case PROP_EMAIL:
			g_value_set_string  (value, self->priv->email  );
			break;
		case PROP_SUBJECT:
			g_value_set_string  (value, self->priv->subject  );
			break;
		case PROP_COMMENT:
			g_value_set_string  (value, self->priv->comment  );
			break;
		case PROP_RENAMED_FILENAME:
			g_value_set_int64  (value, self->priv->renamed_filename  );
			break;
		case PROP_FILENAME:
			g_value_set_string  (value, self->priv->filename  );
			break;
		case PROP_EXT:
			g_value_set_string  (value, self->priv->ext  );
			break;
		case PROP_FSIZE:
			g_value_set_int64 (value, self->priv->fsize  );
			break;
		case PROP_MD5:
			g_value_set_string  (value, self->priv->md5  );
			break;
		case PROP_WIDTH:
			g_value_set_int (value, self->priv->image_width  );
			break;
		case PROP_HEIGHT:
			g_value_set_int (value, self->priv->image_height  );
			break;
		case PROP_THUMBNAIL_WIDTH:
			g_value_set_int (value, self->priv->thumbnail_width  );
			break;
		case PROP_THUMBNAIL_HEIGHT:
			g_value_set_int (value, self->priv->thumbnail_height  );
			break;
		case PROP_FILEDELETED:
			g_value_set_int (value, self->priv->is_file_deleted  );
			break;
		case PROP_SPOILER:
			g_value_set_int (value, self->priv->is_spoiler  );
			break;
		case PROP_CUSTOM_SPOILER:
			g_value_set_int (value, self->priv->custom_spoiler  );
			break;
		case PROP_IMAGE_URL:
			g_value_set_string  (value, horizon_post_get_image_url(self));
			break;
		case PROP_THUMB_URL:
			g_value_set_string  (value, horizon_post_get_thumb_url(self));
			break;
		case PROP_BOARD:
			g_value_set_string  (value, self->priv->board);
			break;
		case PROP_RENDERED:
			g_value_set_boolean  (value, self->priv->rendered);
			break;
		case PROP_THREAD_ID:
			g_value_set_int64 (value, self->priv->thread_id);
			break;
		default:
			/* We don't have any other property... */
	    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
horizon_post_init (HorizonPost *self)
{
  HorizonPostPrivate *priv = NULL;

  self->priv = priv = (HorizonPostPrivate*) g_malloc0(sizeof(HorizonPostPrivate));
}

static void
horizon_post_dispose (GObject *gobject)
{
	//HorizonPost *self = HORIZON_POST (gobject);

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
  G_OBJECT_CLASS (horizon_post_parent_class)->dispose (gobject);
}

static void
horizon_post_finalize (GObject *gobject)
{
  HorizonPost *self = HORIZON_POST (gobject);

  g_free (self->priv->now);
  g_free (self->priv->name);
  g_free (self->priv->trip);
  g_free (self->priv->id);
  g_free (self->priv->capcode);
  g_free (self->priv->country_code);
  g_free (self->priv->country_name);
  g_free (self->priv->email);
  g_free (self->priv->subject);
  g_free (self->priv->comment);
  g_free (self->priv->filename);
  g_free (self->priv->ext);
  g_free (self->priv->md5);
  g_free (self->priv->thumb_url);
  g_free (self->priv->image_url);
  g_free (self->priv->board);

  g_free (self->priv);
  /* Chain up to the parent class */
  G_OBJECT_CLASS (horizon_post_parent_class)->finalize (gobject);
}




static void
horizon_post_class_init (HorizonPostClass *klass)
{
  g_type_class_add_private (klass, sizeof (HorizonPostPrivate));

  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->set_property = horizon_post_set_property;
  gobject_class->get_property = horizon_post_get_property;
  gobject_class->dispose      = horizon_post_dispose;
  gobject_class->finalize     = horizon_post_finalize;

  obj_properties[PROP_POST_NUMBER] =
	  g_param_spec_int64 ("no", /* name */
	                       "post number", /* nick */
	                       "Post's unique identifier", /* blurb */
	                       1  /* minimum value */,
	                       9999999999999 /* maximum value */,
	                       1  /* default value */,
	                       G_PARAM_READWRITE);
  
  obj_properties[PROP_RESTO] =
	  g_param_spec_int64 ("resto",
	                       "Reply to",
	                       "Post is in reply to this post id",
	                       0  /* minimum value */,
	                       9999999999999 /* maximum value */,
	                       0  /* default value */,
	                       G_PARAM_READWRITE);
  obj_properties[PROP_STICKY] =
	  g_param_spec_int ("sticky",
	                    "is sticky",
	                    "",
	                    0,
	                    1,
	                    0, /* default value */
	                    G_PARAM_READWRITE);
  obj_properties[PROP_CLOSED] =
	  g_param_spec_int ("closed",
	                    "thread is closed",
	                    "",
	                    0,
	                    1,
	                    0, /* default value */
	                    G_PARAM_READWRITE);
  obj_properties[PROP_NOW] =
	  g_param_spec_string ("now",
	                       "Date and time",
	                       "",
	                       "no-now-set", /* default value */
	                       G_PARAM_READWRITE);
  obj_properties[PROP_TIME] =
	  g_param_spec_int64 ("time",
	                      "UNIX timestamp",
	                      "",
	                      0,
	                      G_MAXINT64,
	                      0, /* default value */
	                      G_PARAM_READWRITE);
  obj_properties[PROP_NAME] =
	  g_param_spec_string ("name",
	                       "Name",
	                       "",
	                       "no-name-set", /* default value */
	                       G_PARAM_READWRITE);
  obj_properties[PROP_TRIP] =
	  g_param_spec_string ("trip",
	                       "Tripcode",
	                       "",
	                       "no-trip-set", /* default value */
	                       G_PARAM_READWRITE);
  obj_properties[PROP_ID] =
	  g_param_spec_string ("id",
	                       "ID",
	                       "Identifies if Mod or Admin",
	                       "no-id-set", /* default value */
	                       G_PARAM_READWRITE);
  obj_properties[PROP_CAPCODE] =
	  g_param_spec_string ("capcode",
	                       "Capcode",
	                       "",
	                       "no-capcode-set", /* default value */
	                       G_PARAM_READWRITE);
  obj_properties[PROP_COUNTRY] =
	  g_param_spec_string ("country",
	                       "Country Code",
	                       "",
	                       "no-country-set", /* default value */
	                       G_PARAM_READWRITE);
  obj_properties[PROP_COUNTRY_NAME] =
	  g_param_spec_string ("country_name",
	                       "Country Name",
	                       "",
	                       "no-country-name-set", /* default value */
	                       G_PARAM_READWRITE);
  obj_properties[PROP_EMAIL] =
	  g_param_spec_string ("email",
	                       "Email",
	                       "",
	                       "no-email-set", /* default value */
	                       G_PARAM_READWRITE);
  obj_properties[PROP_SUBJECT] =
	  g_param_spec_string ("sub",
	                       "Subject",
	                       "",
	                       "no-sub-set", /* default value */
	                       G_PARAM_READWRITE);
  obj_properties[PROP_COMMENT] =
	  g_param_spec_string ("com",
	                       "Comment",
	                       "",
	                       "no-com-set", /* default value */
	                       G_PARAM_READWRITE);
  obj_properties[PROP_RENAMED_FILENAME] =
	  g_param_spec_int64 ("tim",
	                      "Renamed filename",
	                      "",
	                      0,
	                      G_MAXINT64,
	                      0, /* default value */
	                      G_PARAM_READWRITE);
  obj_properties[PROP_FILENAME] =
	  g_param_spec_string ("filename",
	                       "Original filename",
	                       "",
	                       "no-filename-set", /* default value */
	                       G_PARAM_READWRITE);
  obj_properties[PROP_EXT] =
	  g_param_spec_string ("ext",
	                       "Filename extension",
	                       "",
	                       "no-ext-set", /* default value */
	                       G_PARAM_READWRITE);
  obj_properties[PROP_FSIZE] =
	  g_param_spec_int64 ("fsize",
	                       "File size",
	                       "",
	                       0,
	                       G_MAXINT64,
	                       0, /* default value */
	                       G_PARAM_READWRITE);
  obj_properties[PROP_MD5] =
	  g_param_spec_string ("md5",
	                       "File MD5SUM",
	                       "24 character base64 MD5",
	                       "no-md5-set", /* default value */
	                       G_PARAM_READWRITE);
  obj_properties[PROP_WIDTH] =
	  g_param_spec_int ("w",
	                    "Image width",
	                    "",
	                    0,
	                    10000,
	                    0, /* default value */
	                    G_PARAM_READWRITE);
  obj_properties[PROP_HEIGHT] =
	  g_param_spec_int ("h",
	                    "Image height",
	                    "",
	                    0,
	                    10000,
	                    0, /* default value */
	                    G_PARAM_READWRITE);
  obj_properties[PROP_THUMBNAIL_WIDTH] =
	  g_param_spec_int ("tn_w",
	                    "Thumbnail width",
	                    "",
	                    0,
	                    250,
	                    0, /* default value */
	                    G_PARAM_READWRITE);
  obj_properties[PROP_THUMBNAIL_HEIGHT] =
	  g_param_spec_int ("tn_h",
	                    "Thumbnail height",
	                    "",
	                    0,
	                    250,
	                    0, /* default value */
	                    G_PARAM_READWRITE);
  obj_properties[PROP_FILEDELETED] =
	  g_param_spec_int ("filedeleted",
	                    "File deleted?",
	                    "",
	                    0,
	                    1,
	                    0, /* default value */
	                    G_PARAM_READWRITE);
  obj_properties[PROP_SPOILER] =
	  g_param_spec_int ("spoiler",
	                    "Spoiler Image?",
	                    "",
	                    0,
	                    1,
	                    0, /* default value */
	                    G_PARAM_READWRITE);
  obj_properties[PROP_CUSTOM_SPOILER] =
	  g_param_spec_int ("custom_spoiler",
	                    "Custom Spoilers?",
	                    "",
	                    0,
	                    99,
	                    0, /* default value */
	                    G_PARAM_READWRITE);
  obj_properties[PROP_IMAGE_URL] =
	  g_param_spec_string ("image_url",
	                       "URL for image",
	                       "",
	                       "no-image-url-set", /* default value */
	                       G_PARAM_READWRITE);
  obj_properties[PROP_THUMB_URL] =
	  g_param_spec_string ("thumb_url",
	                       "URL for thumbnail",
	                       "",
	                       "no-thumb-url-set", /* default value */
	                       G_PARAM_READWRITE);
  obj_properties[PROP_BOARD] =
	  g_param_spec_string ("board",
	                       "Image board",
	                       "",
	                       "no-board-set", /* default value */
	                       G_PARAM_READWRITE);
  obj_properties[PROP_RENDERED] =
	  g_param_spec_boolean ("rendered",
	                        "Whether the post is rendered in a postview",
	                        "",
	                        FALSE,
	                        G_PARAM_READWRITE);

  obj_properties[PROP_THREAD_ID] =
	  g_param_spec_int64 ("thread_id", /* name */
	                       "Parent thread id", /* nick */
	                       "Id of the thread that contains this post", /* blurb */
	                       1  /* minimum value */,
	                       9999999999999 /* maximum value */,
	                       1  /* default value */,
	                       G_PARAM_READWRITE);
  
  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);
  
}


const gchar *
horizon_post_get_md5 (const HorizonPost *post)
{
	g_return_val_if_fail (HORIZON_IS_POST (post), NULL);

	if (post->priv->md5 == NULL)
		return NULL;
	else
		return post->priv->md5;
}

gint64 horizon_post_get_fsize (const HorizonPost *post)
{
	g_return_val_if_fail (HORIZON_IS_POST (post), 0);

	return post->priv->fsize;
}

gint64 horizon_post_get_post_number (const HorizonPost *post)
{
	g_return_val_if_fail (HORIZON_IS_POST (post), 0);

	return post->priv->post_number;
}

gint64 horizon_post_get_time (const HorizonPost *post)
{
	g_return_val_if_fail (HORIZON_IS_POST (post), 0);

	return post->priv->time;
}

gint64 horizon_post_get_renamed_filename (const HorizonPost *post)
{
	g_return_val_if_fail (HORIZON_IS_POST (post), 0);

	return post->priv->renamed_filename;
}

const gchar *
horizon_post_get_name (const HorizonPost *post)
{
	g_return_val_if_fail (HORIZON_IS_POST (post), NULL);

	return post->priv->name;
}

const gchar *
horizon_post_get_tripcode (const HorizonPost *post)
{
	g_return_val_if_fail(HORIZON_IS_POST(post), NULL);

	return post->priv->trip;
}

const gchar *
horizon_post_get_subject (const HorizonPost *post)
{
	g_return_val_if_fail (HORIZON_IS_POST (post), NULL);

	return post->priv->subject;
}

const gchar *
horizon_post_get_comment (const HorizonPost *post)
{
	g_return_val_if_fail (HORIZON_IS_POST (post), NULL);

	return post->priv->comment;
}

const gchar *
horizon_post_get_original_filename (const HorizonPost *post)
{
	g_return_val_if_fail (HORIZON_IS_POST (post), NULL);

	return post->priv->filename;
}

const gchar *
horizon_post_get_ext (const HorizonPost *post)
{
	g_return_val_if_fail (HORIZON_IS_POST (post), NULL);

	return post->priv->ext;
}

gint64 horizon_post_get_width (const HorizonPost *post)
{
	g_return_val_if_fail (HORIZON_IS_POST (post), 0);

	return post->priv->image_width;
}

gint64 horizon_post_get_height (const HorizonPost *post)
{
	g_return_val_if_fail (HORIZON_IS_POST (post), 0);

	return post->priv->image_height;
}

gint64 horizon_post_get_thumbnail_width (const HorizonPost *post)
{
	g_return_val_if_fail (HORIZON_IS_POST (post), 0);

	return post->priv->thumbnail_width;
}

gint64 horizon_post_get_thumbnail_height (const HorizonPost *post)
{
	g_return_val_if_fail (HORIZON_IS_POST (post), 0);

	return post->priv->thumbnail_height;
}

gint horizon_post_get_sticky(const HorizonPost *post) {
	g_return_val_if_fail (HORIZON_IS_POST (post), 0);

	return post->priv->sticky;
}

gint horizon_post_get_closed(const HorizonPost *post) {
	g_return_val_if_fail (HORIZON_IS_POST (post), 0);

	return post->priv->closed;
}

gint horizon_post_get_deleted(const HorizonPost *post) {
	g_return_val_if_fail (HORIZON_IS_POST (post), 0);

	return post->priv->is_file_deleted;
}

gint horizon_post_get_spoiler(const HorizonPost *post) {
	g_return_val_if_fail (HORIZON_IS_POST (post), 0);

	return post->priv->is_spoiler;
}

const gchar *
horizon_post_get_board (const HorizonPost *post) {
	return post->priv->board;
}

const gchar *
horizon_post_set_board (HorizonPost *post, const gchar *board) {
	return post->priv->board = g_strdup(board);
}

gint64 
horizon_post_get_thread_id(const HorizonPost *post) {
	return post->priv->thread_id;
}

gint64 
horizon_post_set_thread_id(HorizonPost *post, const gint64 id) {
	return post->priv->thread_id = id;
}

const gchar *
horizon_post_get_thumb_url (HorizonPost *post) {
	if (!post->priv->thumb_url) {
		g_return_val_if_fail(post->priv->board, NULL);

		post->priv->thumb_url = g_strdup_printf("http://thumbs.4chan.org/%s/thumb/%"
		                                        G_GINT64_FORMAT
		                                        "s.jpg",
		                                        post->priv->board,
		                                        post->priv->renamed_filename);
	}

	return post->priv->thumb_url;
}

void
horizon_post_set_thumb_url (HorizonPost *post, const gchar* url) {
	if (post->priv->thumb_url)
		g_free(post->priv->thumb_url);

	post->priv->thumb_url = g_strdup(url);
}

const gchar *
horizon_post_get_image_url (HorizonPost *post) {
	if (!post->priv->image_url) {
		g_return_val_if_fail(post->priv->board, NULL);

		post->priv->image_url = g_strdup_printf("http://images.4chan.org/%s/src/"
		                                        "%"G_GINT64_FORMAT
		                                        "%s",
		                                        post->priv->board,
		                                        post->priv->renamed_filename,
		                                        post->priv->ext);
	}

	return post->priv->image_url;
}

gboolean
horizon_post_is_gif(const HorizonPost *post) {
	return g_str_has_suffix(post->priv->ext, "gif");
}

gboolean
horizon_post_has_image(const HorizonPost *post) {
	return post->priv->fsize > 0;
}

gboolean
horizon_post_is_rendered(const HorizonPost *post) {
	return post->priv->rendered;
}

gboolean
horizon_post_set_rendered(HorizonPost *post, gboolean rendered) {
	return post->priv->rendered = rendered;
}

gboolean
horizon_post_is_same_post(const HorizonPost *left, const HorizonPost *right) {
	HorizonPostPrivate *lpriv = left->priv;
	HorizonPostPrivate *rpriv = right->priv;

	return lpriv->post_number == rpriv->post_number &&
		lpriv->sticky == rpriv->sticky &&
		lpriv->closed == rpriv->closed &&
		lpriv->is_file_deleted == rpriv->is_file_deleted;
}

gboolean
horizon_post_is_not_same_post(const HorizonPost *left, const HorizonPost *right) {
	HorizonPostPrivate *lpriv = left->priv;
	HorizonPostPrivate *rpriv = right->priv;

	return lpriv->post_number != rpriv->post_number ||
		lpriv->sticky != rpriv->sticky ||
		lpriv->closed != rpriv->closed ||
		lpriv->is_file_deleted != rpriv->is_file_deleted;
}

const gchar *
horizon_post_get_capcode(const HorizonPost *post) {
	return post->priv->capcode;
}
