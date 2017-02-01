#include <iostream>

#include <glib.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/video/video.h>

#include <SDL2/SDL.h>

static SDL_Renderer* ren;
static SDL_Texture* tex;
static GstElement* video_sink;
// static GstElement* audio_sink;

static const int WIDTH = 1280;
static const int HEIGHT = 720;


static GstFlowReturn new_buffer (GstAppSink *video_sink) {
  GstSample* sample = gst_app_sink_pull_sample (video_sink);
  if(!sample) {        
    return GST_FLOW_ERROR;
  }

    GstVideoFrame v_frame;
    GstVideoInfo v_info;
    GstBuffer *buf = gst_sample_get_buffer (sample);
    GstCaps *caps = gst_sample_get_caps (sample);

    g_assert_nonnull (buf);
    g_assert_nonnull (caps);

    gst_video_info_from_caps (&v_info, caps);
    gst_video_frame_map (&v_frame, &v_info, buf, GST_MAP_READ);


    static int printed = 0;
    if (!printed)
      {
        g_print ("%dx%d\n", GST_VIDEO_INFO_WIDTH (&v_info),
                 GST_VIDEO_INFO_HEIGHT (&v_info));
        g_print ("is interlaced? %d\n", GST_VIDEO_INFO_IS_INTERLACED (&v_info));
        g_print ("is rgb? %d\n", GST_VIDEO_INFO_IS_RGB (&v_info));
        g_print ("is yuv? %d\n", GST_VIDEO_INFO_IS_YUV (&v_info));
        printed = 1;

        g_print ("gst_caps_to_string: %s\n", gst_caps_to_string (caps));
      }

    guint8 *pixels = (guint8 *) GST_VIDEO_FRAME_PLANE_DATA (&v_frame, 0);
    guint stride = GST_VIDEO_FRAME_PLANE_STRIDE (&v_frame, 0);

    g_assert (SDL_UpdateTexture(tex, NULL, pixels, stride) == 0);

    gst_video_frame_unmap (&v_frame);
    gst_sample_unref(sample);

    return GST_FLOW_OK;
}

static bool texture(){
    GstElement *playbin; //Pipeline da aplicação
    char *uri;
    GstStateChangeReturn ret;
    GstBus *bus;
    GstCaps *caps;
    // GstMessage *msg;

    playbin = gst_element_factory_make ("playbin", "hello"); //Cria a pipeline. 1º param é o nome da fábrica do tipo. 2º param (opcional) é o nome a ser atribuído ao elemento retorno.
    g_assert_nonnull (playbin); //Verifica se a chamada anterior funcionou corretamente.

    uri = gst_filename_to_uri ("bunny.ogg", NULL); //Constrói a URI.
    g_assert_nonnull (uri); //Verifica se a chamada anterior funcionou corretamente.
    g_object_set (G_OBJECT (playbin), "uri", uri, NULL); //Atribui a URI à propriedade uri do elemento playbin.
    g_free (uri); ///Libera a string alocada.

    video_sink = gst_element_factory_make ("appsink", "video_sink");
    g_assert_nonnull (video_sink); //Verifica se a chamada anterior funcionou corretamente.    
    // audio_sink = gst_element_factory_make ("appsink", "audio_sink");
    // g_assert_nonnull (audio_sink); //Verifica se a chamada anterior funcionou corretamente.    

    // g_object_set (G_OBJECT (playbin), "audio-sink", audio_sink, NULL);
    caps = gst_caps_from_string ("video/x-raw,format=YUV,pixel-aspect-ratio=1/1");
    gst_app_sink_set_caps (video_sink, caps);
    gst_object_unref (caps);
    
    g_object_set (G_OBJECT (playbin), 
                  "video-sink", video_sink, 
                  NULL);

    ret = gst_element_set_state (playbin, GST_STATE_PLAYING); //Inicia a pipeline da aplicação.
    g_assert (ret != GST_STATE_CHANGE_FAILURE); //Verifica se requisição foi bem sucedida.

    bus = gst_element_get_bus (playbin); //Referência para o barramento de mensagens (bus) do pipeline.

    tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STATIC, WIDTH, HEIGHT);
    if (tex == nullptr){
        std::cout << "SDL_CreateTexture Error: " << SDL_GetError() << std::endl;
        return -9;
    }

    bool playing = true;
    while(playing){

        SDL_RenderClear(ren);
        if (new_buffer((GstAppSink*)video_sink) != GST_FLOW_OK)
            playing = false;
        else {
            SDL_RenderCopy(ren, tex, NULL, NULL);
            SDL_RenderPresent(ren);
        }
    }

    // msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS); //Bloqueia a thread da aplicação caso uma mensagem de erro ou de EOS (end-of-stream) seja postada no barramento.
    
    // gst_message_unref (msg); //Libera a mensagem retornada na linha anterior.
    gst_object_unref (bus); //Libera a referência para o barramento de mensagens (bus).
    gst_element_set_state (playbin, GST_STATE_NULL); //Para o pipeline da aplicação e libera os recursos.
    gst_object_unref (playbin); //Libera o elemento playbin.

    return true;
}

int main (int argc, char *argv[]){

    gst_init (&argc, &argv);

    //First we need to start up SDL, and make sure it went ok
    if (SDL_Init(SDL_INIT_VIDEO) != 0){
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create window and renderer
    SDL_Window *win = SDL_CreateWindow("GStreamer-SDL2", 100, 100, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    if (win == nullptr){ //Make sure creating our window went ok
            std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
            SDL_Quit();
            return 1;
    }

    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (ren == nullptr){
            std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
            SDL_DestroyWindow(win);
            SDL_Quit();
            return 1;
    }

    texture();

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}
