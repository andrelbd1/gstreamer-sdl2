#include <SDL2/SDL.h>
#include <glib.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/video/video.h>

static const int width = 800;
static const int height = 600;

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;

static void
eos_cb (GstAppSink *appsink, gpointer data)
{
  g_print ("eos\n");
}

static GstFlowReturn
new_preroll_cb (GstAppSink *appsink, gpointer data)
{
  return GST_FLOW_OK;
}

static GstFlowReturn
new_sample_cb (GstAppSink *appsink, gpointer data)
{
  GstSample* sample;
  GstVideoFrame v_frame;
  GstVideoInfo v_info;
  GstBuffer *buf;
  GstCaps *caps;
  guint8 *pixels;
  guint stride;

  sample = gst_app_sink_pull_sample (appsink);
  g_assert_nonnull (sample);

  buf = gst_sample_get_buffer (sample);
  g_assert_nonnull (buf);

  caps = gst_sample_get_caps (sample);
  g_assert_nonnull (caps);

  g_assert (gst_video_info_from_caps (&v_info, caps));
  g_assert (gst_video_frame_map (&v_frame, &v_info, buf, GST_MAP_READ));

  pixels = (guint8 *) GST_VIDEO_FRAME_PLANE_DATA (&v_frame, 0);
  stride = GST_VIDEO_FRAME_PLANE_STRIDE (&v_frame, 0);
  g_assert (SDL_UpdateTexture(texture, NULL, pixels, stride) == 0);
  SDL_RenderCopy (renderer, texture, NULL, NULL);
  SDL_RenderPresent (renderer);

  gst_video_frame_unmap (&v_frame);
  gst_sample_unref (sample);

  user_events();

  return GST_FLOW_OK;
}

void 
user_events()
{
  SDL_Event e;
  SDL_PollEvent(&e);

  if (e.type == SDL_QUIT){
    g_print("Quit");  
  }
  
  if (e.type == SDL_KEYDOWN){
    g_print("Keydown");  
  }

  if (e.type == SDL_MOUSEBUTTONDOWN){
    g_print("Mouse Button Down");
  }  
}

void
create_pipeline (int argc, char **argv)
{
  GstElement *playbin;
  GstElement *bin;
  GstElement *filter;
  GstElement *scale;
  GstElement *sink;

  GstAppSinkCallbacks callbacks;
  GstPad *pad;
  GstCaps *caps;
  GstStructure *st = NULL;

  char *uri;
  GstStateChangeReturn ret;
  GstBus *bus;
  GstMessage *msg;

  gst_init (&argc, &argv);

  playbin = gst_element_factory_make ("playbin", NULL);
  g_assert_nonnull (playbin);

  uri = gst_filename_to_uri ("bunny.ogg", NULL);
  g_assert_nonnull (uri);

  g_object_set (G_OBJECT (playbin), "uri", uri, NULL);
  g_free (uri);

  bin = gst_bin_new (NULL);
  g_assert_nonnull (bin);

  filter = gst_element_factory_make ("capsfilter", NULL);
  g_assert_nonnull (filter);

  st = gst_structure_new_empty ("video/x-raw");
  gst_structure_set (st, "format", G_TYPE_STRING, "RGB",
                     "width", G_TYPE_INT, width,
                     "height", G_TYPE_INT, height, NULL);

  caps = gst_caps_new_full (st, NULL);
  g_assert_nonnull (caps);

  g_object_set (filter, "caps", caps, NULL);
  gst_caps_unref (caps);

  scale = gst_element_factory_make ("videoscale", NULL);
  g_assert_nonnull (scale);

  sink = gst_element_factory_make ("appsink", NULL);
  g_assert_nonnull (sink);

  g_assert (gst_bin_add (GST_BIN (bin), filter));
  g_assert (gst_bin_add (GST_BIN (bin), sink));
  g_assert (gst_bin_add (GST_BIN (bin), scale));

  g_assert (gst_element_link (filter, scale));
  g_assert (gst_element_link (scale, sink));

  pad = gst_element_get_static_pad (filter, "sink");
  gst_element_add_pad (bin, gst_ghost_pad_new ("sink", pad));

  g_object_set (G_OBJECT (playbin), "video-sink", bin, NULL);

  ret = gst_element_set_state (playbin, GST_STATE_PLAYING);
  g_assert (ret != GST_STATE_CHANGE_FAILURE);

  callbacks.eos = eos_cb;
  callbacks.new_preroll = new_preroll_cb;
  callbacks.new_sample = new_sample_cb;
  gst_app_sink_set_callbacks (GST_APP_SINK (sink), &callbacks, NULL, NULL);

  bus = gst_element_get_bus (playbin);
  msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
                                    GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
  gst_message_unref (msg);
  gst_object_unref (bus);
  gst_element_set_state (playbin, GST_STATE_NULL);
  gst_object_unref (playbin);
  gst_object_unref (bin);
}

int main (int argc, char **argv)
{
  #if SDL_VERSION_ATLEAST(2,0,5)
    SDL_Log("SDL_VERSION %i.%i.%i is more or equal to 2.0.5.", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
  #else
    SDL_Log("SDL_VERSION %i.%i.%i is less than 2.0.5.", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
  #endif

  gst_init (&argc, &argv);
  g_assert (SDL_Init (0) == 0);

  SDL_SetHint(SDL_HINT_RENDER_DRIVER,"software");

  if(SDL_CreateWindowAndRenderer
            (width, height, SDL_WINDOW_SHOWN, &window, &renderer))
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, 
        "Couldn't create window and renderer: %s", SDL_GetError());
    return 0;
  }

  texture = SDL_CreateTexture (renderer, SDL_PIXELFORMAT_RGB24,
                               SDL_TEXTUREACCESS_TARGET, width, height);

  if(!texture)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, 
        "Couldn't create texture: %s", SDL_GetError());
    return 0;
  }
 
  create_pipeline (argc, argv);
  
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow (window);
  
  SDL_Quit();
  
  exit (EXIT_SUCCESS);
}
