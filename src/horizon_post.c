#include "horizon_post.h"

#define HORIZON_POST_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HORIZON_TYPE_POST, HorizonPostPrivate))

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

  N_PROPERTIES
};

struct _HorizonPostPrivate
{
	gint64    post_number;
	gint64    resto;
	gint  sticky;
	gint  closed;
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
	gint64   renamed_filename;
	gchar    *filename;
	gchar    *ext;
	gchar    *md5;
	gint64    fsize;
	gint      image_width;
	gint      image_height;
	gint      thumbnail_width;
	gint      thumbnail_height;
	gint  is_file_deleted;
	gint  is_spoiler;
	gint    custom_spoiler;

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

			
		default:
			/* We don't have any other property... */
	    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
horizon_post_init (HorizonPost *self)
{
  HorizonPostPrivate *priv;

  self->priv = priv = HORIZON_POST_GET_PRIVATE (self);

}

static void
horizon_post_dispose (GObject *gobject)
{
  HorizonPost *self = HORIZON_POST (gobject);

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
	                       NULL, /* default value */
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
	                       NULL, /* default value */
	                       G_PARAM_READWRITE);
  obj_properties[PROP_TRIP] =
	  g_param_spec_string ("trip",
	                       "Tripcode",
	                       "",
	                       NULL, /* default value */
	                       G_PARAM_READWRITE);
  obj_properties[PROP_ID] =
	  g_param_spec_string ("id",
	                       "ID",
	                       "Identifies if Mod or Admin",
	                       NULL, /* default value */
	                       G_PARAM_READWRITE);
  obj_properties[PROP_CAPCODE] =
	  g_param_spec_string ("capcode",
	                       "Capcode",
	                       "",
	                       NULL, /* default value */
	                       G_PARAM_READWRITE);
  obj_properties[PROP_COUNTRY] =
	  g_param_spec_string ("country",
	                       "Country Code",
	                       "",
	                       NULL, /* default value */
	                       G_PARAM_READWRITE);
  obj_properties[PROP_COUNTRY_NAME] =
	  g_param_spec_string ("country_name",
	                       "Country Name",
	                       "",
	                       NULL, /* default value */
	                       G_PARAM_READWRITE);
  obj_properties[PROP_EMAIL] =
	  g_param_spec_string ("email",
	                       "Email",
	                       "",
	                       NULL, /* default value */
	                       G_PARAM_READWRITE);
  obj_properties[PROP_SUBJECT] =
	  g_param_spec_string ("sub",
	                       "Subject",
	                       "",
	                       NULL, /* default value */
	                       G_PARAM_READWRITE);
  obj_properties[PROP_COMMENT] =
	  g_param_spec_string ("com",
	                       "Comment",
	                       "",
	                       NULL, /* default value */
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
	                       NULL, /* default value */
	                       G_PARAM_READWRITE);
  obj_properties[PROP_EXT] =
	  g_param_spec_string ("ext",
	                       "Filename extension",
	                       "",
	                       NULL, /* default value */
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
	                       NULL, /* default value */
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
  
  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);
  
}

