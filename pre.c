#include <glib.h>
#include <gst/gst.h>

int main (int argc, char *argv[]){
  GstElement *playbin; //Pipeline da aplicação
  char *uri;
  GstStateChangeReturn ret;
  GstBus *bus;
  GstMessage *msg;

  gst_init (&argc, &argv);

  playbin = gst_element_factory_make ("playbin", "hello"); //Cria a pipeline. 1º param é o nome da fábrica do tipo. 2º param (opcional) é o nome a ser atribuído ao elemento retorno.
  g_assert_nonnull (playbin); //Verifica se a chamada anterior funcionou corretamente.

  uri = gst_filename_to_uri ("bunny.ogg", NULL); //Constrói a URI.
  g_assert_nonnull (uri); //Verifica se a chamada anterior funcionou corretamente.
  g_object_set (G_OBJECT (playbin), "uri", uri, NULL); //Atribui a URI à propriedade uri do elemento playbin.
  g_free (uri); ///Libera a string alocada.
  

  GstElement *binVideo = gst_bin_new("binVideo");
  g_assert_nonnull (binVideo);  

  GstElement *caps_filter = gst_element_factory_make ("capsfilter", "caps_filter");
  g_assert_nonnull (caps_filter);
  gst_bin_add(GST_BIN(binVideo), caps_filter);

  GstCaps *caps = gst_caps_from_string ("video/x-raw,format=YUV,pixel-aspect-ratio=1/1");
  g_assert_nonnull (caps);
  g_object_set(caps_filter, "caps", caps, NULL);

  GstElement *video_convert = gst_element_factory_make ("videoconvert", "video_convert");
  g_assert_nonnull (video_convert);
  gst_bin_add(GST_BIN(binVideo), video_convert);    

  GstElement *app_sink_video = gst_element_factory_make ("autovideosink", "app_sink_video"); 
  g_assert_nonnull (app_sink_video);
  gst_bin_add(GST_BIN(binVideo), app_sink_video);

  gst_element_link_many (caps_filter, video_convert, app_sink_video, NULL); //Conecta os elementos em série.

  GstPad *pad = gst_element_get_static_pad(caps_filter, "sink");
  gst_element_add_pad(binVideo, gst_ghost_pad_new ("sink", pad));


  g_object_set (G_OBJECT (playbin), "video-sink", binVideo, NULL);

  ret = gst_element_set_state (playbin, GST_STATE_PLAYING); //Inicia a pipeline da aplicação.
  g_assert (ret != GST_STATE_CHANGE_FAILURE); //Verifica se requisição foi bem sucedida.

  bus = gst_element_get_bus (playbin); //Referência para o barramento de mensagens (bus) do pipeline.
  msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS); //Bloqueia a thread da aplicação caso uma mensagem de erro ou de EOS (end-of-stream) seja postada no barramento.

  gst_message_unref (msg); //Libera a mensagem retornada na linha anterior.
  gst_object_unref (bus); //Libera a referência para o barramento de mensagens (bus).
  gst_element_set_state (playbin, GST_STATE_NULL); //Para o pipeline da aplicação e libera os recursos.
  gst_object_unref (playbin); //Libera o elemento playbin.

  return 0;

}