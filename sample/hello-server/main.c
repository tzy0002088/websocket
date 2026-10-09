#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include "websocket_service.h"

#define CMDLINE_MAX     (200)
static struct app_websocket ws;
static volatile int is_connect = 0;
static const char *fram_map[11] = {"continue", "text", "bin", "", "", "","", "", "close", "ping", "pong"};

static void ctrl_c(int s)
{
    printf("byby!!!\n");
    app_websocket_set_close_reason(&ws, WEBSOCKET_STATUS_CLOSE_NORMAL, NULL);
    app_websocket_disconnect_server(&ws);
    sleep(1);
    app_websocket_worker_deinit();
    exit(0);
}

static int onmessage(struct app_websocket *ws)
{
    struct app_websocket_frame frame;
    int len = app_websocket_read_data(ws, &frame);
    if(len > 0)
    {
        printf("frame type is: %s frame, fram data is: %s.\n", fram_map[frame.type], (char *)frame.data);
    }
    else if(len == 0){
        printf("frame type is: %s ", fram_map[frame.type]);
    }
    else {
        printf("recv error!!!!!!!\n");
    }

    return WEBSOCKET_OK;
}

static int onopen(struct app_websocket *ws)
{
    printf("connect websocket server success!!!\n");
    is_connect = 1;
    struct app_websocket_frame frame;
    frame.data = "hello server";
    frame.length = strlen("hello server");
    frame.type = WEBSOCKET_TEXT_FRAME;

    return app_websocket_write_data(ws, &frame);
}

static int onclose(struct app_websocket *ws)
{
    printf("server close session!!!\n");
    is_connect = 0;
    return 0;
}

int main(int argc, char *argv[])
{
    char cmdline[CMDLINE_MAX]= {0};
    const char *url = (argc > 1) ? argv[1] : "ws://127.0.0.1:9010";
    signal(SIGINT, ctrl_c);
    app_websocket_worker_init();
    int success = (
        (app_websocket_init(&ws) == WEBSOCKET_OK) &&
        (app_websocket_set_url(&ws, url) == WEBSOCKET_OK) &&
        (app_websocket_connect_server(&ws) == WEBSOCKET_OK)
    );

    if(success)
    {
        app_websocket_message_event(&ws, onmessage);
        app_websocket_open_event(&ws, onopen);
        app_websocket_close_event(&ws, onclose);
    }

    while(success)
    {
        if (fgets(cmdline, CMDLINE_MAX, stdin) == NULL)
        {
            printf("stdin closed, exit!!!\n");
            break;
        }

        cmdline[strlen(cmdline)-1]='\0';
        printf("cmdline len = %ld\n", strlen(cmdline));
        if(strcmp(cmdline, "exit") == 0)
        {
            break;
        }
        else if (is_connect)
        {
            struct app_websocket_frame frame;
            frame.data = cmdline;
            frame.length = strlen(cmdline);
            frame.type = WEBSOCKET_TEXT_FRAME;
            if (app_websocket_write_data(&ws, &frame) < 0)
            {
                fprintf(stderr,"write error, please check connect!!!\n");
                break;
            }
            else
            {
                printf("write [%s] success!!!!\n", cmdline);
            }
        }
        else
        {
            printf("websocket not connected, ignore input!!!\n");
        }
    }

    app_websocket_set_close_reason(&ws, WEBSOCKET_STATUS_CLOSE_NORMAL, NULL);
    app_websocket_disconnect_server(&ws);
    sleep(1);
    app_websocket_worker_deinit();
    exit(0);
}

